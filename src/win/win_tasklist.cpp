#include <windows.h>
#include <tlhelp32.h>

#include <memory>
#include <stdint.h>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <codecvt>

#include "tasklist.h"

#define MAX_NAME 256

// std way to convert wstring to string
std::string ws2s(const std::wstring& wstr) {
    typedef std::codecvt_utf8<wchar_t> convert_typeX;
    std::wstring_convert<convert_typeX, wchar_t> converterX;

    return converterX.to_bytes(wstr);
}

class HandleProcess {
public:
	HandleProcess (uint32_t pid) {
		hProcess = OpenProcess (PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);

		if (!hProcess) {
			throw std::bad_alloc ();
		}
	}

	~HandleProcess () {
		if (hProcess) {
			CloseHandle (hProcess);
		}
	}

	// full image name of process
	std::string path () const {
		WCHAR *pProcessName = new WCHAR[MAX_PATH];
		DWORD pProcessNameSize = MAX_PATH;

		BOOL iResult = QueryFullProcessImageName (hProcess, 0, pProcessName, &pProcessNameSize);

		if (!iResult) {
			return std::string ();
		}

		std::wstring name (pProcessName, pProcessNameSize);
		delete[] pProcessName;

		return ws2s (name);
	}

	// owner of the process
	std::string owner () const {
		HANDLE hToken = nullptr;
		TOKEN_USER *pUserInfo = nullptr;
		DWORD pTokenSize = 0;
		SID_NAME_USE SidType;

		WCHAR *pUserName = new WCHAR[MAX_NAME];
		DWORD pUserSize = MAX_NAME;

		WCHAR *pDomainName = new WCHAR[MAX_NAME];
		DWORD pDomainSize = MAX_NAME;

		//open the processes token
		if (!OpenProcessToken (hProcess, TOKEN_QUERY, &hToken)) {
			delete[] pUserName;
			delete[] pDomainName;

			return std::string ();
		}

		// get the buffer size of the token
		if (!GetTokenInformation (hToken, TokenUser, NULL, pTokenSize, &pTokenSize)) {
			DWORD dwResult = GetLastError ();

			if (dwResult != ERROR_INSUFFICIENT_BUFFER) {
				delete[] pUserName;
				delete[] pDomainName;
				CloseHandle (hToken);

				return std::string ();
			}
		}

		if (!pTokenSize) {
			delete[] pUserName;
			delete[] pDomainName;
			CloseHandle (hToken);

			return std::string ();
		}

		// Allocate the buffer of the token
		pUserInfo = (TOKEN_USER*) HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, pTokenSize);

		if (!pUserInfo) {
			delete[] pUserName;
			delete[] pDomainName;
			CloseHandle (hToken);

			return std::string ();
		}

		// Call GetTokenInformation again to get the SID of the token
		if (!GetTokenInformation (hToken, TokenUser, pUserInfo, pTokenSize, &pTokenSize)) {
			HeapFree (GetProcessHeap (), 0, pUserInfo);
			delete[] pUserName;
			delete[] pDomainName;
			CloseHandle (hToken);

			return std::string ();
		}

		//get the account/domain name of the SID
		if (!LookupAccountSid (NULL, pUserInfo->User.Sid, pUserName, &pUserSize, pDomainName, &pDomainSize, &SidType)) {
			HeapFree (GetProcessHeap (), 0, pUserInfo);
			delete[] pUserName;
			delete[] pDomainName;
			CloseHandle (hToken);

			return std::string ();
		}

		std::wstring username (pUserName, pUserSize);

		HeapFree (GetProcessHeap (), 0, pUserInfo);
		delete[] pUserName;
		delete[] pDomainName;
		CloseHandle (hToken);

		return ws2s (username);
	}

private:
	HANDLE hProcess;

	HandleProcess () = delete;
};

/* структура, содержащая базовую инфу о процессе */
class Entry : public Process {
public:
	Entry (std::shared_ptr<PROCESSENTRY32> entry)
		: pEntry (std::move (entry)) 
	{
		pProcess = nullptr;

		try {
			pProcess = new HandleProcess ( pid() );
		} catch (const std::bad_alloc &) {
		}
	}

	Entry (Entry&& v)
		: pEntry(std::move(v.pEntry)) 
	{ }

	~Entry () {
		if (pProcess) {
			delete pProcess;
		}
	}

	static std::shared_ptr<PROCESSENTRY32> Factory () {
		std::shared_ptr<PROCESSENTRY32> entry = std::make_shared<PROCESSENTRY32> ();
		entry->dwSize = sizeof (PROCESSENTRY32);

		return std::move (entry);
	}

	inline static std::shared_ptr<Entry> New (std::shared_ptr<PROCESSENTRY32> bEntry) {
		return std::make_shared<Entry> (std::move (bEntry));
	}

	inline uint32_t pid () const override {
		return pEntry->th32ProcessID;
	}

	inline uint32_t parentPid () const {
		return pEntry->th32ParentProcessID;
	}

	std::string name () const override {
		return  ws2s (pEntry->szExeFile);
	}

	std::string path () const override {
		std::string fullname;

		if (pProcess) {
			fullname = pProcess->path ();
		}

		return fullname;
	}

	std::string owner () const override {
		std::string username;

		if (pProcess) {
			username = pProcess->owner();
		}

		return username;
	}

	inline uint32_t threads() const override {
		return pEntry->cntThreads;
	}

	inline int32_t priority () const override {
		return pEntry->pcPriClassBase;
	}

private:
	std::shared_ptr<PROCESSENTRY32> pEntry;
	HandleProcess *pProcess;

	Entry () = delete;
	Entry (const Entry&) = delete;
	Entry& operator=(const Entry&) = delete;
};

/* слепок системы - список процессов */
class Snapshot {
public:
	Snapshot () {
		hProcessSnap = CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0);

		if (hProcessSnap == INVALID_HANDLE_VALUE) {
			throw std::bad_alloc ();
		}
	}

	~Snapshot () {
		if (hProcessSnap != INVALID_HANDLE_VALUE) {
			CloseHandle (hProcessSnap);
		}
	}

	void prepare () {

		// мы уже получали список процессов для этого снимка
		if (scope.size () > 0) {
			return;
		}

		this->takeFirst ();

		while (this->takeNext()) { }
	}

	inline std::vector< std::shared_ptr<Process> > list () const {
		return scope;
	}

private:
	HANDLE hProcessSnap;
	std::vector< std::shared_ptr<Process> > scope;

	void takeFirst () {
		std::shared_ptr<PROCESSENTRY32> pcEntry = Entry::Factory ();

		if (!Process32First (hProcessSnap, pcEntry.get ())) {
			throw new std::logic_error ("Process32First");
		}

		std::shared_ptr<Entry> entry = Entry::New (std::move (pcEntry));
		scope.push_back (std::move (entry));
	}

	bool takeNext () {
		std::shared_ptr<PROCESSENTRY32> pcEntry = Entry::Factory ();

		bool status = Process32Next (hProcessSnap, pcEntry.get ());

		if (status) {
			std::shared_ptr<Entry> entry = Entry::New (std::move (pcEntry));
			scope.push_back (std::move (entry));
		}

		return status;
	}
};


std::vector< std::shared_ptr<Process> > tasklist () {
	std::shared_ptr<Snapshot> snap = std::make_shared<Snapshot> ();
	snap->prepare ();

	return snap->list ();
}
