#include "stdafx.h"
#include "browser_mute_script_object.h"
#include "Log.h"
#include <TlHelp32.h>

extern Log g_Log;
extern HMODULE g_hMod;

BrowserMuteScriptObject::BrowserMuteScriptObject(void) {
  api_hook_module_ = NULL;
}

BrowserMuteScriptObject::~BrowserMuteScriptObject(void) {
}

NPObject* BrowserMuteScriptObject::Allocate(NPP npp, NPClass *aClass) {
  BrowserMuteScriptObject* pRet = new BrowserMuteScriptObject;
  char logs[256];
  sprintf(logs, "BrowserMuteScriptObject this=%ld", pRet);
  g_Log.WriteLog("Allocate", logs);
  if (pRet != NULL) {
    pRet->SetPlugin((PluginBase*)npp->pdata);
    Function_Item item;
    strcpy_s(item.function_name, "MuteBrowser");
    item.function_pointer = ON_INVOKEHELPER(&BrowserMuteScriptObject::
        MuteBrowser);
    pRet->AddFunction(item);
  }
  return pRet;
}

void BrowserMuteScriptObject::Deallocate() {
  char logs[256];
  sprintf_s(logs, "BrowserMuteScriptObject this=%ld", this);
  g_Log.WriteLog("Deallocate", logs);
  delete this;
}

void BrowserMuteScriptObject::Invalidate() {

}

bool BrowserMuteScriptObject::Construct(const NPVariant *args,
                                        uint32_t argCount,
                                        NPVariant *result) {
  return true;
}

bool BrowserMuteScriptObject::MuteBrowser(const NPVariant *args,
                                          uint32_t argCount,
                                          NPVariant *result) {
  if (argCount != 1 || !NPVARIANT_IS_BOOLEAN(args[0]))
    return false;
  
  g_Log.WriteLog("Msg", "SetBrowserMute");

  if (!api_hook_module_) {
    HANDLE hmodule = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 
                                              GetCurrentProcessId());
    if (hmodule == INVALID_HANDLE_VALUE) {
      g_Log.WriteLog("Error", "TH32CS_SNAPMODULE");
    }
    MODULEENTRY32 mod = { sizeof(MODULEENTRY32) };
    BOOL flag = FALSE;
    if (Module32First(hmodule, &mod)) {
      while(Module32Next(hmodule, &mod)) {
        if (_wcsicmp(mod.szModule, L"mutechrome.dll") == 0 ||
            _wcsicmp(mod.szModule, L"apihook.dll") == 0) {
          flag = TRUE;
          api_hook_module_ = mod.hModule;
          break;
        }
      }        
    }
    if (hmodule != INVALID_HANDLE_VALUE)
      CloseHandle(hmodule);

    if (!flag) {
      TCHAR dllpath[MAX_PATH*2];
      GetModuleFileName(g_hMod, dllpath, MAX_PATH);
      wchar_t* postfix = wcsrchr(dllpath, '\\');
      if (postfix != NULL) {
        *(postfix+1) = 0;
        wcscat(dllpath, L"mutechrome.dll");
        g_Log.WriteLog("Msg", "Before LoadLibrary");
        api_hook_module_ = LoadLibrary(dllpath);
        g_Log.WriteLog("Msg", "After LoadLibrary");
      }
    }
    set_browser_mute_ = (Pfn_SetBrowserMute)GetProcAddress(api_hook_module_, 
                                                            "SetBrowserMute");
    g_Log.WriteLog("Msg", "After GetProcAddress");
    if (set_browser_mute_) {
      set_browser_mute_(NPVARIANT_TO_BOOLEAN(args[0]));
      g_Log.WriteLog("Msg", "SetBrowserMute1");
    }
  } else {
    if (set_browser_mute_) {
      set_browser_mute_(NPVARIANT_TO_BOOLEAN(args[0]));
      g_Log.WriteLog("Msg", "SetBrowserMute2");
    }
  }

  return true;
}
