#include "stdafx.h"
#include "convenience_plugin.h"
#include "scriptobject_factory.h"
#include "convenience_scriptobject.h"
#include "log.h"
#include <vector>
#include "resource.h"

using namespace std;

extern CLog gLog;
extern HMODULE g_hMod;

WNDPROC CConveniencePlugin::old_proc_ = NULL;
HHOOK gHook = NULL;
HANDLE client_pipe_handle = NULL;

const TCHAR* kFileMappingName = L"Convenience_File";
const TCHAR* kChromeClassName = L"Chrome_WidgetWin_0";
const TCHAR* kPipeName = L"\\\\.\\pipe\\convenience";

int map_current_used_flag = 2;
CConvenienceScriptObject::ShortCutKeyMap g_mapOne;
CConvenienceScriptObject::ShortCutKeyMap g_mapTwo;

KeyStoke_Item gKeyList[MAX_KEY_NUM];

void InitKeyList() {
  for (int i=0;i<MAX_KEY_NUM;i++) {
    gKeyList[i].VK = i;
    gKeyList[i].bIsPressed = FALSE;
  }
}

void UpdateShortcutsFromMemory() {
  gLog.WriteLog("msg","UpdateShortcutsFromMemory");
  HANDLE memory_file_handle = OpenFileMapping(FILE_MAP_READ,FALSE,
    kFileMappingName);
  if (memory_file_handle) {
    LPVOID p = MapViewOfFile(memory_file_handle,FILE_MAP_READ,0,0,0);
    if (p) {
      int count = 0;
      memcpy(&count,p,sizeof(int));
      ShortCut_Item* list = (ShortCut_Item*)((BYTE*)p+sizeof(int));
      CConvenienceScriptObject::ShortCutKeyMap* key_map_new;
      CConvenienceScriptObject::ShortCutKeyMap* key_map_old;
      if (map_current_used_flag == 1) {
        key_map_new = &g_mapTwo;
        key_map_old = &g_mapOne;
      } else {
        key_map_new = &g_mapOne;
        key_map_old = &g_mapTwo;
      }

      for (int i=0;i<count;i++) {
        key_map_new->insert(make_pair(list[i].szShortCutKey,list[i]));
      }
      map_current_used_flag = map_current_used_flag==1 ? 2 : 1;
      key_map_old->clear();
      UnmapViewOfFile(p);
    }
    CloseHandle(memory_file_handle);
  }
}

DWORD WINAPI Client_Thread(void* param) {
  char szLog[256];
  char buffer[MAX_BUFFER_LEN];
  DWORD outlen;
  int offset=0;
  int readlen = sizeof(Cmd_Msg_Item);
  DWORD writelen;
  Cmd_Msg_Item cmd;

  HANDLE event_handle = (HANDLE)param;

  while(true) {
    client_pipe_handle = CreateFile(kPipeName,GENERIC_READ|GENERIC_WRITE,
                                    0,NULL,OPEN_EXISTING,FILE_FLAG_OVERLAPPED,NULL);

    if (client_pipe_handle == INVALID_HANDLE_VALUE) {
      sprintf_s(szLog,"CreateFile Failed,GetLastError=%ld",GetLastError());
      gLog.WriteLog("Error",szLog);
      return -1;
    } else {
      gLog.WriteLog("Msg","start client thread");
    }

    cmd.Cmd = Cmd_Request_Update;
    if (WriteFile(client_pipe_handle,&cmd,sizeof(cmd),&writelen,NULL))
      gLog.WriteLog("msg","write to server");
    else
      gLog.WriteLog("msg","write to server failed");

    while (ReadFile(client_pipe_handle,buffer+offset,readlen,
                    &outlen,NULL)) {
        if (outlen==readlen) {
          memcpy(&cmd,buffer,sizeof(cmd));
          switch(cmd.Cmd) {
          case Cmd_Update:
            UpdateShortcutsFromMemory();
            SetEvent(event_handle);
            break;
          case Cmd_Response_Update:
            SetEvent(event_handle);
            break;
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
    gLog.WriteLog("error","client thread readfile failed");
    CloseHandle(client_pipe_handle);
    client_pipe_handle = NULL;
  }
  return 0;
}

DWORD CConveniencePlugin::Server_Thread(void* param) {
  char szLog[256];
  char buffer[MAX_BUFFER_LEN];
  DWORD outlen;
  int offset=0;
  int readlen = sizeof(Cmd_Msg_Item);
  Cmd_Msg_Item cmd;

  CConveniencePlugin* pPlugin = (CConveniencePlugin*)param;

  pPlugin->server_pipe_handle_ = CreateNamedPipe(kPipeName,
    PIPE_ACCESS_DUPLEX|FILE_FLAG_OVERLAPPED,
    PIPE_TYPE_BYTE|PIPE_READMODE_BYTE|PIPE_WAIT,
    PIPE_UNLIMITED_INSTANCES,
    MAX_BUFFER_LEN,MAX_BUFFER_LEN,0,NULL);
  if (pPlugin->server_pipe_handle_ == INVALID_HANDLE_VALUE) {
    sprintf_s(szLog,"CreateNamedPipe Failed,GetLastError=%ld",GetLastError());
    gLog.WriteLog("Error",szLog);
    return -1;
  }

  while (true) {
    DisconnectNamedPipe(pPlugin->server_pipe_handle_);
    if (!ConnectNamedPipe(pPlugin->server_pipe_handle_,NULL))
      continue;

    while (ReadFile(pPlugin->server_pipe_handle_,buffer+offset,readlen,
      &outlen,NULL)) {
        if (outlen==readlen) {
          memcpy(&cmd,buffer,sizeof(cmd));
          switch(cmd.Cmd) {
          case Cmd_Request_Update:
            if (pPlugin->memory_file_handle_) {
              Cmd_Msg_Item item;
              DWORD writelen;
              item.Cmd = Cmd_Update;
              WriteFile(pPlugin->server_pipe_handle_,&item,sizeof(item),
                &writelen,NULL);
            } else {
              Cmd_Msg_Item item;
              DWORD writelen;
              item.Cmd = Cmd_Response_Update;
              WriteFile(pPlugin->server_pipe_handle_,&item,sizeof(item),
                &writelen,NULL);
            }
            break;
          case Cmd_Event:
            gLog.WriteLog("event","recv from client");
            EnterCriticalSection(&pPlugin->cs_);
            pPlugin->shortcuts_queue_.push(cmd.ShortCuts_Id);
            LeaveCriticalSection(&pPlugin->cs_);
            break;
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
  }
  return 0;
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
  static HANDLE client_thread_handle = NULL;

  if (client_thread_handle == NULL) {
    HANDLE hevent = CreateEvent(NULL,FALSE,FALSE,NULL);
    client_thread_handle = CreateThread(NULL,0,Client_Thread,hevent,0,NULL);
    WaitForSingleObject(hevent,INFINITE);
    CloseHandle(hevent);
    CloseHandle(client_thread_handle);
  }

  if (!(HIWORD(lParam)&KF_REPEAT)) {
    gKeyList[wParam].bIsPressed = TRUE;
    string shortcuts;
    char virual_key[MAX_KEY_LEN];
    if (gKeyList[VK_CONTROL].bIsPressed)
      shortcuts = "17";
    if (gKeyList[VK_MENU].bIsPressed) {
      if (shortcuts.length() > 0)
        shortcuts+="+18";
      else
        shortcuts="18";
    }
    if (gKeyList[VK_SHIFT].bIsPressed) {
      if (shortcuts.length() > 0)
        shortcuts+="+16";
      else
        shortcuts="16";
    }
    itoa(wParam,virual_key,10);
    if (shortcuts.length() > 0) {
      shortcuts+= "+";
      shortcuts += virual_key;
    }
    else
      shortcuts = virual_key;

    CConvenienceScriptObject::ShortCutKeyMap* shortcut_map;
    if (map_current_used_flag==1)
      shortcut_map = &g_mapOne;
    else
      shortcut_map = &g_mapTwo;

    CConvenienceScriptObject::ShortCutKeyMap::iterator iter = shortcut_map->find(shortcuts);
    if (iter != shortcut_map->end() && client_pipe_handle) {
      Cmd_Msg_Item item;
      DWORD writelen;
      item.Cmd = Cmd_Event;
      item.ShortCuts_Id = iter->second.Index;
      gLog.WriteLog("msg","before writefile");
      if (WriteFile(client_pipe_handle,&item,sizeof(item),&writelen,NULL)) {
        gLog.WriteLog("msg","write msg to server");
      } else {
        gLog.WriteLog("error","write msg to server failed");
      }
    }
  }
  else if (HIWORD(lParam)&KF_UP) {
    gKeyList[wParam].bIsPressed = FALSE;
  }

  return CallNextHookEx(gHook,nCode,wParam,lParam);
}

CConveniencePlugin::CConveniencePlugin(void) {
  memory_file_handle_ = NULL;
  server_thread_handle_ = NULL;
  InitializeCriticalSection(&cs_);
}

CConveniencePlugin::~CConveniencePlugin(void) {
  DeleteCriticalSection(&cs_);
}

NPError CConveniencePlugin::Init(NPP instance, uint16_t mode, int16_t argc,
                                 char *argn[], char *argv[], 
                                 NPSavedData *saved) {
   scriptobject_ = NULL;
   instance->pdata = this;
   server_thread_handle_ = CreateThread(NULL,0,Server_Thread,this,0,NULL);
   InitKeyList();
   HWND chrome_hwnd = FindWindowEx(NULL, NULL, kChromeClassName,NULL);
   if (!chrome_hwnd) {
     MessageBox(NULL,L"No find chrome browser window",L"Error",MB_OK);
     return NPERR_GENERIC_ERROR;
   } else {
     DWORD thread_id = GetWindowThreadProcessId(chrome_hwnd,NULL);
     gHook = SetWindowsHookEx(WH_KEYBOARD,KeyboardProc,g_hMod,thread_id);
     if (!gHook)
       return NPERR_GENERIC_ERROR;
   }
   return CPluginBase::Init(instance,mode,argc,argn,argv,saved);
}

NPError CConveniencePlugin::UnInit(NPSavedData **save) {
  CPluginBase::UnInit(save);
  scriptobject_ = NULL;
  if (gHook != NULL) {
    if (UnhookWindowsHookEx(gHook))
      gHook = NULL;
  }
  if (memory_file_handle_)
    CloseHandle(memory_file_handle_);

  return NPERR_NO_ERROR;
}

NPError CConveniencePlugin::GetValue(NPPVariable variable, void *value) {
  switch(variable) {
    case NPPVpluginScriptableNPObject:
      if (scriptobject_ == NULL) {
        scriptobject_ = CScriptObjectFactory::CreateObject(m_NPP,CConvenienceScriptObject::Allocate);
        gLog.WriteLog("GetValue","GetValue");
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

NPError CConveniencePlugin::SetWindow(NPWindow* window) {
  CPluginBase::SetWindow(window);

  if (m_Hwnd == NULL && old_proc_ != NULL) {
    KillTimer(m_Hwnd,1);
    SubclassWindow(m_Hwnd,old_proc_);
    old_proc_ = NULL; 
  }

  if (m_Hwnd != NULL && old_proc_ == NULL) {
    old_proc_ = SubclassWindow(m_Hwnd,WndProc);
    SetWindowLong(m_Hwnd,GWLP_USERDATA,(LONG)this);
    SetTimer(m_Hwnd,1,10,NULL);
  }

  return NPERR_NO_ERROR;
}

void CConveniencePlugin::SetShortcutsToMemory(ShortCut_Item* list,int count) {
  if (!memory_file_handle_)
    CloseHandle(memory_file_handle_);

  int num = sizeof(ShortCut_Item)*count+sizeof(int);

  memory_file_handle_ = CreateFileMapping(NULL,NULL,
                                          PAGE_READWRITE|SEC_COMMIT,
                                          0,num,kFileMappingName);
  if (memory_file_handle_) {
    LPVOID p = MapViewOfFile(memory_file_handle_,
                             FILE_MAP_WRITE,0,0,sizeof(m_Hwnd));
    if (p) {
      memcpy(p,&count,sizeof(int));
      memcpy((BYTE*)p+sizeof(int),list,sizeof(ShortCut_Item)*count);
      UnmapViewOfFile(p);
      Cmd_Msg_Item item;
      item.Cmd = Cmd_Update;
      DWORD writelen;
      WriteFile(server_pipe_handle_,&item,sizeof(item),&writelen,NULL);
    }
  } else {
    char szLog[256];
    sprintf(szLog,"GetLastError=%ld",GetLastError());
    gLog.WriteLog("Error",szLog);
  }
}

LRESULT CConveniencePlugin::WndProc(HWND hWnd, UINT Msg, 
                                    WPARAM wParam, LPARAM lParam) {
  CConveniencePlugin* plugin = (CConveniencePlugin*)GetWindowLong(hWnd,GWLP_USERDATA);
  CConvenienceScriptObject* pObject = (CConvenienceScriptObject*)plugin->scriptobject_;
  if (!pObject)
    return CallWindowProc(old_proc_,hWnd,Msg,wParam,lParam);

  switch(Msg) {
    case WM_TIMER:
      EnterCriticalSection(&plugin->cs_);
      while (!plugin->shortcuts_queue_.empty()) {
        gLog.WriteLog("WndProc","WM_TIMER");
        int shortcut_id = plugin->shortcuts_queue_.front();
        plugin->shortcuts_queue_.pop();
        pObject->TriggerEvent(shortcut_id);
      }
      LeaveCriticalSection(&plugin->cs_);
      break;
    case WM_HOTKEY:
      {
        char shortcuts_name[MAX_KEY_LEN];
        GlobalGetAtomNameA(wParam,shortcuts_name,MAX_KEY_LEN);
        pObject->TriggerEvent(shortcuts_name);
      }
      break;
    default:
      return CallWindowProc(old_proc_,hWnd,Msg,wParam,lParam);
  }
  return TRUE;
}