#include "stdafx.h"
#include "convenience_plugin.h"
#include "script_object_factory.h"
#include "convenience_script_object.h"
#include "log.h"
#include <vector>
#include "resource.h"
#include <TlHelp32.h>
#include <Psapi.h>
#include <map>

using namespace std;

extern Log g_Log;
extern HMODULE g_hMod;
bool g_IsListening = false;
bool g_Close_Last_Tab = false;
bool g_CloseChrome_Prompt = true;
bool g_DBClickCloseTab = true;
bool g_EnableSwitchTab = false;
Local_Message_Item g_Local_Message;
typedef map<HWND, Cmd_Msg_Item::Cmd_Msg_Value::TabCount> ChromeWindowIdMap;
ChromeWindowIdMap g_ChromeWindowMap;

DWORD g_ChromeMainThread = 0;

WNDPROC ConveniencePlugin::old_proc_ = NULL;
HHOOK g_KeyboardHook = NULL;
HHOOK g_GetMsgHook = NULL;
HHOOK g_CallWndHook = NULL;
HANDLE client_pipe_handle = INVALID_HANDLE_VALUE;
HANDLE client_thread_handle = INVALID_HANDLE_VALUE;

DWORD WINAPI Client_Thread(void* param);
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK GetMsgProc(int code, WPARAM wParam, LPARAM lParam);
void WriteToServer(const Cmd_Msg_Item& item);

const TCHAR* kFileMappingName = _T("Convenience_File");
const TCHAR* kMsgFileMappingName = _T("Convenience_Message_File");
const TCHAR* kChromeClassName = _T("Chrome_WidgetWin_0");
const TCHAR* kChromeAddressBar = _T("Chrome_AutocompleteEditView");
const TCHAR* kPipeName = _T("\\\\.\\pipe\\convenience");

const int kCloseTabButtonLeftOffset = 186;
const int kCloseTabButtonTopOffset = 18;
const int kCloseTabButtonLeftOffset_MaxState = 182;
const int kCloseTabButtonTopOffset_MaxState = 5;
const int kChromeWindowMinChangeSize = 343;
const int kCloseTabButtonWidth = 20;
const int kCloseTabButtonHeight = 20;

int map_current_used_flag = 2;
ConvenienceScriptObject::ShortCutKeyMap g_mapOne;
ConvenienceScriptObject::ShortCutKeyMap g_mapTwo;

int ReadPluginProcessId() {
  TCHAR current_path[MAX_PATH];
  GetCurrentDirectory(MAX_PATH, current_path);
  TCHAR file_name[MAX_PATH];
  _stprintf(file_name, _T("%s\\convenience.ini"), current_path);
  return GetPrivateProfileInt(_T("CFG"), _T("PID"), 0, file_name);
}

void WritePluginProcessId() {
  TCHAR current_path[MAX_PATH];
  GetCurrentDirectory(MAX_PATH, current_path);
  TCHAR file_name[MAX_PATH];
  _stprintf(file_name, _T("%s\\convenience.ini"), current_path);
  TCHAR value[32];
  _stprintf(value, _T("%ld"), GetCurrentProcessId());
  WritePrivateProfileString(_T("CFG"), _T("PID"), value, file_name);
}

void UpdateShortcutsFromMemory() {
  g_Log.WriteLog("msg", "UpdateShortcutsFromMemory");
  TCHAR filemap_name[MAX_PATH];
  _stprintf(filemap_name, _T("%s_%ld"), kFileMappingName, 
            ReadPluginProcessId());
  HANDLE memory_file_handle = OpenFileMapping(FILE_MAP_READ, FALSE,
                                              filemap_name);
  if (memory_file_handle) {
    LPVOID p = MapViewOfFile(memory_file_handle, FILE_MAP_READ, 0, 0, 0);
    if (p) {
      int count = 0;
      memcpy(&count, p, sizeof(int));
      ShortCut_Item* list = (ShortCut_Item*)((BYTE*)p+sizeof(int));
      ConvenienceScriptObject::ShortCutKeyMap* key_map_new;
      ConvenienceScriptObject::ShortCutKeyMap* key_map_old;
      if (map_current_used_flag == 1) {
        key_map_new = &g_mapTwo;
        key_map_old = &g_mapOne;
      } else {
        key_map_new = &g_mapOne;
        key_map_old = &g_mapTwo;
      }

      for (int i = 0; i < count; i++) {
        key_map_new->insert(make_pair(list[i].shortcuts_key, list[i]));
      }
      map_current_used_flag = map_current_used_flag == 1 ? 2 : 1;
      key_map_old->clear();
      g_Log.WriteLog("msg", "UpdateShortcutsSuccess");
      UnmapViewOfFile(p);
    }
    CloseHandle(memory_file_handle);
  }
}

void GetMessageFromMemory() {
  g_Log.WriteLog("msg", "GetMessageFromMemory");
  TCHAR filemap_name[MAX_PATH];
  _stprintf(filemap_name, _T("%s_%ld"), kMsgFileMappingName, 
            ReadPluginProcessId());
  HANDLE memory_file_handle = OpenFileMapping(FILE_MAP_READ, FALSE,
                                              filemap_name);
  if (memory_file_handle) {
    LPVOID p = MapViewOfFile(memory_file_handle, FILE_MAP_READ, 0, 0, 0);
    if (p) {
      memcpy(&g_Local_Message, p, sizeof(g_Local_Message));
      UnmapViewOfFile(p);
    }
    CloseHandle(memory_file_handle);
  }
}

void WriteMessageToMemory() {
  TCHAR filemap_name[MAX_PATH];
  _stprintf(filemap_name, _T("%s_%ld"), kMsgFileMappingName, 
            ReadPluginProcessId());

  HANDLE memory_file_handle = CreateFileMapping(NULL, NULL,
      PAGE_READWRITE|SEC_COMMIT,
      0, sizeof(g_Local_Message), filemap_name);
  if (memory_file_handle) {
    LPVOID p = MapViewOfFile(memory_file_handle,
        FILE_MAP_WRITE, 0, 0, sizeof(g_Local_Message));
    if (p) {
      memcpy(p, &g_Local_Message, sizeof(g_Local_Message));
      UnmapViewOfFile(p);
    }
  }
}

#define SIZEOF_STRUCT_WITH_SPECIFIED_LAST_MEMBER(struct_name, member) \
  offsetof(struct_name, member) + \
  (sizeof static_cast<struct_name*>(NULL)->member)
#define NONCLIENTMETRICS_SIZE_PRE_VISTA \
  SIZEOF_STRUCT_WITH_SPECIFIED_LAST_MEMBER(NONCLIENTMETRICS, lfMessageFont)

enum WinVersion {
  WINVERSION_PRE_2000 = 0,  // Not supported
  WINVERSION_2000 = 1,      // Not supported
  WINVERSION_XP = 2,
  WINVERSION_SERVER_2003 = 3,
  WINVERSION_VISTA = 4,
  WINVERSION_2008 = 5,
  WINVERSION_WIN7 = 6,
};

WinVersion GetWinVersion() {
  static bool checked_version = false;
  static WinVersion win_version = WINVERSION_PRE_2000;
  if (!checked_version) {
    OSVERSIONINFOEX version_info;
    version_info.dwOSVersionInfoSize = sizeof version_info;
    GetVersionEx(reinterpret_cast<OSVERSIONINFO*>(&version_info));
    if (version_info.dwMajorVersion == 5) {
      switch (version_info.dwMinorVersion) {
        case 0:
          win_version = WINVERSION_2000;
          break;
        case 1:
          win_version = WINVERSION_XP;
          break;
        case 2:
        default:
          win_version = WINVERSION_SERVER_2003;
          break;
      }
    } else if (version_info.dwMajorVersion == 6) {
      if (version_info.wProductType != VER_NT_WORKSTATION) {
        // 2008 is 6.0, and 2008 R2 is 6.1.
        win_version = WINVERSION_2008;
      } else {
        if (version_info.dwMinorVersion == 0) {
          win_version = WINVERSION_VISTA;
        } else {
          win_version = WINVERSION_WIN7;
        }
      }
    } else if (version_info.dwMajorVersion > 6) {
      win_version = WINVERSION_WIN7;
    }
    checked_version = true;
  }
  return win_version;
}

void GetNonClientMetrics(NONCLIENTMETRICS* metrics) {
  static const UINT SIZEOF_NONCLIENTMETRICS =
    (GetWinVersion() >= WINVERSION_VISTA) ?
    sizeof(NONCLIENTMETRICS) : NONCLIENTMETRICS_SIZE_PRE_VISTA;
  metrics->cbSize = SIZEOF_NONCLIENTMETRICS;
  const bool success = !!SystemParametersInfo(SPI_GETNONCLIENTMETRICS,
    SIZEOF_NONCLIENTMETRICS, metrics,
    0);
}

BOOL CALLBACK CloseChromeDialgProc(HWND dlg, UINT message, WPARAM wParam, 
                                   LPARAM lParam) {
  switch (message) { 
    case WM_INITDIALOG:
      {
        NONCLIENTMETRICS metrics;
        GetNonClientMetrics(&metrics);
        HFONT font = CreateFontIndirect(&(metrics.lfMenuFont));
        SendMessage(GetDlgItem(dlg, IDC_MESSAGE), WM_SETFONT, (WPARAM)font, TRUE);
        SendMessage(GetDlgItem(dlg, IDC_NOALERT), WM_SETFONT, (WPARAM)font, TRUE);
        SendMessage(GetDlgItem(dlg, IDOK), WM_SETFONT, (WPARAM)font, TRUE);
        SendMessage(GetDlgItem(dlg, IDCANCEL), WM_SETFONT, (WPARAM)font, TRUE);
        SetWindowText(dlg, g_Local_Message.msg_closechrome_title);
        SetDlgItemText(dlg, IDOK, g_Local_Message.msg_closechrome_ok);
        SetDlgItemText(dlg, IDCANCEL, g_Local_Message.msg_closechrome_cancel);
        SetDlgItemText(dlg, IDC_MESSAGE, g_Local_Message.msg_closechrome_message);
        SetDlgItemText(dlg, IDC_NOALERT, g_Local_Message.msg_closechrome_noalert);
      }
      break;
    case WM_COMMAND:
      if (LOWORD(wParam) == IDOK) {
        if (Button_GetCheck(GetDlgItem(dlg, IDC_NOALERT)) == BST_CHECKED) {
          Cmd_Msg_Item item;
          item.cmd = Cmd_Update_CloseChrome_Prompt;
          item.value.is_closechrome_prompt = false;
          WriteToServer(item);
          g_CloseChrome_Prompt = false;
        }
        HFONT font = (HFONT)SendMessage(GetDlgItem(dlg, IDC_MESSAGE), WM_GETFONT, 0, 0);
        if (font)
          DeleteObject(font);
        EndDialog(dlg, IDOK);
      }
      else if (LOWORD(wParam) == IDCANCEL)
        EndDialog(dlg, IDCANCEL);
      break;
  } 
  return FALSE; 
} 

DWORD WINAPI Client_Thread(void* param) {
  char szLog[256];
  char buffer[MAX_BUFFER_LEN];
  DWORD outlen;
  int offset = 0;
  int readlen = sizeof(Cmd_Msg_Item);
  Cmd_Msg_Item cmd;

  OVERLAPPED ol = { 0 };
  ol.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
  TCHAR pipe_name[MAX_PATH];
  _stprintf(pipe_name, _T("%s_%ld"), kPipeName, ReadPluginProcessId());

  while(true) {
    client_pipe_handle = CreateFile(pipe_name, GENERIC_READ | GENERIC_WRITE,
                                    0, NULL, OPEN_EXISTING, 
                                    FILE_FLAG_OVERLAPPED, NULL);

    if (client_pipe_handle == INVALID_HANDLE_VALUE) {
      DWORD errorcode = GetLastError();
      if (errorcode == ERROR_PIPE_BUSY) {
        sprintf(szLog, "CreateFile Failed,GetLastError=%ld", errorcode);
        g_Log.WriteLog("Error", szLog);
        Sleep(10);
        continue;
      } else {
        sprintf(szLog, "CreateFile Failed,GetLastError=%ld", errorcode);
        g_Log.WriteLog("Error", szLog);
        Sleep(10);
        continue;
      }
    } else {
      g_Log.WriteLog("Msg", "CreateFile success, start client thread");
    }

    cmd.cmd = Cmd_Request_Update;
    WriteToServer(cmd);

    BOOL result;
    while (true) {
      result = ReadFile(client_pipe_handle, buffer+offset, readlen,
                        NULL, &ol);
      if (result || (!result && GetLastError() == ERROR_IO_PENDING)) {
        if (!GetOverlappedResult(client_pipe_handle, &ol, &outlen, TRUE))
          break;
      } else {
        sprintf(szLog, "Client ReadFile Error,GetLastError=%ld", 
                GetLastError());
        g_Log.WriteLog("Error", szLog);
        break;
      }
      if (outlen == readlen) {
        memcpy(&cmd, buffer, sizeof(cmd));
        sprintf(szLog, "recv from server, cmd=%ld",cmd.cmd);
        g_Log.WriteLog("ReadFile", szLog);
        switch(cmd.cmd) {
        case Cmd_Update_Shortcuts:
          UpdateShortcutsFromMemory();
          break;
        case Cmd_Response_Update:
          break;
        case Cmd_Update_Local_Message:
          GetMessageFromMemory();
          break;
        case Cmd_Update_CloseChrome_Prompt:
          g_CloseChrome_Prompt = cmd.value.is_closechrome_prompt;
          break;
        case Cmd_Update_DBClick_CloseTab:
          g_DBClickCloseTab = cmd.value.double_click_closetab;
          break;
        case Cmd_Update_Is_Listening:
          g_IsListening = cmd.value.is_listening;
          break;
        case Cmd_Update_CloseLastTab:
          g_Close_Last_Tab = cmd.value.close_last_tab;
          break;
        case Cmd_Update_SwitchTab:
          g_EnableSwitchTab = cmd.value.enable_switch_tab;
          break;
        case Cmd_Update_TabCount:
          {  
            ChromeWindowIdMap::iterator iter;
            for (iter = g_ChromeWindowMap.begin(); iter != g_ChromeWindowMap.end();
                 iter++) {
              if (iter->second.windowid == cmd.value.tabcount.windowid) {
                iter->second.tabcount = cmd.value.tabcount.tabcount;
                sprintf(szLog, "windowid=%d, tabcount=%ld", 
                  cmd.value.tabcount.windowid,
                  cmd.value.tabcount.tabcount);
                g_Log.WriteLog("Cmd_Update_TabCount", szLog);
                break;
              }
            }
          }
          break;
        case Cmd_ChromeWindowCreated:
          {
            Cmd_Msg_Item::Cmd_Msg_Value::TabCount tabcount;
            tabcount.windowid = cmd.value.chrome_window.windowid;
            tabcount.tabcount = 1;
            g_ChromeWindowMap.insert(make_pair(
                cmd.value.chrome_window.chrome_handle, 
                tabcount));
            sprintf(szLog, "handle=%X, windowid=%ld", 
                    cmd.value.chrome_window.chrome_handle,
                    cmd.value.chrome_window.windowid);
            g_Log.WriteLog("ChromeWindowCreated", szLog);
          }
          break;
        case Cmd_ChromeWindowRemoved:
          {
            ChromeWindowIdMap::iterator iter;
            for (iter = g_ChromeWindowMap.begin(); iter != g_ChromeWindowMap.end();
                 iter++) {
              if (iter->second.windowid == cmd.value.chrome_window.windowid) {
                g_ChromeWindowMap.erase(iter);
                break;
              }
            }
          }
          break;
        case Cmd_ServerShutDown:
          {
            Cmd_Msg_Item item;
            item.cmd = Cmd_ClientShutDown;
            WriteToServer(item);
            CloseHandle(client_pipe_handle);
            client_thread_handle = INVALID_HANDLE_VALUE;
          }
          return 0;
        }
        readlen = sizeof(Cmd_Msg_Item);
        offset = 0;
      } else {
        offset += outlen;
        if (offset == sizeof(Cmd_Msg_Item))
          offset = 0;
        readlen = sizeof(Cmd_Msg_Item) - offset;
      }
    }
    g_Log.WriteLog("error", "client thread readfile failed, closefile");
    CloseHandle(client_pipe_handle);
    client_pipe_handle = INVALID_HANDLE_VALUE;
  }
  return 0;
}

DWORD ConveniencePlugin::Server_Thread(void* param) {
  char szLog[256];
  char buffer[MAX_BUFFER_LEN];
  DWORD outlen;
  int offset = 0;
  int readlen = sizeof(Cmd_Msg_Item);
  Cmd_Msg_Item cmd;

  ConveniencePlugin* pPlugin = (ConveniencePlugin*)param;
  TCHAR pipe_name[MAX_PATH];
  _stprintf(pipe_name, _T("%s_%ld"), kPipeName, ReadPluginProcessId());

  pPlugin->server_pipe_handle_ = CreateNamedPipe(pipe_name,
      PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
      PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
      1, MAX_BUFFER_LEN, MAX_BUFFER_LEN, 0, NULL);
  if (pPlugin->server_pipe_handle_ == INVALID_HANDLE_VALUE) {
    sprintf(szLog, "CreateNamedPipe Failed,GetLastError=%ld", GetLastError());
    g_Log.WriteLog("Error", szLog);
    return -1;
  }
  g_Log.WriteLog("Msg", "Start Server_Thread");

  OVERLAPPED ol = {0};
  ol.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
  while (true) {
	  if (ConnectNamedPipe(pPlugin->server_pipe_handle_, &ol)) {
      g_Log.WriteLog("Error", "ConnectNamedPipe Failed");
      Sleep(10);
      continue;
    } else {
      WaitForSingleObject(ol.hEvent, INFINITE);
      g_Log.WriteLog("msg", "pipe client connected");
    }

    pPlugin->UpdateDBClick_CloseTab(g_DBClickCloseTab);
    pPlugin->UpdateCloseChromePromptFlag(g_CloseChrome_Prompt);
    pPlugin->UpdateCloseLastTab(g_Close_Last_Tab);
    pPlugin->EnableMouseSwitchTab(g_EnableSwitchTab);
    ChromeWindowIdMap::iterator iter;
    for (iter = g_ChromeWindowMap.begin(); iter != g_ChromeWindowMap.end();
         iter++) {
      Cmd_Msg_Item item;
      item.cmd = Cmd_ChromeWindowCreated;
      item.value.chrome_window.chrome_handle = iter->first;
      item.value.chrome_window.windowid = iter->second.windowid;
      pPlugin->WriteToClient(item);
    }
    g_ChromeWindowMap.clear();
    
    BOOL result;
    while (true) {
      result = ReadFile(pPlugin->server_pipe_handle_, buffer+offset, readlen,
                        NULL, &ol);
      if (result || (!result && GetLastError() == ERROR_IO_PENDING)) {
        if (!GetOverlappedResult(pPlugin->server_pipe_handle_, 
                                 &ol, &outlen, TRUE))
          break;
      } else {
        sprintf(szLog, "ReadFile Error,GetLastError=%ld", GetLastError());
        g_Log.WriteLog("Error", szLog);
        break;
      }

      if (outlen == readlen) {
        memcpy(&cmd, buffer, sizeof(cmd));
        sprintf(szLog, "recv from client, cmd=%ld", cmd.cmd);
        g_Log.WriteLog("ReadFile", szLog);
        switch(cmd.cmd) {
        case Cmd_Request_Update:
          if (pPlugin->memory_file_handle_) {
            Cmd_Msg_Item item;
            item.cmd = Cmd_Update_Shortcuts;
            pPlugin->WriteToClient(item);
          } else {
            Cmd_Msg_Item item;
            item.cmd = Cmd_Response_Update;
            pPlugin->WriteToClient(item);
          }
          break;
        case Cmd_Event:
          EnterCriticalSection(&pPlugin->cs_);
          pPlugin->shortcuts_queue_.push(cmd.value.shortcuts_Id);
          LeaveCriticalSection(&pPlugin->cs_);
          break;
        case Cmd_ChromeClose:
          PostMessage(pPlugin->hwnd_, WM_CHROMECLOSE, 0, 0);
          break;
        case Cmd_TabClose:
          PostMessage(pPlugin->hwnd_, WM_TABCLOSE, 0, 0);
          break;
        case Cmd_Update_CloseChrome_Prompt:
          PostMessage(pPlugin->hwnd_, WM_UPDATE_CLOSECHROME_PROMPT,
                      cmd.value.is_closechrome_prompt, 0);
          break;
        case Cmd_DBClick_CloseTab:
          PostMessage(pPlugin->hwnd_, WM_CLOSE_CURRENT_TAB, 0, 0);
          break;
        case  Cmd_KeyDown:
          PostMessage(pPlugin->hwnd_, WM_KEYDOWN, cmd.value.key_down.wparam,
                      cmd.value.key_down.lparam);
          break;
        case Cmd_KeyUp:
          PostMessage(pPlugin->hwnd_, WM_KEYUP, cmd.value.key_down.wparam,
                      cmd.value.key_down.lparam);
          break;
        case Cmd_MouseRotated:
          PostMessage(pPlugin->hwnd_, WM_CHROMEMOUSEWHEEL, 
                      0, cmd.value.rotatedcount);
          break;
        case Cmd_ClientShutDown:
          if (pPlugin->memory_file_handle_)
            CloseHandle(pPlugin->memory_file_handle_);
          if (pPlugin->server_pipe_handle_)
            CloseHandle(pPlugin->server_pipe_handle_);
          return 0;
        }
        readlen = sizeof(Cmd_Msg_Item);
        offset = 0;
      } else {
        offset += outlen;
        if (offset == sizeof(Cmd_Msg_Item))
          offset = 0;
        readlen = sizeof(Cmd_Msg_Item) - offset;
      }
    }
    g_Log.WriteLog("Error", "recv from client failed, DisconnectNamedPipe");
    DisconnectNamedPipe(pPlugin->server_pipe_handle_);
  }
  return 0;
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
  if (!(HIWORD(lParam) & KF_REPEAT)) {
    string shortcuts;
    char virual_key[MAX_KEY_LEN];
    if (GetKeyState(VK_CONTROL) & 0x80)
      shortcuts = "17";
    if (GetKeyState(VK_MENU) & 0x80) {
      if (shortcuts.length() > 0)
        shortcuts += "+18";
      else
        shortcuts = "18";
    }
    if (GetKeyState(VK_SHIFT) & 0x80) {
      if (shortcuts.length() > 0)
        shortcuts += "+16";
      else
        shortcuts = "16";
    }
    _itoa(wParam, virual_key, 10);
    if (shortcuts.length() > 0) {
      shortcuts += "+";
      shortcuts += virual_key;
    }
    else
      shortcuts = virual_key;

    HWND hwnd = GetForegroundWindow();
    ChromeWindowIdMap::iterator tabcount_iter = g_ChromeWindowMap.find(hwnd);
    if ((wParam == VK_F4 || wParam == 'W') && 
        ((GetKeyState(VK_CONTROL) & 0x80)) && g_Close_Last_Tab &&
        tabcount_iter != g_ChromeWindowMap.end() && 
        tabcount_iter->second.tabcount == 1) {
      Cmd_Msg_Item item;
      item.cmd = Cmd_TabClose;
      WriteToServer(item);
      return TRUE;
    }

    ConvenienceScriptObject::ShortCutKeyMap* shortcut_map;
    if (map_current_used_flag == 1)
      shortcut_map = &g_mapOne;
    else
      shortcut_map = &g_mapTwo;

    ConvenienceScriptObject::ShortCutKeyMap::iterator iter = shortcut_map->
        find(shortcuts);

    Cmd_Msg_Item item;
    if (g_IsListening) {
      item.cmd = Cmd_KeyDown;
      item.value.key_down.wparam = wParam;
      item.value.key_down.lparam = lParam;
      WriteToServer(item);
    } else {
      item.cmd = Cmd_Event;
      if (iter != shortcut_map->end() && client_pipe_handle) {
        item.value.shortcuts_Id = iter->second.index;
        WriteToServer(item);
      }
    }
  }
  else if (HIWORD(lParam) & KF_UP) {
    if (g_IsListening) {
      Cmd_Msg_Item item;
      item.cmd = Cmd_KeyUp;
      item.value.key_down.wparam = wParam;
      item.value.key_down.lparam = lParam;
      WriteToServer(item);
    }
  }

  return CallNextHookEx(g_KeyboardHook, nCode, wParam, lParam);
}

LRESULT CALLBACK GetMsgProc(int code, WPARAM wParam, LPARAM lParam){
  if (client_thread_handle == INVALID_HANDLE_VALUE) {
    client_thread_handle = CreateThread(NULL, 0, Client_Thread, NULL, 0, 
                                        NULL);
  }
  MSG* msg = (MSG*)lParam;

  ChromeWindowIdMap::iterator iter = g_ChromeWindowMap.find(msg->hwnd);
  if (iter == g_ChromeWindowMap.end())
    return CallNextHookEx(g_GetMsgHook, code, wParam, lParam);

  bool is_only_one_tab = (iter->second.tabcount == 1);

  if ((msg->message == WM_LBUTTONDOWN || msg->message == WM_LBUTTONDBLCLK) 
      && wParam == PM_REMOVE && msg->wParam == MK_LBUTTON && g_Close_Last_Tab 
      && is_only_one_tab) {
    TCHAR class_name[256];
    GetClassName(msg->hwnd, class_name, 256);
    if (_tcscmp(class_name, kChromeClassName) == 0 && 
        GetParent(msg->hwnd) == NULL) { 
      RECT rt = {0};
      if (IsMaximized(msg->hwnd)) {
        rt.left = kCloseTabButtonLeftOffset_MaxState;
        rt.top = kCloseTabButtonTopOffset_MaxState;
      } else {
        RECT chrome_rect = { 0 };
        GetWindowRect(msg->hwnd, &chrome_rect);
        if (chrome_rect.right - chrome_rect.left < kChromeWindowMinChangeSize) {
          rt.left = kCloseTabButtonLeftOffset - 
                    (kChromeWindowMinChangeSize - 
                    (chrome_rect.right - chrome_rect.left));
          rt.top = kCloseTabButtonTopOffset;
        } else {
          rt.left = kCloseTabButtonLeftOffset;
          rt.top = kCloseTabButtonTopOffset;
        }
      }
      rt.bottom = rt.top + kCloseTabButtonHeight;
      rt.right = rt.left + kCloseTabButtonWidth;
      POINT pt;
      pt.x = GET_X_LPARAM(msg->lParam);
      pt.y = GET_Y_LPARAM(msg->lParam);
      char logs[256];
      sprintf(logs, "x=%ld, y=%ld", pt.x, pt.y);
      g_Log.WriteLog("ClickCloseTab", logs);
      if (PtInRect(&rt, pt)) {
        msg->message = WM_NULL;
        Cmd_Msg_Item item;
        item.cmd = Cmd_TabClose;
        WriteToServer(item);
      }
    }
  }

  if ((msg->message == WM_NCLBUTTONDOWN && wParam == PM_REMOVE && 
       msg->wParam == HTCLOSE) && g_CloseChrome_Prompt && !is_only_one_tab) {
    TCHAR class_name[256];
    GetClassName(msg->hwnd, class_name, 256);
    HWND address_hwnd = FindWindowEx(msg->hwnd, NULL, kChromeAddressBar, NULL);
    if (_tcscmp(class_name, kChromeClassName) == 0 && 
        address_hwnd && (GetWindowLong(address_hwnd, GWL_STYLE) & WS_VISIBLE)) {
      msg->message = WM_NULL;
      Cmd_Msg_Item item;
      item.cmd = Cmd_ChromeClose;
      WriteToServer(item);
    }
  }

  if ((msg->message == WM_SYSCOMMAND && wParam == PM_REMOVE && 
       msg->wParam == SC_CLOSE) && g_CloseChrome_Prompt && !is_only_one_tab) {
    TCHAR class_name[256];
    GetClassName(msg->hwnd, class_name, 256);
    HWND address_hwnd = FindWindowEx(msg->hwnd, NULL, kChromeAddressBar, NULL);
    if (_tcscmp(class_name, kChromeClassName) == 0 && 
        address_hwnd && (GetWindowLong(address_hwnd, GWL_STYLE) & WS_VISIBLE)) {
      if (DialogBox(g_hMod, MAKEINTRESOURCE(IDD_CLOSECHROME), msg->hwnd,
                    CloseChromeDialgProc) != IDOK) {
        msg->message = WM_NULL;
      }
    }
  }

  if ((msg->message == WM_LBUTTONDBLCLK || msg->message == WM_MBUTTONDOWN ||
       msg->message == WM_MBUTTONDBLCLK) && wParam == PM_REMOVE) {
    POINT pt;
    pt.x = GET_X_LPARAM(msg->lParam);
    pt.y = GET_Y_LPARAM(msg->lParam);
    RECT tab_client_rect = { 0 };
    GetClientRect(msg->hwnd, &tab_client_rect);
    if (IsMaximized(msg->hwnd)) {
      tab_client_rect.top = 0;
      tab_client_rect.bottom = 25;
    } else {
      tab_client_rect.top += 15;
      tab_client_rect.bottom = tab_client_rect.top + 25;
    }

    char logs[256];
    sprintf(logs, "x=%ld,y=%ld,left=%ld,right=%ld,top=%ld,\
bottom=%ld,is_only_one_tab=%d",
            pt.x, pt.y, tab_client_rect.left, tab_client_rect.right,
            tab_client_rect.top, tab_client_rect.bottom, is_only_one_tab);
    g_Log.WriteLog("DBClickTab", logs);

    if (!PtInRect(&tab_client_rect, pt))
      return CallNextHookEx(g_GetMsgHook, code, wParam, lParam);

    TCHAR class_name[256];
    GetClassName(msg->hwnd, class_name, 256);
    if (_tcscmp(class_name, kChromeClassName) == 0 && 
        GetParent(msg->hwnd) == NULL) {
      Cmd_Msg_Item item;
      if (g_Close_Last_Tab && is_only_one_tab) {
        msg->message = WM_NULL;
        item.cmd = Cmd_TabClose;
        WriteToServer(item);
      } else if (msg->message == WM_LBUTTONDBLCLK && g_DBClickCloseTab) {
        msg->message = WM_NULL;
        item.cmd = Cmd_DBClick_CloseTab;
        WriteToServer(item);
      }
    }
  }
  return CallNextHookEx(g_GetMsgHook, code, wParam, lParam);
}

LRESULT CALLBACK CallWndProcHook(int code, WPARAM wParam, LPARAM lParam){
  CWPSTRUCT* msg = (CWPSTRUCT*)lParam;

  ChromeWindowIdMap::iterator iter = g_ChromeWindowMap.find(msg->hwnd);
  if (g_EnableSwitchTab && iter != g_ChromeWindowMap.end()) {
    switch(msg->message) {
      case WM_MOUSEWHEEL:
        {
          RECT window_rect = {0};
          POINT pt;
          pt.x = GET_X_LPARAM(msg->lParam);
          pt.y = GET_Y_LPARAM(msg->lParam);
          GetWindowRect(msg->hwnd, &window_rect);
          if (PtInRect(&window_rect, pt)) {
            Cmd_Msg_Item item;
            item.cmd = Cmd_MouseRotated;
            item.value.rotatedcount = ((short)HIWORD(msg->wParam) / WHEEL_DELTA);
            WriteToServer(item);
          }
        }
        break;
      default:
        break;
    }
  }
  return CallNextHookEx(g_CallWndHook, code, wParam, lParam);
}

ConveniencePlugin::ConveniencePlugin(void) {
  memory_file_handle_ = NULL;
  server_thread_handle_ = NULL;
  get_message_flag_ = false;
  InitializeCriticalSection(&cs_);
}

ConveniencePlugin::~ConveniencePlugin(void) {
  DeleteCriticalSection(&cs_);
}

NPError ConveniencePlugin::Init(NPP instance, uint16_t mode, int16_t argc,
                                char *argn[], char *argv[],
                                NPSavedData *saved) {
  scriptobject_ = NULL;
  instance->pdata = this;
  g_Log.WriteLog("Msg", "ConveniencePlugin Init");
  WritePluginProcessId();

  server_thread_handle_ = CreateThread(NULL, 0, Server_Thread, this, 0, NULL);
  TCHAR exe_name[MAX_PATH];
  GetModuleBaseName(GetCurrentProcess(), GetModuleHandle(NULL), exe_name, MAX_PATH);
  HANDLE hprocess = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  PROCESSENTRY32 process = { sizeof(PROCESSENTRY32) };
  DWORD parent_processid = 0;
  BOOL ret = Process32First(hprocess, &process);
  while (ret) {
    if (_tcsicmp(process.szExeFile, exe_name) == 0 && 
        process.th32ProcessID == GetCurrentProcessId()) {
      parent_processid = process.th32ParentProcessID;
      break;
    }
    ret = Process32Next(hprocess, &process);
  }
  if (hprocess != INVALID_HANDLE_VALUE)
    CloseHandle(hprocess);
  HWND chrome_hwnd = FindWindowEx(NULL, NULL, kChromeClassName, NULL);
  while (chrome_hwnd) {
    DWORD process_id;
    g_ChromeMainThread = GetWindowThreadProcessId(chrome_hwnd, &process_id);
    if (process_id == parent_processid) {
      g_KeyboardHook = SetWindowsHookEx(WH_KEYBOARD, KeyboardProc, g_hMod, 
                                        g_ChromeMainThread);
      g_GetMsgHook = SetWindowsHookEx(WH_GETMESSAGE, GetMsgProc, g_hMod, 
                                      g_ChromeMainThread);
      g_CallWndHook = SetWindowsHookEx(WH_CALLWNDPROC, CallWndProcHook, g_hMod,
                                       g_ChromeMainThread);
      if (!g_KeyboardHook || !g_GetMsgHook || !g_CallWndHook) {
        return NPERR_GENERIC_ERROR;
      } else {
        g_Log.WriteLog("Msg", "ConveniencePlugin Init Success");
        break;
      }
    } else {
      chrome_hwnd = FindWindowEx(NULL, chrome_hwnd, kChromeClassName, NULL);
    }
  }
  return PluginBase::Init(instance, mode, argc, argn, argv, saved);
}

NPError ConveniencePlugin::UnInit(NPSavedData **save) {
  PluginBase::UnInit(save);
  scriptobject_ = NULL;

  Cmd_Msg_Item item;
  item.cmd = Cmd_ServerShutDown;
  WriteToClient(item);

  UnhookWindowsHookEx(g_GetMsgHook);
  UnhookWindowsHookEx(g_KeyboardHook);
  UnhookWindowsHookEx(g_CallWndHook);

  if (WaitForSingleObject(server_thread_handle_, 10) == WAIT_TIMEOUT) {
    TerminateThread(server_thread_handle_, 0);
    if (memory_file_handle_)
      CloseHandle(memory_file_handle_);
    if (server_pipe_handle_)
      CloseHandle(server_pipe_handle_);
  }

  return NPERR_NO_ERROR;
}

bool ConveniencePlugin::GetNPMessage(int index, TCHAR* msg, int msglen) {
  NPObject* window;
  _tcscpy(msg, _T(""));
  NPN_GetValue(npp_, NPNVWindowNPObject, &window);
  NPIdentifier id;
  id = NPN_GetStringIdentifier("getNPMessage");
  NPVariant result;
  VOID_TO_NPVARIANT(result);
  if (id) {
    NPVariant param;
    INT32_TO_NPVARIANT(index, param);
    if (NPN_Invoke(npp_, window, id, &param, 1, &result)) {
      if (MultiByteToWideChar(CP_UTF8, 0, 
        NPVARIANT_TO_STRING(result).UTF8Characters, -1, msg, msglen)) {
      }
      NPN_ReleaseVariantValue(&result);
      return true;
    }
  }

  return false;
}

NPError ConveniencePlugin::GetValue(NPPVariable variable, void *value) {
  switch(variable) {
    case NPPVpluginScriptableNPObject:
      if (scriptobject_ == NULL) {
        scriptobject_ = ScriptObjectFactory::CreateObject(npp_,
            ConvenienceScriptObject::Allocate);
        g_Log.WriteLog("GetValue", "GetValue");
        NPN_RetainObject(scriptobject_);
      }
      if (scriptobject_ != NULL) {
        *(NPObject**)value = scriptobject_;
      }
      else
        return NPERR_OUT_OF_MEMORY_ERROR;
      break;
    default:
      return NPERR_GENERIC_ERROR;
  }

  return NPERR_NO_ERROR;
}

NPError ConveniencePlugin::SetWindow(NPWindow* window) {
  PluginBase::SetWindow(window);

  if (hwnd_ == NULL && old_proc_ != NULL) {
    KillTimer(hwnd_, 1);
    SubclassWindow(hwnd_, old_proc_);
    old_proc_ = NULL; 
  }

  if (hwnd_ != NULL && old_proc_ == NULL) {
    old_proc_ = SubclassWindow(hwnd_, WndProc);
    SetWindowLong(hwnd_, GWLP_USERDATA, (LONG)this);
    SetTimer(hwnd_, 1, 10, NULL);
  }

  return NPERR_NO_ERROR;
}

void ConveniencePlugin::SetShortcutsToMemory(ShortCut_Item* list, int count) {
  if (!memory_file_handle_)
    CloseHandle(memory_file_handle_);

  int num = sizeof(ShortCut_Item)*count + sizeof(int);
  TCHAR filemap_name[MAX_PATH];
  _stprintf(filemap_name, _T("%s_%ld"), kFileMappingName, ReadPluginProcessId());

  memory_file_handle_ = CreateFileMapping(NULL, NULL,
                                          PAGE_READWRITE|SEC_COMMIT,
                                          0, num, filemap_name);
  if (memory_file_handle_) {
    LPVOID p = MapViewOfFile(memory_file_handle_,
                             FILE_MAP_WRITE, 0, 0, num);
    if (p) {
      memcpy(p, &count, sizeof(int));
      memcpy((BYTE*)p+sizeof(int), list, sizeof(ShortCut_Item)*count);
      UnmapViewOfFile(p);
      Cmd_Msg_Item item;
      item.cmd = Cmd_Update_Shortcuts;
      WriteToClient(item);
    }
  } else {
    char szLog[256];
    sprintf(szLog, "GetLastError=%ld", GetLastError());
    g_Log.WriteLog("Error", szLog);
  }
}

void ConveniencePlugin::UpdateDBClick_CloseTab(bool double_click_closetab) {
  Cmd_Msg_Item item;
  item.cmd = Cmd_Update_DBClick_CloseTab;
  item.value.double_click_closetab = double_click_closetab;
  g_DBClickCloseTab = double_click_closetab;
  WriteToClient(item);
}

void ConveniencePlugin::UpdateIsListening(bool is_listening) {
  Cmd_Msg_Item item;
  item.cmd = Cmd_Update_Is_Listening;
  item.value.is_listening = is_listening;
  WriteToClient(item);
}

void ConveniencePlugin::UpdateTabCount(int windowid, int tabcount) {
  Cmd_Msg_Item item;
  item.cmd = Cmd_Update_TabCount;
  item.value.tabcount.windowid = windowid;
  item.value.tabcount.tabcount = tabcount;
  WriteToClient(item);
}

void ConveniencePlugin::UpdateCloseChromePromptFlag(bool flag) {
  Cmd_Msg_Item item;
  item.cmd = Cmd_Update_CloseChrome_Prompt;
  item.value.is_closechrome_prompt = flag;
  g_CloseChrome_Prompt = flag;
  WriteToClient(item);
}

void ConveniencePlugin::UpdateCloseLastTab(bool close_last_tab) {
  Cmd_Msg_Item item;
  item.cmd = Cmd_Update_CloseLastTab;
  item.value.close_last_tab = close_last_tab;
  g_Close_Last_Tab = close_last_tab;
  WriteToClient(item);
}

void ConveniencePlugin::ChromeWindowCreated(int windowid) {
  Cmd_Msg_Item item;
  item.cmd = Cmd_ChromeWindowCreated;
  item.value.chrome_window.windowid = windowid;
  item.value.chrome_window.chrome_handle = NULL;
  HWND chrome_hwnd = FindWindowEx(NULL, NULL, kChromeClassName, NULL);
  char logs[256];
  while(chrome_hwnd) {
    BOOL visible = IsWindowVisible(chrome_hwnd);
    HWND hwnd = GetParent(chrome_hwnd);
    sprintf(logs, "chrome_hwnd=%X,IsWindowVisible=%d,GetParent=%X", chrome_hwnd, 
            visible, hwnd);
    g_Log.WriteLog("create", logs);
    if (GetWindowThreadProcessId(chrome_hwnd, NULL) == g_ChromeMainThread &&
        hwnd == NULL) {
        item.value.chrome_window.chrome_handle = chrome_hwnd;
        Cmd_Msg_Item::Cmd_Msg_Value::TabCount tabcount;
        tabcount.windowid = windowid;
        tabcount.tabcount = 1;
        g_ChromeWindowMap.insert(make_pair(chrome_hwnd, tabcount));
        break;
    }
    chrome_hwnd = FindWindowEx(NULL, chrome_hwnd, kChromeClassName, NULL);
  }

  WriteToClient(item);
}

void ConveniencePlugin::ChromeWindowRemoved(int windowid) {
  Cmd_Msg_Item item;
  item.cmd = Cmd_ChromeWindowRemoved;
  item.value.chrome_window.windowid = windowid;
  WriteToClient(item);
}

void ConveniencePlugin::EnableMouseSwitchTab(bool flag) {
  Cmd_Msg_Item item;
  item.cmd = Cmd_Update_SwitchTab;
  item.value.enable_switch_tab = flag;
  g_EnableSwitchTab = flag;
  WriteToClient(item);
}

void ConveniencePlugin::GetLocalMessage() {
  if (get_message_flag_)
    return;

  GetNPMessage(MSG_BOSSKEY_DEFINED, g_Local_Message.msg_bosskey_defined,
      sizeof(g_Local_Message.msg_bosskey_defined));
  GetNPMessage(MSG_ALWAYS_ON_TOP, g_Local_Message.msg_always_on_top,
      sizeof(g_Local_Message.msg_always_on_top));
  GetNPMessage(MSG_CLOSECHROME_TITLE, g_Local_Message.msg_closechrome_title,
      sizeof(g_Local_Message.msg_closechrome_title));
  GetNPMessage(MSG_CLOSECHROME_MESSAGE, g_Local_Message.msg_closechrome_message,
      sizeof(g_Local_Message.msg_closechrome_message));
  GetNPMessage(MSG_CLOSECHROME_OK, g_Local_Message.msg_closechrome_ok,
      sizeof(g_Local_Message.msg_closechrome_ok));
  GetNPMessage(MSG_CLOSECHROME_CANCEL, g_Local_Message.msg_closechrome_cancel,
      sizeof(g_Local_Message.msg_closechrome_cancel));
  GetNPMessage(MSG_CLOSECHROME_NOALERT, g_Local_Message.msg_closechrome_noalert,
      sizeof(g_Local_Message.msg_closechrome_noalert));
  WriteMessageToMemory();
  get_message_flag_ = true;
  Cmd_Msg_Item item;
  item.cmd = Cmd_Update_Local_Message;
  WriteToClient(item);
}

LRESULT ConveniencePlugin::WndProc(HWND hWnd, UINT Msg, 
                                    WPARAM wParam, LPARAM lParam) {
  ConveniencePlugin* plugin = (ConveniencePlugin*)GetWindowLong(hWnd, 
                                                                GWLP_USERDATA);
  ConvenienceScriptObject* pObject = (ConvenienceScriptObject*)plugin->
      scriptobject_;
  if (!pObject)
    return CallWindowProc(old_proc_, hWnd, Msg, wParam, lParam);

  switch(Msg) {
    case WM_TIMER:
      EnterCriticalSection(&plugin->cs_);
      while (!plugin->shortcuts_queue_.empty()) {
        g_Log.WriteLog("WndProc", "WM_TIMER");
        int shortcut_id = plugin->shortcuts_queue_.front();
        plugin->shortcuts_queue_.pop();
        if (!pObject->get_is_listened())
          pObject->TriggerEvent(shortcut_id);
      }
      LeaveCriticalSection(&plugin->cs_);
      break;
    case WM_CHROMECLOSE:
      pObject->TriggerShortcuts(MOD_ALT, VK_F4);
      break;
    case WM_TRIGGER_CHROME_SHORTCUTS:
      pObject->TriggerShortcuts(wParam, lParam);
      break;
    case WM_TABCLOSE:
      pObject->TriggerTabClose();
      break;
    case WM_CLOSE_CURRENT_TAB:
      pObject->TriggerShortcuts(MOD_CONTROL, VK_F4);
      break;
    case WM_UPDATE_CLOSECHROME_PROMPT:
      pObject->UpdateCloseChromePromptFlag(wParam);
      break;
    case WM_CHROMEMOUSEWHEEL:
      {
        bool forward = lParam > 0 ? true : false;
        pObject->TriggerSwitchTab(forward);
      }
      break;
    case WM_KEYDOWN:
      {
        bool control = false;
        bool alt = false;
        bool shift = false;
        if (GetKeyState(VK_CONTROL) & 0x80)
          control = true;
        if (GetKeyState(VK_MENU) & 0x80)
          alt = true;
        if (GetKeyState(VK_SHIFT) & 0x80)
          shift = true;
        char logs[256];
        sprintf(logs, "wparam=%d, lparam=%d, control=%d, shift=%d, alt=%d", 
                wParam, lParam, control, shift, alt);
        g_Log.WriteLog("keydown", logs);
        pObject->OnKeyDown(control, alt, shift, wParam, lParam);
      }
      break;
    case WM_KEYUP:
      break;
    case WM_HOTKEY:
      {
        if (pObject->get_is_listened())
          break;

        g_Log.WriteLog("msg", "WM_HOTKEY");
        char shortcuts_name[MAX_KEY_LEN];
        GlobalGetAtomNameA(wParam, shortcuts_name, MAX_KEY_LEN);
        pObject->TriggerEvent(shortcuts_name);
      }
      break;
    default:
      return CallWindowProc(old_proc_, hWnd, Msg, wParam, lParam);
  }
  return TRUE;
}

void ConveniencePlugin::WriteToClient(const Cmd_Msg_Item& item) {
  char logs[256];
  sprintf(logs, "WriteToClient, cmd=%d", item.cmd);
  DWORD writelen;
  if (WriteFile(server_pipe_handle_, &item, sizeof(item), &writelen, NULL))
    g_Log.WriteLog("Send", logs);
  else
    g_Log.WriteLog("Send Error", logs);
}

void WriteToServer(const Cmd_Msg_Item& item) {
  char logs[256];
  sprintf(logs, "WriteToServer, cmd=%d", item.cmd);
  DWORD writelen;
  if (WriteFile(client_pipe_handle, &item, sizeof(item), &writelen, NULL))
    g_Log.WriteLog("Send", logs);
  else
    g_Log.WriteLog("Send Error", logs);
}