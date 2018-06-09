/**
 * Copyright (c) 2014 Dmitry Tsvettsikh <https://github.com/reklatsmasters>
 *
 * MIT License <https://github.com/reklatsmasters/node-process-list/blob/master/LICENSE>
 */

#include "tasklist.h"  // NOLINT(build/include)

#define _WIN32_DCOM

#include <windows.h>
#include <comdef.h>
#include <Wbemidl.h>
#include "OAIdl.h"
#include <WbemCli.h>
#include <tchar.h>
#include <atlbase.h>

#include <codecvt>
#include <string>
#include <iostream>
#include <ctime>

using pl::process;

#pragma comment(lib, "wbemuuid.lib")

#define EPOCH_SINCE_UNIX_NANO 116444736000000000
#define SEC_TO_MS 10000

#define KBYTE 1024

/**
 * convert 100-nanosecond units to milliseconds
 * @link https://msdn.microsoft.com/en-us/library/windows/desktop/dd979590(v=vs.85).aspx
 */
#define TO_MS(x) ((uint64_t)(x / SEC_TO_MS))

static std::string ws2s(const std::wstring& wstr) {
  typedef std::codecvt_utf8<wchar_t> convert_typeX;
  std::wstring_convert<convert_typeX, wchar_t> converterX;

  return converterX.to_bytes(wstr);
}

// The SafeRelease Pattern
template <class T>
void SafeRelease(T **ppT) {
  if (*ppT) {
    (*ppT)->Release();
    *ppT = NULL;
  }
}

struct WMI {
  IWbemLocator *pLoc;
  IWbemServices *pSvc;
  IEnumWbemClassObject *pEnumerator;
};

struct WMIEntry {
  WMIEntry() : pClsObj(NULL) {}

  ~WMIEntry() {
    SafeRelease(&pClsObj);
  }

  IWbemClassObject *pClsObj;
};

class CoInitializeHelper {
 public:
  CoInitializeHelper() :
    m_hres(CoInitializeEx(NULL, COINIT_MULTITHREADED))
    { }

  ~CoInitializeHelper() {
    if (SUCCEEDED(m_hres)) {
      CoUninitialize();
    }
  }

  operator HRESULT() const {
    return m_hres;
  }

 private:
  HRESULT m_hres;
};

/**
 * open wmi stream
 */
static WMI * wmiopen(const char *query, LONG flags) {
  HRESULT hres;
  WMI *wmi = reinterpret_cast<WMI *>(malloc(sizeof(struct WMI)));

  if (wmi == NULL) {
    throw std::logic_error("Failed to initialize struct WMI");
  }

  memset(wmi, 0, sizeof(struct WMI));

  // Obtain the initial locator to Windows Management
  // on a particular host computer.
  hres = CoCreateInstance(
    CLSID_WbemLocator,
    0,
    CLSCTX_INPROC_SERVER,
    IID_IWbemLocator,
    reinterpret_cast<LPVOID *>(&wmi->pLoc));

  if (FAILED(hres)) {
    free(wmi);
    throw std::logic_error("Failed to create IWbemLocator object.");
  }

  // Connect to the root\cimv2 namespace with the
  // current user and obtain pointer pSvc
  // to make IWbemServices calls.
  hres = wmi->pLoc->ConnectServer(
    bstr_t(L"ROOT\\CIMV2"),   // WMI namespace
    NULL,                     // User name
    NULL,                     // User password
    0,                        // Locale
    NULL,                     // Security flags
    0,                        // Authority
    0,                        // Context object
    &wmi->pSvc);              // IWbemServices proxy

  if (FAILED(hres)) {
    wmi->pLoc->Release();
    free(wmi);
    throw std::logic_error("Could not connect to root\\cimv2.");
  }

  // Set the IWbemServices proxy so that impersonation
  // of the user (client) occurs.
  hres = CoSetProxyBlanket(
    wmi->pSvc,                    // the proxy to set
    RPC_C_AUTHN_WINNT,            // authentication service
    RPC_C_AUTHZ_NONE,             // authorization service
    NULL,                         // Server principal name
    RPC_C_AUTHN_LEVEL_CALL,       // authentication level
    RPC_C_IMP_LEVEL_IMPERSONATE,  // impersonation level
    NULL,                         // client identity
    EOAC_NONE);                   // proxy capabilities

  if (FAILED(hres)) {
    wmi->pSvc->Release();
    wmi->pLoc->Release();
    free(wmi);
    throw std::logic_error("Could not set proxy blanket.");
  }

  // Use the IWbemServices pointer to make requests of WMI.
  hres = wmi->pSvc->ExecQuery(
    bstr_t("WQL"),
    bstr_t(query),
    flags,
    NULL,
    &wmi->pEnumerator);

  if (FAILED(hres)) {
    wmi->pSvc->Release();
    wmi->pLoc->Release();
    free(wmi);
    throw std::logic_error("Query for processes failed.");
  }

  return wmi;
}

/**
 * close wmi stream
 */
static void wmiclose(WMI *wmi) {
  wmi->pEnumerator->Release();
  wmi->pSvc->Release();
  wmi->pLoc->Release();

  free(wmi);
}

/**
 * read wmi entry
 */
static int wmiread(WMI *wmi, WMIEntry *entry) {
  if (!wmi) {
    return -1;
  }

  if (!wmi->pEnumerator) {
    return -1;
  }

  ULONG ret = 0;
  HRESULT hres = wmi->pEnumerator->Next(
    WBEM_INFINITE,
    1,
    &entry->pClsObj,
    &ret);

  if (ret == 0) {
    return -1;
  }

  return 0;
}

template<typename T>
inline T getter(const VARIANT *) {
  throw new std::logic_error("Specialization not found for this type");
}

/**
 * read `variant` as `std::string`
 */
template<>
inline std::string getter(const VARIANT *prop) {
  return ws2s(prop->bstrVal);
}

/**
 * read `variant` as `BSTR`
 */
template<>
inline BSTR getter(const VARIANT *prop) {
  return prop->bstrVal;
}

/**
 * read `variant` as `uint32_t`
 */
template<>
inline uint32_t getter(const VARIANT *prop) {
  return prop->ulVal;
}

/**
 * read property from wmi entry
 */
template <typename T>
static T wmiprop(WMIEntry *entry, const wchar_t *prop, T defaultValue) {
  if (!entry->pClsObj) {
    return defaultValue;
  }

  VARIANT vtProp;
  HRESULT hres = entry->pClsObj->Get(prop, 0, &vtProp, NULL, NULL);

  if (FAILED(hres)) {
    return defaultValue;
  }

  T res = (vtProp.vt != VT_NULL) ? getter<T>(&vtProp) : defaultValue;
  VariantClear(&vtProp);
  return res;
}

/**
 * call wmi method
 */
template <typename T>
static T wmicall(WMI *wmi, WMIEntry *entry,
                 const wchar_t *methodName,
                 const wchar_t *fieldName,
                 T defaultValue) {
  WMIEntry outParams;
  BSTR handlePath = wmiprop<BSTR>(entry, L"__Path", L"");

  if (handlePath == L"") {
    return defaultValue;
  }

  HRESULT hres = wmi->pSvc->ExecMethod(
      handlePath,
      bstr_t(methodName),
      0,
      NULL,
      NULL,
      &outParams.pClsObj,
      NULL);

  if (FAILED(hres)) {
    return defaultValue;
  }

  VARIANT vtProp;
  hres = outParams.pClsObj->Get(
    _bstr_t(fieldName),
    0,
    &vtProp,
    NULL,
    NULL);

  if (FAILED(hres)) {
    return defaultValue;
  }

  T res = (vtProp.vt != VT_NULL) ? getter<T>(&vtProp) : defaultValue;
  VariantClear(&vtProp);
  return res;
}

/**
 * special reader for `variant` `datetime` type
 */
static uint64_t wmitime(WMIEntry *entry, const wchar_t *prop) {
  if (!entry->pClsObj) {
    return 0;
  }

  VARIANT vtProp;
  HRESULT hres = entry->pClsObj->Get(prop, 0, &vtProp, NULL, NULL);

  if (FAILED(hres)) {
    return 0;
  }

  if (vtProp.vt == VT_NULL) {
    VariantClear(&vtProp);
    return 0;
  }

  BSTR datetime = getter<BSTR>(&vtProp);
  VariantClear(&vtProp);

  CComPtr<ISWbemDateTime> pSWbemDateTime;

  // create SWbemDateTime instance
  // @link https://msdn.microsoft.com/en-us/library/aa393687(v=vs.85).aspx
  hres = CoCreateInstance(CLSID_SWbemDateTime, NULL, CLSCTX_INPROC_SERVER,
    IID_PPV_ARGS(&pSWbemDateTime));

  if (FAILED(hres)) {
    return 0;
  }

  // set CIM_DATETIME
  // @link https://msdn.microsoft.com/en-us/library/aa387237(v=vs.85).aspx
  hres = pSWbemDateTime->put_Value(datetime);

  if (FAILED(hres)) {
    return 0;
  }

  // get as FILETIME
  // @link https://msdn.microsoft.com/en-us/library/ms724284(v=vs.85).aspx
  BSTR bstrFileTime;
  hres = pSWbemDateTime->GetFileTime(VARIANT_FALSE, &bstrFileTime);

  if (FAILED(hres)) {
    return 0;
  }

  // convert FILETIME to Unix Time
  ULARGE_INTEGER ulargeFileTime, unixEpoch;
  ulargeFileTime.QuadPart = _wtoi64(bstrFileTime);

  unixEpoch.QuadPart = EPOCH_SINCE_UNIX_NANO;
  ulargeFileTime.QuadPart -= unixEpoch.QuadPart;

  SysFreeString(bstrFileTime);
  SysFreeString(datetime);

  return ulargeFileTime.QuadPart / SEC_TO_MS;
}

namespace pl {

  /**
   * main function
   */
  list_t list(const struct process_fields &requested_fields) {
    // Initialize COM.
    CoInitializeHelper co;

    if (FAILED(co)) {
      throw std::logic_error("Failed to initialize COM library");
    }

    list_t proclist;
    LONG flagsOpen = WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY;

    struct WMI *wmi = wmiopen("SELECT * FROM Win32_Process", flagsOpen);

    while (true) {
      struct WMIEntry entry;
      struct process proc;

      int ret = wmiread(wmi, &entry);

      if (ret < 0) {
        break;
      }

      if (requested_fields.pid) {
        proc.pid = wmiprop<uint32_t>(&entry, L"ProcessId", 0);
      }

      if (requested_fields.ppid) {
        proc.ppid = wmiprop<uint32_t>(&entry, L"ParentProcessId", 0);
      }

      if (requested_fields.name) {
        proc.name = wmiprop<std::string>(&entry, L"Name", "");
      }

      if (requested_fields.path) {
        proc.path = wmiprop<std::string>(&entry, L"ExecutablePath", "");
      }

      if (requested_fields.cmdline) {
        proc.cmdline = wmiprop<std::string>(&entry, L"CommandLine", "");
      }

      if (requested_fields.threads) {
        proc.threads = wmiprop<uint32_t>(&entry, L"ThreadCount", 0);
      }

      if (requested_fields.priority) {
        proc.priority = wmiprop<uint32_t>(&entry, L"Priority", 0);
      }

      if (requested_fields.owner) {
        proc.owner = wmicall<std::string>(
          wmi,
          &entry,
          L"GetOwner",
          L"User",
          std::string());
      }

      if (requested_fields.starttime) {
        proc.starttime = wmitime(&entry, L"CreationDate");
      }

      if (requested_fields.vmem) {
        proc.vmem = wmiprop<uint32_t>(&entry, L"PageFileUsage", 0) * KBYTE;
      }

      if (requested_fields.pmem) {
        proc.pmem = _wtoi64(wmiprop<BSTR>(&entry, L"WorkingSetSize", 0));
      }

      if (requested_fields.cpu) {
        auto utime = _wtoi64(wmiprop<BSTR>(&entry, L"UserModeTime", 0));
        auto stime = _wtoi64(wmiprop<BSTR>(&entry, L"KernelModeTime", 0));

        utime = TO_MS(utime);
        stime = TO_MS(stime);

        uint64_t starttime = requested_fields.starttime ?
          proc.starttime :
          wmitime(&entry, L"CreationDate");
        uint64_t now = std::time(nullptr) * 1000;

        double cpu = static_cast<double>(utime + stime) / (now - starttime);
        proc.cpu = NORMAL(cpu * 100, 0.0f, 100.0f);
      }

      if (requested_fields.utime) {
        proc.utime = TO_MS(_wtoi64(wmiprop<BSTR>(&entry, L"UserModeTime", 0)));
      }

      if (requested_fields.stime) {
        proc.stime = _wtoi64(wmiprop<BSTR>(&entry, L"KernelModeTime", 0));
        proc.stime = TO_MS(proc.stime);
      }

      proclist.push_back(proc);
    }

    wmiclose(wmi);
    return proclist;
  }
}  // namespace pl
