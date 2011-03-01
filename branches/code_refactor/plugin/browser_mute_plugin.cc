#include "stdafx.h"
#include "browser_mute_plugin.h"
#include "log.h"
#include "script_object_factory.h"
#include "browser_mute_script_object.h"
#include <TlHelp32.h>
#include <Psapi.h>
#include <Mmdeviceapi.h>
#include <Audiopolicy.h>
#include <map>

extern Log g_Log;
extern HMODULE g_hMod;

#define CHECK_RESULT(hr)     if (FAILED(hr)) {\
  Sleep(100);\
  continue;\
}

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
    TCHAR dllpath[MAX_PATH];
    GetModuleFileName(g_hMod, dllpath, MAX_PATH);
    TCHAR* postfix = _tcsrchr(dllpath, '\\');
    if (postfix != NULL) {
      *(postfix+1) = 0;
      _tcscat(dllpath, _T("mutechrome.dll"));
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
  TCHAR dllpath[MAX_PATH] = _T("");
  TCHAR newdllpath[MAX_PATH] = _T("");
  GetModuleFileName(g_hMod, dllpath, MAX_PATH);
  _tcscpy(newdllpath, dllpath);
  TCHAR* postfix = _tcsrchr(newdllpath, '\\');
  if (postfix != NULL) {
    *(postfix+1) = 0;
    _tcscat(newdllpath, _T("mutechrome.dll"));
  }
  postfix = _tcsrchr(dllpath, '\\');
  if (postfix != NULL) {
    *(postfix+1) = 0;
    _tcscat(dllpath, _T("apihook.dll"));
  }
  FILE* src_file = _tfopen(dllpath, _T("rb"));
  FILE* dest_file = _tfopen(newdllpath, _T("wb"));
  if (src_file && dest_file) {
    byte buffer[1024];
    int len = fread(buffer, 1, 1024, src_file);
    while (len > 0) {
      fwrite(buffer, 1, len, dest_file);
      len = fread(buffer, 1, 1024, src_file);
    }
    fclose(src_file);
    fclose(dest_file);
  }
}

bool GetProdcutVersion(LPCTSTR filename, DWORD* mostversion, 
                       DWORD* leastversion) {
  DWORD infosize_handle = 0;
  DWORD version_info_size = ::GetFileVersionInfoSize(filename, 
                                                     &infosize_handle);
  if(version_info_size) {
    unsigned int info_size = 0;
    VS_FIXEDFILEINFO* file_info;
    
    BYTE* version_buffer = new BYTE[version_info_size];
    GetFileVersionInfo(filename, infosize_handle, version_info_size,
                       version_buffer);
    if (VerQueryValue(version_buffer, _T("\\"), (void**)&file_info, &info_size)) {
      *mostversion = file_info->dwProductVersionMS;
      *leastversion = file_info->dwProductVersionLS;
      delete[] version_buffer;
      return true;
    } else {
      char logs[256];
      sprintf(logs, "GetLastError1=%ld", GetLastError());
      g_Log.WriteLog("error", logs);
    }
    delete[] version_buffer;
  } else {
    char logs[512];
    sprintf(logs, "GetLastError2=%ld,%S", GetLastError(), filename);
    g_Log.WriteLog("error", logs);
  }
  return false;
}

DWORD BrowserMutePlugin::Mute_Thread(void* param) {
  BrowserMutePlugin* plugin = (BrowserMutePlugin*)param;
  g_Log.WriteLog("msg", "Start Mute_Thread");
  CoInitialize(NULL);

  HRESULT hr = E_FAIL;
  IMMDeviceEnumerator* device_enumerator;
  IMMDevice* defaultdevice;
  IAudioSessionManager2* audio_session_mamanger2;
  IAudioSessionEnumerator* audio_session_enumerator;
  ISimpleAudioVolume* simple_audio_volume;
  BOOL mute_flag = FALSE;
  map<DWORD,DWORD> chrome_process_map;
  DWORD parent_pid = 0;
  TCHAR exe_name[MAX_PATH];
  TCHAR chrome_exe_path[MAX_PATH];
  GetModuleBaseName(GetCurrentProcess(), GetModuleHandle(NULL), exe_name, MAX_PATH);
  GetModuleFileName(GetModuleHandle(NULL), chrome_exe_path, MAX_PATH);

  //GetParentProcessID
  HANDLE hprocess = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  PROCESSENTRY32 process = { sizeof(PROCESSENTRY32) };
  BOOL find_same_chrome_version = FALSE;
  BOOL ret = Process32First(hprocess, &process);
  while (ret) {
    if (process.th32ProcessID == GetCurrentProcessId()) {
      parent_pid = process.th32ParentProcessID;
      break;
    }
    ret = Process32Next(hprocess, &process);
  }
  if (hprocess != INVALID_HANDLE_VALUE)
    CloseHandle(hprocess);

  hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL,
    CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&device_enumerator);
  if (FAILED(hr))
    return hr;

  while(WaitForSingleObject(plugin->stop_event_, 1000) == WAIT_TIMEOUT) {
    if (plugin->script_object_ == NULL)
      continue;
    mute_flag = ((BrowserMuteScriptObject*)plugin->script_object_)->
          get_mute_flag();

    chrome_process_map.clear();

    HANDLE hprocess = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 process = { sizeof(PROCESSENTRY32) };
    BOOL find_same_chrome_version = FALSE;
    BOOL ret = Process32First(hprocess, &process);
    while (ret) {
      if (process.th32ParentProcessID == parent_pid || 
          process.th32ProcessID == parent_pid) {
        chrome_process_map.insert(make_pair(process.th32ProcessID, 
            process.th32ProcessID));
      }
      ret = Process32Next(hprocess, &process);
    }
    if (hprocess != INVALID_HANDLE_VALUE)
      CloseHandle(hprocess);

    CHECK_RESULT(device_enumerator->GetDefaultAudioEndpoint(eRender, eConsole, 
      &defaultdevice));

    CHECK_RESULT(defaultdevice->Activate(__uuidof(IAudioSessionManager2), 
      CLSCTX_ALL, NULL, (void**)&audio_session_mamanger2));

    CHECK_RESULT(audio_session_mamanger2->
      GetSessionEnumerator(&audio_session_enumerator));

    int count;
    CHECK_RESULT(audio_session_enumerator->GetCount(&count));

    for (int i = 0; i < count; i++) {
      IAudioSessionControl* audio_session_control;
      IAudioSessionControl2* audio_session_control2;
      CHECK_RESULT(audio_session_enumerator->
        GetSession(i, &audio_session_control));
      CHECK_RESULT(audio_session_control->
        QueryInterface(__uuidof(IAudioSessionControl2), 
        (void**)&audio_session_control2));
      DWORD processid;
      CHECK_RESULT(audio_session_control2->GetProcessId(&processid));
      if (chrome_process_map.find(processid) != chrome_process_map.end()) {
        CHECK_RESULT(audio_session_control2->
          QueryInterface(__uuidof(ISimpleAudioVolume), 
          (void**) &simple_audio_volume));
        CHECK_RESULT(simple_audio_volume->SetMute(mute_flag, NULL));
        simple_audio_volume->Release();
      }
      audio_session_control->Release();
      audio_session_control2->Release();
    }

    audio_session_mamanger2->Release();
    audio_session_enumerator->Release();
    defaultdevice->Release();
  }
  CoUninitialize();
  g_Log.WriteLog("msg", "Stop Mute_Thread");

  return 0;
}

void BrowserMutePlugin::ScanAndInject() {
  TCHAR chrome_exe_path[MAX_PATH];
  GetModuleFileName(GetModuleHandle(NULL), chrome_exe_path, MAX_PATH);

  HANDLE hprocess = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  PROCESSENTRY32 process = { sizeof(PROCESSENTRY32) };
  TCHAR exe_name[MAX_PATH];
  GetModuleBaseName(GetCurrentProcess(), GetModuleHandle(NULL), exe_name, MAX_PATH);
  BOOL find_same_chrome_version = FALSE;
  BOOL ret = Process32First(hprocess, &process);
  while (ret) {
    if (_tcsicmp(process.szExeFile, exe_name) == 0) {
      HANDLE hmodule = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 
          process.th32ProcessID);
      MODULEENTRY32 mod = { sizeof(MODULEENTRY32) };
      BOOL find_apihook_flag = FALSE;
      if (Module32First(hmodule, &mod)) {
        if (_tcsicmp(mod.szExePath, chrome_exe_path) != 0) {
          CloseHandle(hmodule);
          ret = Process32Next(hprocess, &process);
          continue;
        } else {
          find_same_chrome_version = TRUE;
        }
        while(Module32Next(hmodule, &mod)) {
          if (_tcsicmp(mod.szModule, _T("mutechrome.dll")) == 0 ||
            _tcsicmp(mod.szModule, _T("apihook.dll")) == 0) {
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
}

NPError BrowserMutePlugin::Init(NPP instance, uint16_t mode, int16_t argc,
                                char *argn[], char *argv[],
                                NPSavedData *saved) {
  script_object_ = NULL;
  g_Log.WriteLog("Msg", "BrowserMutePlugin Init");
  instance->pdata = this;
  use_apihook_flag_ = TRUE;

  OSVERSIONINFO versionInfo = { 0 };
  versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);
  GetVersionEx(&versionInfo);
  if (versionInfo.dwMajorVersion > 6 || (versionInfo.dwMajorVersion == 6 && 
      versionInfo.dwMinorVersion > 0)) {
    use_apihook_flag_ = FALSE;
    stop_event_ = CreateEvent(NULL, FALSE, FALSE, NULL);
    mute_thread_handle_ = CreateThread(NULL, 0, Mute_Thread, this, 0, 0);
  } else {
    RenameApiHookDll();
    ScanAndInject();
  }
  return PluginBase::Init(instance, mode, argc, argn, argv, saved);
}

NPError BrowserMutePlugin::UnInit(NPSavedData **save) {
  PluginBase::UnInit(save);
  script_object_ = NULL;
  g_Log.WriteLog("Msg", "BrowserMutePlugin UnInit");
  if (!use_apihook_flag_) {
    SetEvent(stop_event_);
    if (WaitForSingleObject(mute_thread_handle_, 10) == WAIT_TIMEOUT) {
      TerminateThread(mute_thread_handle_, 0);
      mute_thread_handle_ = INVALID_HANDLE_VALUE;
    }
  }

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