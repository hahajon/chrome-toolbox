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

void InjectIntoProcess(HANDLE hprocess) {
  LPVOID p = VirtualAllocEx(hprocess, NULL, 
                            MAX_PATH*sizeof(TCHAR), 
                            MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

  char logs[256];
  if (p) {
    g_Log.WriteLog("Msg", "VirtualAllocEx");
    TCHAR dllpath[MAX_PATH*2];
    GetModuleFileName(g_hMod, dllpath, MAX_PATH);
    wchar_t* postfix = wcsrchr(dllpath, '\\');
    if (postfix != NULL) {
      *(postfix+1) = 0;
      wcscat(dllpath, L"mutechrome.dll");
    } else {
      g_Log.WriteLog("Error", "postfix==NULL");
      return;
    }
    SIZE_T n ;
    g_Log.WriteLog("Msg", "WriteProcessMemory");
    BOOL ret = WriteProcessMemory(hprocess, p, dllpath, 
                                  MAX_PATH*sizeof(TCHAR), &n);
    if (!ret) {
      sprintf(logs, "WriteProcessMemory Failed,GetLastError=%ld", 
              GetLastError());
      g_Log.WriteLog("Error", logs);
    }
    g_Log.WriteLog("Msg", "CreateRemoteThread");
    HANDLE h = CreateRemoteThread(hprocess, NULL, 0,
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
}

void RenameApiHookDll() {
  TCHAR dllpath[MAX_PATH*2] = L"";
  TCHAR newdllpath[MAX_PATH*2] = L"";
  GetModuleFileName(g_hMod, dllpath, MAX_PATH);
  wcscpy(newdllpath, dllpath);
  wchar_t* postfix = wcsrchr(newdllpath, '\\');
  if (postfix != NULL) {
    *(postfix+1) = 0;
    wcscat(newdllpath, L"mutechrome.dll");
  }
  postfix = wcsrchr(dllpath, '\\');
  if (postfix != NULL) {
    *(postfix+1) = 0;
    wcscat(dllpath, L"apihook.dll");
  }
  CopyFile(dllpath, newdllpath, FALSE);
}

NPError BrowserMutePlugin::Init(NPP instance, uint16_t mode, int16_t argc,
                                char *argn[], char *argv[],
                                NPSavedData *saved) {
  script_object_ = NULL;
  g_Log.WriteLog("Msg", "BrowserMutePlugin Init");
  instance->pdata = this;
  RenameApiHookDll();

  HANDLE hprocess = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  PROCESSENTRY32 process = { sizeof(PROCESSENTRY32) };
  WCHAR chrome_exe_path[MAX_PATH];
  GetModuleFileName(GetModuleHandle(NULL), chrome_exe_path, MAX_PATH);
  BOOL find_same_chrome_version = FALSE;
  BOOL ret = Process32First(hprocess, &process);
  while (ret) {
    if (_wcsicmp(process.szExeFile, L"chrome.exe") == 0) {
      HANDLE hmodule = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 
                                                process.th32ProcessID);
      MODULEENTRY32 mod = { sizeof(MODULEENTRY32) };
      BOOL find_apihook_flag = FALSE;
      if (Module32First(hmodule, &mod)) {
        if (_wcsicmp(mod.szExePath, chrome_exe_path) != 0) {
          CloseHandle(hmodule);
          ret = Process32Next(hprocess, &process);
          continue;
        } else {
          find_same_chrome_version = TRUE;
        }
        while(Module32Next(hmodule, &mod)) {
          if (_wcsicmp(mod.szModule, L"mutechrome.dll") == 0 ||
              _wcsicmp(mod.szModule, L"apihook.dll") == 0) {
            find_apihook_flag = TRUE;
            break;
          }
        }
      }
      if (hmodule != INVALID_HANDLE_VALUE)
        CloseHandle(hmodule);
      if (find_apihook_flag) {
        if (find_same_chrome_version)
          break;

        ret = Process32Next(hprocess, &process);
        continue;
      }
      HANDLE process_handle = OpenProcess(PROCESS_CREATE_THREAD | 
          PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE |
          PROCESS_VM_READ, FALSE, process.th32ProcessID);
      char logs[256];
      if (!process_handle) {
        sprintf(logs, "OpenProcess GetLastError=%ld", GetLastError());
        g_Log.WriteLog("Error", logs);
      } else {
        sprintf(logs, "InjectIntoProcess, ProcessId=%ld", process.th32ProcessID);
        g_Log.WriteLog("Msg", logs);
      }
      InjectIntoProcess(process_handle);
      if (process_handle)
        CloseHandle(process_handle);
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
        NPN_RetainObject(script_object_);
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