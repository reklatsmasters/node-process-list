#include <windows.h>
#include <tlhelp32.h>

#include <memory>
#include <stdint.h>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <codecvt>

#include "tasklist.h"

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

private:
	HANDLE hProcess;

	HandleProcess () = delete;
};

/* структура, содержащая базовую инфу о процессе */
class Entry : public Process {
public:
	Entry (std::shared_ptr<PROCESSENTRY32> entry)
		: pEntry (std::move (entry)) 
	{ }

	Entry (Entry&& v)
		: pEntry(std::move(v.pEntry)) 
	{ }

	static std::shared_ptr<PROCESSENTRY32> Factory () {
		std::shared_ptr<PROCESSENTRY32> entry = std::make_shared<PROCESSENTRY32> ();
		entry->dwSize = sizeof (PROCESSENTRY32);

		return std::move (entry);
	}

	inline static std::shared_ptr<Entry> New (std::shared_ptr<PROCESSENTRY32> bEntry) {
		return std::make_shared<Entry> (std::move (bEntry));
	}

	uint32_t pid () const {
		return pEntry->th32ProcessID;
	}

	uint32_t parentPid () const {
		return pEntry->th32ParentProcessID;
	}

	std::string name () const {
		return  ws2s (pEntry->szExeFile);
	}

	std::string path () const {
		std::string fullname = std::string ();

		try {
			auto process = std::make_shared<HandleProcess> (this->pid ());
			fullname = process->path ();
		} catch (const std::bad_alloc &) {}

		return fullname;
	}

private:
	std::shared_ptr<PROCESSENTRY32> pEntry;

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
