#include "stdafx.h"
#include "convenience_script_object.h"
#include "log.h"
#include "convenience_plugin.h"

extern Log g_Log;
extern const TCHAR* kChromeClassName;

ConvenienceScriptObject::ConvenienceScriptObject(void) {
  shortcuts_list_ = NULL;
}

ConvenienceScriptObject::~ConvenienceScriptObject(void) {
}

NPObject* ConvenienceScriptObject::Allocate(NPP npp, NPClass *aClass) {
  ConvenienceScriptObject* pRet = new ConvenienceScriptObject;
  char szLog[256];
  sprintf_s(szLog, "CConvenienceScriptObject this=%ld", pRet);
  g_Log.WriteLog("Allocate", szLog);
  if (pRet != NULL) {
    pRet->SetPlugin((PluginBase*)npp->pdata);
    Function_Item item;
    strcpy_s(item.function_name, "UpdateShortCutList");
    item.function_pointer = ON_INVOKEHELPER(&ConvenienceScriptObject::
        UpdateShortCutList);
    pRet->AddFunction(item);
    strcpy_s(item.function_name, "PressBossKey");
    item.function_pointer = ON_INVOKEHELPER(&ConvenienceScriptObject::
        PressBossKey);
    pRet->AddFunction(item);
  }
  return pRet;
}

void ConvenienceScriptObject::Deallocate() {
  char szLog[256];
  sprintf_s(szLog, "CConvenienceScriptObject this=%ld", this);
  g_Log.WriteLog("Deallocate", szLog);
  delete this;
}

void ConvenienceScriptObject::Invalidate() {

}

bool ConvenienceScriptObject::Construct(const NPVariant *args,
                                        uint32_t argCount,
                                        NPVariant *result) {
  return true;
}

bool ConvenienceScriptObject::UpdateShortCutList(const NPVariant *args,
                                                 uint32_t argCount,
                                                 NPVariant *result) {
  NPObject* window;
  NPN_GetValue(plugin_->get_npp(), NPNVWindowNPObject, &window);

  if (argCount != 1 || !NPVARIANT_IS_OBJECT(args[0])) {
    NPN_SetException(this, "parameter is invalid");
    return false;
  }

  NPObject* shortcut_list = NPVARIANT_TO_OBJECT(args[0]);
  NPVariant r;

  NPIdentifier id;
  id = NPN_GetStringIdentifier("length");
  if (!id) {
    NPN_SetException(this, "object has not property of length");
    return false;
  }

  if (NPN_GetProperty(plugin_->get_npp(), shortcut_list, id, &r)) {
    int len = NPVARIANT_TO_INT32(r);
    NPVariant array_item;
    NPObject* array_object;
    NPVariant property_value;

    ShortCutKeyMap* key_map_new;
    ShortCutKeyMap* key_map_old;
    if (shortcuts_used_flag_ == 2) {
      key_map_new = &map_one_;
      key_map_old = &map_two_;
    } else {
      key_map_new = &map_two_;
      key_map_old = &map_one_;
    }

    if (shortcuts_list_!=NULL)
      delete[] shortcuts_list_;

    shortcuts_list_ = new ShortCut_Item[len];

    for (int i=0;i<len;i++) {
      ShortCut_Item item = {0};
      item.Index = i;
      id = NPN_GetIntIdentifier(i);
      NPN_GetProperty(plugin_->get_npp(), shortcut_list, id, &array_item);
      array_object = NPVARIANT_TO_OBJECT(array_item);
      id = NPN_GetStringIdentifier("ShortCutKey");
      if (id) {
        NPN_GetProperty(plugin_->get_npp(), array_object,
                        id, &property_value);
        strcpy(item.szShortCutKey,
               NPVARIANT_TO_STRING(property_value).UTF8Characters);
      }
      id = NPN_GetStringIdentifier("Function");
      if (id) {
        NPN_GetProperty(plugin_->get_npp(), array_object,
                        id, &property_value);
        if (NPVARIANT_IS_STRING(property_value))
          strcpy(item.szFunction,
                 NPVARIANT_TO_STRING(property_value).UTF8Characters);
      }
      id = NPN_GetStringIdentifier("IsHotKey");
      if (id) {
        NPN_GetProperty(plugin_->get_npp(), array_object, id, &property_value);
        if (NPVARIANT_IS_BOOLEAN(property_value))
          item.IsHotKey = NPVARIANT_TO_BOOLEAN(property_value);
      }
      key_map_new->insert(ShortCutPair(item.szShortCutKey, item));
      shortcuts_list_[i] = item;
    }

    ConveniencePlugin* pPlugin = (ConveniencePlugin*)plugin_;
    pPlugin->SetShortcutsToMemory(shortcuts_list_,len);
    
    ShortCutKeyMap::iterator iter;
    for (iter = key_map_old->begin(); iter != key_map_old->end(); iter++) {
      if (iter->second.IsHotKey) {
        ATOM atom = GlobalFindAtomA(iter->second.szShortCutKey);
        UnregisterHotKey(plugin_->get_hwnd(), atom);
        GlobalDeleteAtom(atom);
      }
    }
    key_map_old->clear();
    for (iter = key_map_new->begin(); iter != key_map_new->end(); iter++) {
      if (iter->second.IsHotKey) {
        ATOM atom = GlobalAddAtomA(iter->second.szShortCutKey);
        UINT vk = 0,modify = 0;
        GetShortCutsKey(iter->second.szShortCutKey, modify, vk);
        if (!RegisterHotKey(plugin_->get_hwnd(), atom, modify, vk))
          MessageBox(NULL, L"RegisterHotKey Failed", L"Error", MB_OK);
      }
    }
    shortcuts_used_flag_ = shortcuts_used_flag_ == 2 ? 1 : 2;
  }

  return true;
}

void ConvenienceScriptObject::GetShortCutsKey(char* shortcuts, UINT& modify,
                                              UINT& vk) {
  char* pStart = shortcuts;
  char* pEnd = pStart;
  char temp_value[10];
  int temp_key;
  modify = 0;
  pEnd = strstr(pStart, "+");
  if (!pEnd) {
    vk = atoi(shortcuts);
    return;
  }

  while (pEnd = strstr(pStart, "+")) {
    memcpy(temp_value, pStart, pEnd-pStart);
    temp_value[pEnd-pStart]=0;
    pStart = pEnd+1;
    temp_key = atoi(temp_value);
    switch(temp_key) {
      case VK_CONTROL:
        modify |= MOD_CONTROL;
        break;
      case VK_SHIFT:
        modify |= MOD_SHIFT;
        break;
      case VK_MENU:
        modify |= MOD_ALT;
        break;
      case VK_LWIN:
      case VK_RWIN:
        modify |= MOD_WIN;
        break;
      default:
        vk = temp_key;
        break;
    }
  }

  strcpy(temp_value,pStart);
  temp_key = atoi(temp_value);
  switch(temp_key) {
      case VK_CONTROL:
        modify |= MOD_CONTROL;
        break;
      case VK_SHIFT:
        modify |= MOD_SHIFT;
        break;
      case VK_MENU:
        modify |= MOD_ALT;
        break;
      case VK_LWIN:
      case VK_RWIN:
        modify |= MOD_WIN;
        break;
      default:
        vk = temp_key;
        break;
  }
}

void ConvenienceScriptObject::TriggerEvent(const char* shortcuts) {
  g_Log.WriteLog("TriggerEvent",shortcuts);
  ShortCutKeyMap* shortcut_map;
  if (shortcuts_used_flag_ == 1)
    shortcut_map = &map_one_;
  else
    shortcut_map = &map_two_;

  ShortCutKeyMap::iterator iter = shortcut_map->find(shortcuts);
  if (iter != shortcut_map->end()) {
    NPObject* window;
    NPN_GetValue(plugin_->get_npp(), NPNVWindowNPObject, &window);
    NPIdentifier id;
    id = NPN_GetStringIdentifier(iter->second.szFunction);
    NPVariant result;
    NPN_Invoke(plugin_->get_npp(), window, id, NULL, 0, &result);
    NPN_ReleaseVariantValue(&result);
  }
}

void ConvenienceScriptObject::TriggerEvent(int index) {
  g_Log.WriteLog("TriggerEvent", "index");

  NPObject* window;
  NPN_GetValue(plugin_->get_npp(), NPNVWindowNPObject, &window);
  NPIdentifier id;
  id = NPN_GetStringIdentifier(shortcuts_list_[index].szFunction);
  NPVariant result;
  NPN_Invoke(plugin_->get_npp(), window, id, NULL, 0, &result);
  NPN_ReleaseVariantValue(&result);
}

bool ConvenienceScriptObject::PressBossKey(const NPVariant *args,
                                           uint32_t argCount,
                                           NPVariant *result) {
  static BOOL bosskey_state = FALSE;
  static vector<HWND> window_list;
  HWND chrome_hwnd;
  if (bosskey_state){
    bosskey_state = FALSE;
    chrome_hwnd = FindWindowEx(NULL, NULL, kChromeClassName, NULL);
    while(chrome_hwnd) {
      if (IsWindowVisible(chrome_hwnd)) {
        window_list.insert(window_list.begin(), chrome_hwnd);
        bosskey_state = TRUE;
      }
      chrome_hwnd = FindWindowEx(NULL, chrome_hwnd, kChromeClassName, NULL);
    }
    if (bosskey_state) {
      vector<HWND>::iterator iter;
      for (iter = window_list.begin(); iter != window_list.end(); iter++) {
        ShowWindow(*iter, SW_HIDE);
      }
      return true;
    }

    vector<HWND>::iterator iter;
    for (iter = window_list.begin(); iter != window_list.end(); iter++) {
      ShowWindow(*iter, SW_SHOW);
    }
    window_list.clear();
  } else {
    bosskey_state = TRUE;
    chrome_hwnd = FindWindowEx(NULL, NULL, kChromeClassName, NULL);
    while(chrome_hwnd) {
      if (IsWindowVisible(chrome_hwnd)) {
        window_list.insert(window_list.begin(), chrome_hwnd);
      }
      chrome_hwnd = FindWindowEx(NULL,chrome_hwnd,kChromeClassName,NULL);
    }
    vector<HWND>::iterator iter;
    for (iter = window_list.begin(); iter != window_list.end(); iter++) {
      ShowWindow(*iter, SW_HIDE);
    }
  }

  return true;
}