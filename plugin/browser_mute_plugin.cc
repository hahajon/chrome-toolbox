#include "stdafx.h"
#include "browser_mute_plugin.h"
#include "log.h"
#include "script_object_factory.h"
#include "browser_mute_script_object.h"
#include <TlHelp32.h>

extern Log g_Log;
extern HMODULE g_hMod;

BrowserMutePlugin::BrowserMutePlugin(void) {
}

BrowserMutePlugin::~BrowserMutePlugin(void) {
}

void InjectIntoProcess(LPPROCESS_INFORMATION lpProcessInformation) {
  //SuspendThread(lpProcessInformation->hThread);
  LPVOID p = VirtualAllocEx(lpProcessInformation->hProcess, NULL, 
                            MAX_PATH*sizeof(TCHAR), 
                            MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

  char logs[256];
  if (p) {
    g_Log.WriteLog("Msg", "VirtualAllocEx");
    TCHAR dllpath[MAX_PATH];
    GetModuleFileName(g_hMod, dllpath, MAX_PATH);
    wchar_t* postfix = wcsrchr(dllpath, '\\');
    *(postfix+1) = 0;
    wcscat(dllpath, L"apihook.dll");
    SIZE_T n ;
    g_Log.WriteLog("Msg", "WriteProcessMemory");
    BOOL ret = WriteProcessMemory(lpProcessInformation->hProcess, p, dllpath, 
                                  MAX_PATH*sizeof(TCHAR), &n);
    if (!ret) {
      sprintf(logs, "WriteProcessMemory Failed,GetLastError=%ld", 
              GetLastError());
      g_Log.WriteLog("Error", logs);
    }
    g_Log.WriteLog("Msg", "CreateRemoteThread");
    HANDLE h = CreateRemoteThread(lpProcessInformation->hProcess, NULL, 0,
                                  (LPTHREAD_START_ROUTINE)LoadLibrary, p, 0, 
                                  NULL);
    if (h) {
      g_Log.WriteLog("Msg", "CreateRemoteThread Success");
      CloseHandle(h);
    } else {
      sprintf(logs, "CreateRemoteThread Failed,GetLastError=%ld", 
              GetLastError());
      g_Log.WriteLog("Error", logs);
    }
  } else {
    sprintf(logs, "VirtualAllocEx Failed,GetLastError=%ld", GetLastError());
    g_Log.WriteLog("Error", logs);
  }
  //ResumeThread(lpProcessInformation->hThread);
}

NPError BrowserMutePlugin::Init(NPP instance, uint16_t mode, int16_t argc,
                                char *argn[], char *argv[],
                                NPSavedData *saved) {
  script_object_ = NULL;
  g_Log.WriteLog("Msg", "BrowserMutePlugin Init");
  instance->pdata = this;
  HANDLE hprocess = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  PROCESSENTRY32 process = { sizeof(PROCESSENTRY32) };
  BOOL ret = Process32First(hprocess, &process);
  while (ret) {
    if (_wcsicmp(process.szExeFile, L"chrome.exe") == 0 
        && process.th32ProcessID != GetCurrentProcessId()) {
      HANDLE hmodule = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 
                                                process.th32ProcessID);
      MODULEENTRY32 mod = { sizeof(MODULEENTRY32) };
      BOOL flag = FALSE;
      if (Module32First(hmodule, &mod)) {
        if (_wcsicmp(mod.szModule, L"apihook.dll") == 0) {
          flag = TRUE;          
        }
        while(Module32Next(hmodule, &mod)) {
          if (_wcsicmp(mod.szModule, L"apihook.dll") == 0) {
            flag = TRUE;
            break;
          }
        }        
      }
      if (hmodule != INVALID_HANDLE_VALUE)
        CloseHandle(hmodule);
      if (flag) {
        ret = Process32Next(hprocess, &process);
        continue;
      }
      HANDLE hthread = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
      THREADENTRY32 thd = { sizeof(THREADENTRY32) };
      if (Thread32First(hthread, &thd)) {
        while (Thread32Next(hthread, &thd)) {
          if (thd.th32OwnerProcessID == process.th32ProcessID)
            break;
        }
      }
      if (hthread != INVALID_HANDLE_VALUE)
        CloseHandle(hthread);
      PROCESS_INFORMATION info;
      info.hProcess = OpenProcess(PROCESS_CREATE_THREAD | 
          PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE |
          PROCESS_VM_READ, FALSE, process.th32ProcessID);
      char logs[256];
      if (!info.hProcess) {
        sprintf(logs, "GetLastError=%ld", GetLastError());
        g_Log.WriteLog("Error", logs);
      }
      info.hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, 
                                thd.th32ThreadID);
      if (!info.hThread) {
        sprintf(logs, "GetLastError=%ld", GetLastError());
        g_Log.WriteLog("Error", logs);
      }
      InjectIntoProcess(&info);
      if (info.hProcess)
        CloseHandle(info.hProcess);
      if (info.hThread)
        CloseHandle(info.hThread);
    }
    ret = Process32Next(hprocess, &process);
  }
  if (hprocess != INVALID_HANDLE_VALUE)
    CloseHandle(hprocess);

  return PluginBase::Init(instance, mode, argc, argn, argv, saved);
}

NPError BrowserMutePlugin::UnInit(NPSavedData **save) {
  PluginBase::UnInit(save);
  script_object_ = NULL;
  g_Log.WriteLog("Msg", "BrowserMutePlugin UnInit");

  return NPERR_NO_ERROR;
}

NPError BrowserMutePlugin::GetValue(NPPVariable variable, void *value) {
  switch(variable) {
    case NPPVpluginScriptableNPObject:
      g_Log.WriteLog("GetValue", "GetValue");
      if (script_object_ == NULL) {
        script_object_ = ScriptObjectFactory::CreateObject(npp_,
            BrowserMuteScriptObject::Allocate);
      }
      if (script_object_ != NULL) {
        *(NPObject**)value = script_object_;
      }
      else
        return NPERR_OUT_OF_MEMORY_ERROR;
      break;
    default:
      return NPERR_GENERIC_ERROR;
  }

  return NPERR_NO_ERROR;
}