#include "tasklist.h"  // NOLINT(build/include)

#define _WIN32_DCOM

#include <windows.h>
#include <comdef.h>
#include <Wbemidl.h>
#include "OAIdl.h"
#include <WbemCli.h>
#include <tchar.h>

#include <codecvt>
#include <string>
#include <iostream>

using pl::process;

#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "OleAut32.lib")

struct WMI {
  IWbemLocator *pLoc;
  IWbemServices *pSvc;
  IEnumWbemClassObject *pEnumerator;
};

struct wmient {
  IWbemClassObject *pClsObj;
  VARIANT *data;
};

// open wmi stream
static WMI * wmiopen(const char *query, LONG flags) {
  HRESULT hres;
  WMI *wmi = (WMI *)malloc(sizeof(struct WMI));

  if (wmi == NULL) {
    throw std::logic_error("Failed to initialize strict WMI");
  }

  memset(wmi, 0, sizeof(struct WMI));

  // Initialize COM.
  hres =  CoInitializeEx(0, COINIT_MULTITHREADED);

  if (FAILED(hres)) {
    free(wmi);
    throw std::logic_error("Failed to initialize COM library");
  }

  // Initialize
  hres =  CoInitializeSecurity(
    NULL,
    -1,  // COM negotiates service
    NULL,  // Authentication services
    NULL,  // Reserved
    RPC_C_AUTHN_LEVEL_DEFAULT,  // authentication
    RPC_C_IMP_LEVEL_IMPERSONATE,  // Impersonation
    NULL,  // Authentication info
    EOAC_NONE,  // Additional capabilities
    NULL);

  if (FAILED(hres)) {
    free(wmi);
    CoUninitialize();
    throw std::logic_error("Failed to initialize security.");
  }

  // Obtain the initial locator to Windows Management
  // on a particular host computer.
  hres = CoCreateInstance(
    CLSID_WbemLocator,
    0,
    CLSCTX_INPROC_SERVER,
    IID_IWbemLocator,
    (LPVOID *) &wmi->pLoc);

  if (FAILED(hres)) {
    free(wmi);
    CoUninitialize();
    throw std::logic_error("Failed to create IWbemLocator object.");
  }

  // Connect to the root\cimv2 namespace with the
  // current user and obtain pointer pSvc
  // to make IWbemServices calls.
  hres = wmi->pLoc->ConnectServer(
    _bstr_t(L"ROOT\\CIMV2"),  // WMI namespace
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
    CoUninitialize();
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
    CoUninitialize();
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
    CoUninitialize();
    throw std::logic_error("Query for processes failed.");
  }

  return wmi;
}

// close wmi stream
static void wmiclose(WMI *wmi) {
  wmi->pEnumerator->Release();
  wmi->pSvc->Release();
  wmi->pLoc->Release();

  free(wmi);
  CoUninitialize();
}

static wmient* wmiread(WMI *wmi) {
  if (!wmi) {
    return nullptr;
  }

  if (!wmi->pEnumerator) {
    return nullptr;
  }

  // define only on first call
  static wmient *entry = nullptr;

  // init struct
  if (entry == nullptr) {
    entry = (wmient *)malloc(sizeof(struct wmient));
    memset(entry, 0, sizeof(struct wmient));
  }

  ULONG ret = 0;
  HRESULT hres = wmi->pEnumerator->Next(
    WBEM_INFINITE,
    1,
    &entry->pClsObj,
    &ret);

  if (ret == 0) {
    if (entry->pClsObj) {
      entry->pClsObj->Release();
    }

    if (entry->data) {
      VariantClear(entry->data);
    }

    free(entry);
    return nullptr;
  }

  // clear prev call wmiprop()
  // `data` can be equals 0, nullptr or valid data
  if (entry->data) {
    VariantClear(entry->data);
    entry->data = nullptr;
  }

  return entry;
}

static std::string ws2s(const std::wstring& wstr) {
  typedef std::codecvt_utf8<wchar_t> convert_typeX;
  std::wstring_convert<convert_typeX, wchar_t> converterX;

  return converterX.to_bytes(wstr);
}

template<typename T>
inline T getter(const VARIANT *) {
  throw new std::logic_error("Specialization not found for this type");
}

template<>
inline std::string getter(const VARIANT *prop) {
  return ws2s(prop->bstrVal);
}

template<>
inline BSTR getter(const VARIANT *prop) {
  return prop->bstrVal;
}

template<>
inline uint32_t getter(const VARIANT *prop) {
  return prop->ulVal;
}

template <typename T>
static T wmiprop(struct wmient *wmie, const wchar_t *prop, T defaultValue) {
  if (!wmie->pClsObj) {
    return defaultValue;
  }

  if (!wmie->data) {
    wmie->data = (VARIANT *)malloc(sizeof(VARIANT));
  }

  HRESULT hres = wmie->pClsObj->Get(prop, 0, wmie->data, NULL, NULL);

  if (FAILED(hres) || wmie->data->vt == VT_NULL) {
    VariantClear(wmie->data);
    free(wmie->data);
    wmie->data = nullptr;

    return defaultValue;
  }

  T val = getter<T>(wmie->data);

  VariantClear(wmie->data);
  free(wmie->data);
  wmie->data = nullptr;

  return val;
}

template <typename T>
static T wmicall(struct WMI *wmi,
                 struct wmient *wmientry,
                 const wchar_t *methodName,
                 const wchar_t *fieldName,
                 T defaultValue) {
  IWbemClassObject *pOutParams = NULL;
  BSTR handlePath = wmiprop<BSTR>(wmientry, L"__Path", L"");

  if (handlePath == L"") {
    return defaultValue;
  }

  HRESULT hres = wmi->pSvc->ExecMethod(
      handlePath,
      bstr_t(methodName),
      0,
      NULL,
      NULL,
      &pOutParams,
      NULL);

  if (FAILED(hres)) {
    return defaultValue;
  }

  VARIANT *varReturnValue = (VARIANT *)malloc(sizeof(VARIANT));
  hres = pOutParams->Get(_bstr_t(fieldName), 0, varReturnValue, NULL, NULL);

  if (FAILED(hres) || varReturnValue->vt == VT_NULL) {
    pOutParams->Release();
    VariantClear(varReturnValue);
    free(varReturnValue);
    return defaultValue;
  }

  T val = getter<T>(varReturnValue);

  pOutParams->Release();
  VariantClear(varReturnValue);
  free(varReturnValue);

  return val;
}

namespace pl {

  list_t list(const struct process_fields &requested_fields) {
    list_t proclist;

    LONG flagsOpen = WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY;
    struct WMI *wmi = wmiopen("SELECT * FROM Win32_Process", flagsOpen);
    struct wmient *entry = nullptr;

    while ((entry = wmiread(wmi)) != nullptr) {
      struct process proc;

      if (requested_fields.pid) {
        proc.pid = wmiprop<uint32_t>(entry, L"ProcessId", 0);
      }

      if (requested_fields.ppid) {
        proc.ppid = wmiprop<uint32_t>(entry, L"ParentProcessId", 0);
      }

      if (requested_fields.name) {
        proc.name = wmiprop<std::string>(entry, L"Name", "");
      }

      if (requested_fields.path) {
        proc.path = wmiprop<std::string>(entry, L"ExecutablePath", "");
      }

      if (requested_fields.cmdline) {
        proc.cmdline = wmiprop<std::string>(entry, L"CommandLine", "");
      }

      if (requested_fields.threads) {
        proc.threads = wmiprop<uint32_t>(entry, L"ThreadCount", 0);
      }

      if (requested_fields.priority) {
        proc.priority = wmiprop<uint32_t>(entry, L"Priority", 0);
      }

      if (requested_fields.owner) {
        proc.owner = wmicall<std::string>(wmi, entry, L"GetOwner", L"User", "");
      }

      proclist.push_back(proc);
    }

    wmiclose(wmi);
    return proclist;
  }
}  // namespace pl
