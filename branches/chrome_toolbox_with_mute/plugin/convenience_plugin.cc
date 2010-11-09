#include "stdafx.h"
#include "convenience_plugin.h"
#include "script_object_factory.h"
#include "convenience_script_object.h"
#include "log.h"
#include <vector>
#include "resource.h"

using namespace std;

extern Log g_Log;
extern HMODULE g_hMod;
extern bool g_DBClickCloseTab;
bool g_IsListening = false;
bool g_IsOnlyOneTab = false;

DWORD g_ChromeMainThread = 0;

WNDPROC ConveniencePlugin::old_proc_ = NULL;
HHOOK g_KeyboardHook = NULL;
HHOOK g_GetMsgHook = NULL;
HANDLE client_pipe_handle = INVALID_HANDLE_VALUE;
HANDLE client_thread_handle = INVALID_HANDLE_VALUE;

DWORD WINAPI Client_Thread(void* param);
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK GetMsgProc(int code, WPARAM wParam, LPARAM lParam);
void WriteToServer(Cmd_Msg_Item& item);

const TCHAR* kFileMappingName = L"Convenience_File";
const TCHAR* kChromeClassName = L"Chrome_WidgetWin_0";
const TCHAR* kChromeAddressBar = L"Chrome_AutocompleteEditView";
const TCHAR* kPipeName = L"\\\\.\\pipe\\convenience";

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
  wsprintf(file_name, L"%s\\convenience.ini", current_path);
  return GetPrivateProfileInt(L"CFG", L"PID", 0, file_name);
}

void WritePluginProcessId() {
  TCHAR current_path[MAX_PATH];
  GetCurrentDirectory(MAX_PATH, current_path);
  TCHAR file_name[MAX_PATH];
  wsprintf(file_name, L"%s\\convenience.ini", current_path);
  TCHAR value[32];
  wsprintf(value, L"%ld", GetCurrentProcessId());
  WritePrivateProfileString(L"CFG", L"PID", value, file_name);
}

void UpdateShortcutsFromMemory() {
  g_Log.WriteLog("msg", "UpdateShortcutsFromMemory");
  TCHAR filemap_name[MAX_PATH];
  wsprintf(filemap_name, L"%s_%ld", kFileMappingName, ReadPluginProcessId());
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
  wsprintf(pipe_name, L"%s_%ld", kPipeName, ReadPluginProcessId());

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
        Sleep(1000);
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
        sprintf(szLog, "recv from server,cmd=%ld",cmd.cmd);
        g_Log.WriteLog("ReadFile", szLog);
        switch(cmd.cmd) {
        case Cmd_Update_Shortcuts:
          UpdateShortcutsFromMemory();
          break;
        case Cmd_Response_Update:
          break;
        case Cmd_Update_DBClick_CloseTab:
          g_DBClickCloseTab = cmd.value.double_click_closetab;
          break;
        case Cmd_Update_Is_Listening:
          g_IsListening = cmd.value.is_listening;
          break;
        case Cmd_Update_Is_Only_One_Tab:
          g_IsOnlyOneTab = cmd.value.is_only_on_tab;
          break;
        case Cmd_ServerShutDown:
          {
            Cmd_Msg_Item item;
            item.cmd = Cmd_ClientShutDown;
            WriteToServer(item);
            CloseHandle(client_pipe_handle);
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
  wsprintf(pipe_name, L"%s_%ld", kPipeName, ReadPluginProcessId());

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
        sprintf(szLog, "recv from client,cmd=%ld", cmd.cmd);
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

  if ((msg->message == WM_LBUTTONDOWN || msg->message == WM_LBUTTONDBLCLK) 
      && wParam == PM_REMOVE && msg->wParam == MK_LBUTTON && g_IsOnlyOneTab) {
    TCHAR class_name[256];
    GetClassName(msg->hwnd, class_name, 256);
    if (wcscmp(class_name, kChromeClassName) == 0) { 
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
  
  if (msg->message == WM_LBUTTONDBLCLK && wParam == PM_REMOVE 
      && g_DBClickCloseTab) {
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
bottom=%ld,g_IsOnlyOneTab=%d",
            pt.x, pt.y, tab_client_rect.left, tab_client_rect.right,
            tab_client_rect.top, tab_client_rect.bottom, g_IsOnlyOneTab);
    g_Log.WriteLog("DBClickTab", logs);

    if (!PtInRect(&tab_client_rect, pt))
      return CallNextHookEx(g_GetMsgHook, code, wParam, lParam);

    TCHAR class_name[256];
    GetClassName(msg->hwnd, class_name, 256);
    if (wcscmp(class_name, kChromeClassName) == 0) {
      Cmd_Msg_Item item;
      if (g_IsOnlyOneTab) {
        msg->message = WM_NULL;
        item.cmd = Cmd_TabClose;
        WriteToServer(item);
        return CallNextHookEx(g_GetMsgHook, code, wParam, lParam);
      }

      item.cmd = Cmd_DBClick_CloseTab;
      WriteToServer(item);
    }
  }
  return CallNextHookEx(g_GetMsgHook, code, wParam, lParam);
}

ConveniencePlugin::ConveniencePlugin(void) {
  memory_file_handle_ = NULL;
  server_thread_handle_ = NULL;
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
   WritePluginProcessId();
   server_thread_handle_ = CreateThread(NULL, 0, Server_Thread, this, 0, NULL);
   HWND chrome_hwnd = FindWindowEx(NULL, NULL, kChromeClassName, NULL);
   if (!chrome_hwnd) {
     MessageBox(NULL, L"No find chrome browser window", L"Error", MB_OK);
     return NPERR_GENERIC_ERROR;
   } else {
     g_Log.WriteLog("Msg", "Plugin Init");
     g_ChromeMainThread = GetWindowThreadProcessId(chrome_hwnd, NULL);
     g_KeyboardHook = SetWindowsHookEx(WH_KEYBOARD, KeyboardProc, g_hMod, 
                                       g_ChromeMainThread);
     g_GetMsgHook = SetWindowsHookEx(WH_GETMESSAGE, GetMsgProc, g_hMod, 
                                     g_ChromeMainThread);
     if (!g_KeyboardHook || !g_GetMsgHook)
       return NPERR_GENERIC_ERROR;
     else
       g_Log.WriteLog("Msg", "Plugin Init Success");
   }
   return PluginBase::Init(instance, mode, argc, argn, argv, saved);
}

NPError ConveniencePlugin::UnInit(NPSavedData **save) {
  PluginBase::UnInit(save);
  scriptobject_ = NULL;

  Cmd_Msg_Item item;
  item.cmd = Cmd_ServerShutDown;
  WriteToClient(item);

  UnhookWindowsHookEx(g_KeyboardHook);
  UnhookWindowsHookEx(g_GetMsgHook);

  return NPERR_NO_ERROR;
}

NPError ConveniencePlugin::GetValue(NPPVariable variable, void *value) {
  switch(variable) {
    case NPPVpluginScriptableNPObject:
      if (scriptobject_ == NULL) {
        scriptobject_ = ScriptObjectFactory::CreateObject(npp_,
            ConvenienceScriptObject::Allocate);
        g_Log.WriteLog("GetValue", "GetValue");
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
  wsprintf(filemap_name, L"%s_%ld", kFileMappingName, ReadPluginProcessId());

  memory_file_handle_ = CreateFileMapping(NULL, NULL,
                                          PAGE_READWRITE|SEC_COMMIT,
                                          0, num, filemap_name);
  if (memory_file_handle_) {
    LPVOID p = MapViewOfFile(memory_file_handle_,
                             FILE_MAP_WRITE, 0, 0, sizeof(hwnd_));
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

void ConveniencePlugin::UpdateIsOnlyOneTab(bool is_only_one_tab) {
  Cmd_Msg_Item item;
  item.cmd = Cmd_Update_Is_Only_One_Tab;
  item.value.is_listening = is_only_one_tab;
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
      pObject->TriggerChromeClose();
      break;
    case WM_TRIGGER_CHROME_SHORTCUTS:
      pObject->TriggerShortcuts(wParam, lParam);
      break;
    case WM_TABCLOSE:
      pObject->TriggerTabClose();
      break;
    case WM_CLOSE_CURRENT_TAB:
      Sleep(50);
      pObject->TriggerCloseCurrentTab();
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

void ConveniencePlugin::WriteToClient(Cmd_Msg_Item& item) {
  char logs[256];
  sprintf(logs, "WriteToClient, cmd=%d", item.cmd);
  DWORD writelen;
  if (WriteFile(server_pipe_handle_, &item, sizeof(item), &writelen, NULL))
    g_Log.WriteLog("Send", logs);
  else
    g_Log.WriteLog("Send Error", logs);
}

void WriteToServer(Cmd_Msg_Item& item) {
  char logs[256];
  sprintf(logs, "WriteToServer, cmd=%d", item.cmd);
  DWORD writelen;
  if (WriteFile(client_pipe_handle, &item, sizeof(item), &writelen, NULL))
    g_Log.WriteLog("Send", logs);
  else
    g_Log.WriteLog("Send Error", logs);
}