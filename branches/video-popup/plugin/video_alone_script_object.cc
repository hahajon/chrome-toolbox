#include "stdafx.h"
#include "video_alone_script_object.h"
#include "log.h"
#include "video_window_manager.h"

#include <dwmapi.h>

HHOOK hCallWndProcHook = NULL;
extern Log g_Log;
extern HMODULE g_hMod;
VideoWindowManager g_VideoWinMan;

int g_Chrome_MajorVersion = 0;
BOOL g_Enable_DWM = FALSE;

WNDPROC g_OldProc = NULL;

int ReadChromeMajorVersion() {
  TCHAR current_path[MAX_PATH];
  GetCurrentDirectory(MAX_PATH, current_path);
  TCHAR file_name[MAX_PATH];
  wsprintf(file_name, L"%s\\convenience.ini", current_path);
  return GetPrivateProfileInt(L"CFG", L"MAJORVERSION", 0, file_name);
}

void WriteChromeMajorVersion() {
  TCHAR current_path[MAX_PATH];
  GetCurrentDirectory(MAX_PATH, current_path);
  TCHAR file_name[MAX_PATH];
  wsprintf(file_name, L"%s\\convenience.ini", current_path);
  TCHAR value[32];
  wsprintf(value, L"%ld", g_Chrome_MajorVersion);
  WritePrivateProfileString(L"CFG", L"MAJORVERSION", value, file_name);
}

LRESULT CALLBACK NewWndProc(HWND hWnd, UINT Msg, WPARAM wParam, 
                            LPARAM lParam) {
  LRESULT ret = 0;
  if (Msg == WM_PAINT) {
    ret = CallWindowProc(g_OldProc, hWnd, Msg, wParam, lParam);
    g_VideoWinMan.WndProc(hWnd, Msg, wParam, lParam);
  } else {
    ret = g_VideoWinMan.WndProc(hWnd, Msg, wParam, lParam);
    ret = CallWindowProc(g_OldProc, hWnd, Msg, wParam, lParam);
    if (Msg == WM_NCHITTEST && ret == HTCLIENT)
      ret = HTCAPTION;
  }
  return ret;
}

LRESULT CALLBACK CallWndProc(int nCode, WPARAM wParam, LPARAM lParam) {
  CWPSTRUCT * pRet = (CWPSTRUCT *)lParam;

  if (pRet->message == WM_CHROMEHWND) {
    if (g_Chrome_MajorVersion == 0)
      g_Chrome_MajorVersion = ReadChromeMajorVersion();

    HMODULE h = LoadLibrary(L"dwmapi.dll");
    if (h) {
      DwmIsCompositionEnabled(&g_Enable_DWM);
      FreeLibrary(h);
    }

    g_VideoWinMan.AddNewVideoWindow((HWND)pRet->wParam, (HWND)pRet->lParam);
    g_OldProc = SubclassWindow((HWND)pRet->wParam, NewWndProc);


    if (g_Chrome_MajorVersion >= MINIMUM_VERSION_SUPPORT_POPUP && 
        g_Enable_DWM) {
      DwmSetWindowAttribute((HWND)pRet->wParam, DWMWA_ALLOW_NCPAINT, 
                            &g_Enable_DWM, sizeof(g_Enable_DWM));
      SendMessage((HWND)pRet->wParam, WM_NCPAINT, 0, 0);
    }

    if (g_Chrome_MajorVersion < MINIMUM_VERSION_SUPPORT_POPUP) {
      SetTimer((HWND)pRet->wParam, EVENTID_FRESH, 100, NULL);
    }
  }
  
  return CallNextHookEx(hCallWndProcHook, nCode, wParam, lParam);
}

VideoAloneScriptObject::VideoAloneScriptObject(void) {
}

VideoAloneScriptObject::~VideoAloneScriptObject(void) {
  
}

NPObject* VideoAloneScriptObject::Allocate(NPP npp, NPClass *aClass) {
  VideoAloneScriptObject* pRet = new VideoAloneScriptObject;
  if (pRet != NULL) {
    pRet->SetPlugin((PluginBase*)npp->pdata);
    Function_Item item;
    strcpy(item.function_name, "ShowVideoAlone");
    item.function_pointer = ON_INVOKEHELPER(&VideoAloneScriptObject::ShowVideoAlone);
    pRet->AddFunction(item);
  }
  return pRet;
}

void VideoAloneScriptObject::Deallocate() {
  delete this;
}

void VideoAloneScriptObject::Invalidate() {

}

bool VideoAloneScriptObject::ShowVideoAlone(const NPVariant *args,
                                            uint32_t argCount,
                                            NPVariant *result) {
  char szLog[512];

  if (argCount < 5)
    return false;

  for(int i = 0; i < 2; i++) {
    if (!NPVARIANT_IS_STRING(args[i])) {
      return false;
    }
  }

  for (int i = 2; i < 5; i++) {
    if (!NPVARIANT_IS_INT32(args[i]) && !NPVARIANT_IS_DOUBLE(args[i])) {
      return false;
    }
  }

  if (g_Chrome_MajorVersion == 0) {
    string user_agent = NPN_UserAgent(plugin_->get_npp());
    int index = user_agent.find("Chrome/");
    if (index != string::npos) {
      int last_index = user_agent.find('.', index);
      if (last_index != string::npos) {
        string majorversion = user_agent.substr(index+7, last_index-index-7);
        g_Chrome_MajorVersion = atoi(majorversion.c_str());
        WriteChromeMajorVersion();
      }
    }
  }

  TCHAR szWinText[512];
  MultiByteToWideChar(CP_UTF8, 0, NPVARIANT_TO_STRING(args[0]).UTF8Characters,
                      -1, szWinText, 512);

  int nLoop = 0;
  HWND hParentWnd = NULL;
  HWND hwnd;
  while(nLoop < 100) {
    hParentWnd = FindWindowEx(NULL, hParentWnd, L"Chrome_WidgetWin_0", NULL);
    if (hParentWnd != NULL) {
      hwnd = FindWindowEx(hParentWnd, NULL, L"Chrome_WidgetWin_0", szWinText);
      if (hwnd)
        break;
    }
    sprintf(szLog, "WindowText=%s,Len=%ld,hParentWnd=0x%X",
            NPVARIANT_TO_STRING(args[0]).UTF8Characters,
            NPVARIANT_TO_STRING(args[0]).UTF8Length,
            hParentWnd);
    g_Log.WriteLog("Loop", szLog);
    nLoop++;
    Sleep(10);
  }

  if (hwnd == NULL) {
    g_Log.WriteLog("Error", "No Find Window");
    return false;
  }

  MultiByteToWideChar(CP_UTF8, 0, NPVARIANT_TO_STRING(args[1]).UTF8Characters,
                      -1, szWinText, 512);

  SetWindowText(hwnd, szWinText);
  if (GetFirstChild(hwnd))
    SetWindowText(GetFirstChild(hwnd), szWinText);
  if (!SetWindowText(hParentWnd, szWinText)) {
    sprintf(szLog, "SetWindowText GetLastError=%ld", GetLastError());
    g_Log.WriteLog("Error", szLog);
  }
  SetWindowText(hParentWnd, szWinText);

  hwnd = hParentWnd;

  if (!hwnd) {
    sprintf(szLog, "WindowText=%s,Len=%ld,hParentWnd=0x%X",
            NPVARIANT_TO_STRING(args[0]).UTF8Characters,
            NPVARIANT_TO_STRING(args[0]).UTF8Length,
            hParentWnd);
    g_Log.WriteLog("ShowVideoAlone4", szLog);
    return false;
  }

  DWORD threadid = GetWindowThreadProcessId(hwnd, NULL);

  sprintf(szLog, "Chrome hwnd=0x%x,threadid=%ld", hwnd, threadid);
  g_Log.WriteLog("Msg", szLog);

  if (!hCallWndProcHook)
    hCallWndProcHook = SetWindowsHookEx(WH_CALLWNDPROC, CallWndProc,
                                        g_hMod, threadid);
  if (!hCallWndProcHook) {
    sprintf(szLog, "SetWindowsHookEx Failed,GetLastError=%ld",
            GetLastError());
    g_Log.WriteLog("Error", szLog);
  } else {
    g_Log.WriteLog("Msg", "SetWindowsHookEx Success");
    SendMessage(hwnd, WM_CHROMEHWND, (WPARAM)hwnd, (LPARAM)plugin_->get_hwnd());
  }

  HWND hEditHwnd = GetWindow(hwnd, GW_CHILD);
  while ((hEditHwnd = GetWindow(hEditHwnd, GW_HWNDNEXT)) != NULL && 
          GetClassNameA(hEditHwnd, szLog, 256) != 0 && 
          _stricmp(szLog, "Chrome_AutocompleteEditView") == 0) {
    SendMessage(hEditHwnd, WM_CLOSE, 0, 0);
    break;
  }

  WindowID_Item item = { 0 };
  item.parent_window_id = NPVARIANT_IS_INT32(args[2]) ? 
      NPVARIANT_TO_INT32(args[2]) : NPVARIANT_TO_DOUBLE(args[2]);
  item.window_id = NPVARIANT_IS_INT32(args[3]) ? 
      NPVARIANT_TO_INT32(args[3]) : NPVARIANT_TO_DOUBLE(args[3]);
  item.tab_id = NPVARIANT_IS_INT32(args[4]) ? 
      NPVARIANT_TO_INT32(args[4]) : NPVARIANT_TO_DOUBLE(args[4]);
  window_list_.insert(WindowMapPair(hwnd, item));

  BOOLEAN_TO_NPVARIANT(true, *result);

  return true;
}

bool VideoAloneScriptObject::Construct(const NPVariant *args,uint32_t argCount,
                                       NPVariant *result) {
  return true;
}