#include "stdafx.h"
#include "convenience_script_object.h"
#include "log.h"
#include "convenience_plugin.h"

extern Log g_Log;
extern const TCHAR* kChromeClassName;
extern DWORD g_ChromeMainThread;
extern Local_Message_Item g_Local_Message;
bool g_DBClickCloseTab = false;

ConvenienceScriptObject::ConvenienceScriptObject(void) {
  shortcuts_list_ = NULL;
  shortcuts_object_ = NULL;
  is_listened_ = false;
}

ConvenienceScriptObject::~ConvenienceScriptObject(void) {
}

NPObject* ConvenienceScriptObject::Allocate(NPP npp, NPClass *aClass) {
  ConvenienceScriptObject* pRet = new ConvenienceScriptObject;
  char szLog[256];
  sprintf(szLog, "CConvenienceScriptObject this=%ld", pRet);
  g_Log.WriteLog("Allocate", szLog);
  if (pRet != NULL) {
    pRet->SetPlugin((PluginBase*)npp->pdata);
    Function_Item item;
    strcpy(item.function_name, "UpdateShortCutList");
    item.function_pointer = ON_INVOKEHELPER(&ConvenienceScriptObject::
        UpdateShortCutList);
    pRet->AddFunction(item);
    strcpy(item.function_name, "TriggerChromeShortcuts");
    item.function_pointer = ON_INVOKEHELPER(&ConvenienceScriptObject::
        TriggerChromeShortcuts);
    pRet->AddFunction(item);    
    strcpy(item.function_name, "PressBossKey");
    item.function_pointer = ON_INVOKEHELPER(&ConvenienceScriptObject::
        PressBossKey);
    pRet->AddFunction(item);
    strcpy(item.function_name, "SetDBClickCloseTab");
    item.function_pointer = ON_INVOKEHELPER(&ConvenienceScriptObject::
        SetDBClickCloseTab);
    pRet->AddFunction(item);  
    strcpy(item.function_name, "AddListener");
    item.function_pointer = ON_INVOKEHELPER(&ConvenienceScriptObject::
        AddListener);
    pRet->AddFunction(item);    
    strcpy(item.function_name, "RemoveListener");
    item.function_pointer = ON_INVOKEHELPER(&ConvenienceScriptObject::
        RemoveListener);
    pRet->AddFunction(item);    
    strcpy(item.function_name, "IsOnlyOneTab");
    item.function_pointer = ON_INVOKEHELPER(&ConvenienceScriptObject::
        IsOnlyOneTab);
    pRet->AddFunction(item);
    strcpy(item.function_name, "CloseChromePrompt");
    item.function_pointer = ON_INVOKEHELPER(&ConvenienceScriptObject::
        CloseChromePrompt);
    pRet->AddFunction(item);
  }
  return pRet;
}

void ConvenienceScriptObject::Deallocate() {
  char szLog[256];
  sprintf(szLog, "CConvenienceScriptObject this=%ld", this);
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
  ((ConveniencePlugin*)plugin_)->GetLocalMessage();
  NPN_GetValue(plugin_->get_npp(), NPNVWindowNPObject, &window);

  if (argCount != 1 || !NPVARIANT_IS_OBJECT(args[0])) {
    NPN_SetException(this, "parameter is invalid");
    return false;
  }
  if (shortcuts_object_ != NULL) 
    NPN_ReleaseObject(shortcuts_object_);

  shortcuts_object_ = NPVARIANT_TO_OBJECT(args[0]);
  NPN_RetainObject(shortcuts_object_);
  NPVariant length;

  NPIdentifier id;
  id = NPN_GetStringIdentifier("length");
  if (!id) {
    NPN_SetException(this, "object has not property of length");
    return false;
  }

  if (NPN_GetProperty(plugin_->get_npp(), shortcuts_object_, id, &length)) {
    int len = NPVARIANT_TO_INT32(length);
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
    if (shortcuts_list_ != NULL)
      delete[] shortcuts_list_;

    shortcuts_list_ = new ShortCut_Item[len];

    for (int i = 0; i < len; i++) {
      ShortCut_Item item = { 0 };
      item.index = i;
      id = NPN_GetIntIdentifier(i);
      NPN_GetProperty(plugin_->get_npp(), shortcuts_object_, id, &array_item);
      array_object = NPVARIANT_TO_OBJECT(array_item);
      id = NPN_GetStringIdentifier("shortcut");
      if (id) {
        NPN_GetProperty(plugin_->get_npp(), array_object,
                        id, &property_value);
        if (NPVARIANT_IS_STRING(property_value))
          strcpy(item.shortcuts_key,
                 NPVARIANT_TO_STRING(property_value).UTF8Characters);
      }
      id = NPN_GetStringIdentifier("operation");
      if (id) {
        NPN_GetProperty(plugin_->get_npp(), array_object,
                        id, &property_value);
        if (NPVARIANT_IS_STRING(property_value))
          strcpy(item.function,
                 NPVARIANT_TO_STRING(property_value).UTF8Characters);
      }
      id = NPN_GetStringIdentifier("type");
      if (id) {
        NPN_GetProperty(plugin_->get_npp(), array_object, id, &property_value);
        if (NPVARIANT_IS_BOOLEAN(property_value))
          item.ishotkey = NPVARIANT_TO_BOOLEAN(property_value);
      }
      item.object = array_object;
      key_map_new->insert(ShortCutPair(item.shortcuts_key, item));
      shortcuts_list_[i] = item;
    }

    ConveniencePlugin* pPlugin = (ConveniencePlugin*)plugin_;
    pPlugin->SetShortcutsToMemory(shortcuts_list_, len);
    
    ShortCutKeyMap::iterator iter;
    for (iter = key_map_old->begin(); iter != key_map_old->end(); iter++) {
      if (iter->second.ishotkey) {
        ATOM atom = GlobalFindAtomA(iter->second.shortcuts_key);
        UnregisterHotKey(plugin_->get_hwnd(), atom);
        GlobalDeleteAtom(atom);
      }
    }
    key_map_old->clear();
    static bool errorflag = false;
    for (iter = key_map_new->begin(); iter != key_map_new->end(); iter++) {
      if (iter->second.ishotkey) {
        ATOM atom = GlobalAddAtomA(iter->second.shortcuts_key);
        UINT vk = 0, modify = 0;
        GetShortCutsKey(iter->second.shortcuts_key, &modify, &vk);
        BOOL register_ret = RegisterHotKey(plugin_->get_hwnd(), 
                                           atom, modify, vk);
        if (!register_ret && !errorflag) {
          errorflag = true;
          if (MessageBox(NULL, g_Local_Message.msg_bosskey_defined, 
                         0, MB_OK) == IDOK) {
            RedefineBossKey();
          }
        } else if (register_ret) {
          errorflag = false;
        }
      }
    }
    shortcuts_used_flag_ = shortcuts_used_flag_ == 2 ? 1 : 2;
  }

  return true;
}

void ConvenienceScriptObject::GetShortCutsKey(char* shortcuts, UINT* modify,
                                              UINT* vk) {
  char* pStart = shortcuts;
  char* pEnd = pStart;
  char temp_value[10];
  int temp_key;
  *modify = 0;
  pEnd = strstr(pStart, "+");
  if (!pEnd) {
    *vk = atoi(shortcuts);
    return;
  }

  while (pEnd = strstr(pStart, "+")) {
    memcpy(temp_value, pStart, pEnd-pStart);
    temp_value[pEnd-pStart]=0;
    pStart = pEnd+1;
    temp_key = atoi(temp_value);
    switch(temp_key) {
      case VK_CONTROL:
        *modify |= MOD_CONTROL;
        break;
      case VK_SHIFT:
        *modify |= MOD_SHIFT;
        break;
      case VK_MENU:
        *modify |= MOD_ALT;
        break;
      case VK_LWIN:
      case VK_RWIN:
        *modify |= MOD_WIN;
        break;
      default:
        *vk = temp_key;
        break;
    }
  }

  strcpy(temp_value, pStart);
  temp_key = atoi(temp_value);
  switch(temp_key) {
      case VK_CONTROL:
        *modify |= MOD_CONTROL;
        break;
      case VK_SHIFT:
        *modify |= MOD_SHIFT;
        break;
      case VK_MENU:
        *modify |= MOD_ALT;
        break;
      case VK_LWIN:
      case VK_RWIN:
        *modify |= MOD_WIN;
        break;
      default:
        *vk = temp_key;
        break;
  }
}

void ConvenienceScriptObject::RedefineBossKey() {
  NPObject* window;
  NPN_GetValue(plugin_->get_npp(), NPNVWindowNPObject, &window);
  NPIdentifier id;
  id = NPN_GetStringIdentifier("redefineBossKey");
  NPVariant result;
  VOID_TO_NPVARIANT(result);
  if (id) {
    if (NPN_Invoke(plugin_->get_npp(), window, id, NULL, 0, &result))
      NPN_ReleaseVariantValue(&result);
  }
}

void ConvenienceScriptObject::TriggerEvent(const char* shortcuts) {
  g_Log.WriteLog("TriggerEvent", shortcuts);
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
    id = NPN_GetStringIdentifier("executeShortcut");
    NPVariant result;
    VOID_TO_NPVARIANT(result);
    if (id) {
      NPVariant param;
      OBJECT_TO_NPVARIANT((NPObject*)iter->second.object, param);
      NPN_Invoke(plugin_->get_npp(), window, id, &param, 1, &result);
      NPN_ReleaseVariantValue(&result);
    }
  }
}

void ConvenienceScriptObject::TriggerEvent(int index) {
  g_Log.WriteLog("TriggerEvent", "index");
  if (shortcuts_list_[index].ishotkey)
    return;

  NPObject* window;
  NPN_GetValue(plugin_->get_npp(), NPNVWindowNPObject, &window);
  NPIdentifier id;
  id = NPN_GetStringIdentifier("executeShortcut");
  NPVariant result;
  VOID_TO_NPVARIANT(result);
  if (id) {
    NPVariant param;
    OBJECT_TO_NPVARIANT((NPObject*)shortcuts_list_[index].object, param);
    NPN_Invoke(plugin_->get_npp(), window, id, &param, 1, &result);
    NPN_ReleaseVariantValue(&result);
  }
}

void ConvenienceScriptObject::TriggerChromeClose() {
  g_Log.WriteLog("Invoke", "TriggerChromeClose");

  NPObject* window;
  NPN_GetValue(plugin_->get_npp(), NPNVWindowNPObject, &window);
  NPIdentifier id;
  id = NPN_GetStringIdentifier("chromeBeforeClose");
  NPVariant result;
  VOID_TO_NPVARIANT(result);
  if (id)
    NPN_Invoke(plugin_->get_npp(), window, id, NULL, 0, &result);
  NPN_ReleaseVariantValue(&result);
}

void ConvenienceScriptObject::TriggerTabClose() {
  g_Log.WriteLog("Invoke", "TriggerTabClose");

  NPObject* window;
  NPN_GetValue(plugin_->get_npp(), NPNVWindowNPObject, &window);
  NPIdentifier id;
  id = NPN_GetStringIdentifier("beforeLastTabClose");
  NPVariant result;
  VOID_TO_NPVARIANT(result);
  if (id)
    NPN_Invoke(plugin_->get_npp(), window, id, NULL, 0, &result);
  NPN_ReleaseVariantValue(&result);
}

void ConvenienceScriptObject::TriggerCloseCurrentTab() {
  g_Log.WriteLog("Invoke", "TriggerCloseCurrentTab");

  NPObject* window;
  NPN_GetValue(plugin_->get_npp(), NPNVWindowNPObject, &window);
  NPIdentifier id;
  id = NPN_GetStringIdentifier("closeCurrentTab");
  NPVariant result;
  VOID_TO_NPVARIANT(result);
  if (id)
    NPN_Invoke(plugin_->get_npp(), window, id, NULL, 0, &result);
  NPN_ReleaseVariantValue(&result);
}

void ConvenienceScriptObject::TriggerShortcuts(UINT modify, UINT vk) {
  INPUT inputs[4] = { 0 };
  int keycount = 0;

  Sleep(100);

  if (modify & MOD_CONTROL) {
    inputs[keycount].type = INPUT_KEYBOARD;
    inputs[keycount].ki.wVk = VK_CONTROL;
    inputs[keycount].ki.wScan = MapVirtualKey(VK_CONTROL, MAPVK_VK_TO_VSC);
    inputs[keycount].ki.time = GetTickCount();
    keycount++;
  }
  if (modify & MOD_SHIFT) {
    inputs[keycount].type = INPUT_KEYBOARD;
    inputs[keycount].ki.wVk = VK_SHIFT;
    inputs[keycount].ki.wScan = MapVirtualKey(VK_SHIFT, MAPVK_VK_TO_VSC);
    inputs[keycount].ki.time = GetTickCount();
    keycount++;
  }
  if (modify & MOD_ALT) {
    inputs[keycount].type = INPUT_KEYBOARD;
    inputs[keycount].ki.wVk = VK_MENU;
    inputs[keycount].ki.wScan = MapVirtualKey(VK_MENU, MAPVK_VK_TO_VSC);
    inputs[keycount].ki.time = GetTickCount();
    keycount++;
  }
  inputs[keycount].type = INPUT_KEYBOARD;
  inputs[keycount].ki.wVk = vk;
  if ((vk >= VK_PRIOR && vk <= VK_DELETE) || (vk >= VK_LWIN && vk <= VK_APPS) ||
      vk == VK_NUMLOCK || vk == VK_DIVIDE)
    inputs[keycount].ki.dwFlags = KEYEVENTF_EXTENDEDKEY;
  inputs[keycount].ki.wScan = MapVirtualKey(vk, MAPVK_VK_TO_VSC);
  inputs[keycount].ki.time = GetTickCount();
  keycount++;
  char logs[256];
  sprintf(logs, "keycount=%d, vk=%d", keycount, vk);
  g_Log.WriteLog("SendInput start", logs);
  SendInput(keycount, inputs, sizeof(INPUT));

  for (int i = 0; i < keycount; i++) {
    inputs[i].ki.dwFlags |= KEYEVENTF_KEYUP;
  }
  SendInput(keycount, inputs, sizeof(INPUT));
  sprintf(logs, "keycount=%d", keycount);
  g_Log.WriteLog("SendInput end", logs);
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
      if (IsWindowVisible(chrome_hwnd) && 
          GetWindowThreadProcessId(chrome_hwnd, NULL) == g_ChromeMainThread) {
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
      if (IsWindowVisible(chrome_hwnd) && 
          GetWindowThreadProcessId(chrome_hwnd, NULL) == g_ChromeMainThread) {
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

bool ConvenienceScriptObject::TriggerChromeShortcuts(const NPVariant *args, 
                                                     uint32_t argCount, 
                                                     NPVariant *result) {
  BOOLEAN_TO_NPVARIANT(FALSE, *result);
  if (argCount != 1 || !NPVARIANT_IS_STRING(args[0]))
    return false;

  char* shortcuts = (char*)NPVARIANT_TO_STRING(args[0]).UTF8Characters;
  g_Log.WriteLog("Shortcuts", shortcuts);

  UINT modify, vk, keycount = 0;
  GetShortCutsKey(shortcuts, &modify, &vk);
  INPUT inputs[4] = { 0 };
  inputs[0].type = INPUT_KEYBOARD;
  inputs[0].ki.wVk = VK_ESCAPE;
  inputs[0].ki.wScan = MapVirtualKey(VK_ESCAPE, MAPVK_VK_TO_VSC);
  inputs[0].ki.time = GetTickCount();
  SendInput(1, inputs, sizeof(INPUT));
  inputs[0].ki.dwFlags = KEYEVENTF_KEYUP;
  SendInput(1, inputs, sizeof(INPUT));

  PostMessage(plugin_->get_hwnd(), WM_TRIGGER_CHROME_SHORTCUTS, modify, vk);

  BOOLEAN_TO_NPVARIANT(TRUE, *result);

  return true;
}

bool ConvenienceScriptObject::SetDBClickCloseTab(const NPVariant *args,
                                                 uint32_t argCount, 
                                                 NPVariant *result) {
  BOOLEAN_TO_NPVARIANT(FALSE, *result);
  if (argCount != 1 || !NPVARIANT_IS_BOOLEAN(args[0]))
    return true;
  
  g_DBClickCloseTab = NPVARIANT_TO_BOOLEAN(args[0]);
  ConveniencePlugin* plugin = (ConveniencePlugin*)plugin_;
  plugin->UpdateDBClick_CloseTab(g_DBClickCloseTab);
  BOOLEAN_TO_NPVARIANT(TRUE, *result);
  return true;
}

bool ConvenienceScriptObject::AddListener(const NPVariant *args,
                                          uint32_t argCount,
                                          NPVariant *result) {
  if (argCount != 1 || !NPVARIANT_IS_OBJECT(args[0]))
    return false;

  char logs[256];
  input_object_ = NPVARIANT_TO_OBJECT(args[0]);
  sprintf(logs, "input_object_=%ld", input_object_);
  g_Log.WriteLog("AddListener", logs);
  NPN_RetainObject(input_object_);
  is_listened_ = true;
  ConveniencePlugin* plugin = (ConveniencePlugin*)plugin_;
  plugin->UpdateIsListening(is_listened_);
  return true;
}

bool ConvenienceScriptObject::RemoveListener(const NPVariant *args, 
                                             uint32_t argCount, 
                                             NPVariant *result) {
  if (!is_listened_)
    return true;

  is_listened_ = false;
  NPN_ReleaseObject(input_object_);
  char logs[256];
  sprintf(logs, "input_object_=%ld", input_object_);
  g_Log.WriteLog("RemoveListener", logs);
  ConveniencePlugin* plugin = (ConveniencePlugin*)plugin_;
  plugin->UpdateIsListening(is_listened_);
  return true;
}

bool ConvenienceScriptObject::IsOnlyOneTab(const NPVariant *args, 
                                           uint32_t argCount, 
                                           NPVariant *result) {
  if (argCount != 1 || !NPVARIANT_IS_BOOLEAN(args[0]))
    return false;

  bool only_one_tab = NPVARIANT_TO_BOOLEAN(args[0]);
  ConveniencePlugin* plugin = (ConveniencePlugin*)plugin_;
  plugin->UpdateIsOnlyOneTab(only_one_tab);
  return true;
}

bool ConvenienceScriptObject::CloseChromePrompt(const NPVariant *args, 
                                                uint32_t argCount, 
                                                NPVariant *result) {
  if (argCount != 1 || !NPVARIANT_IS_BOOLEAN(args[0]))
    return false;

  bool prompt  = NPVARIANT_TO_BOOLEAN(args[0]);
  ConveniencePlugin* plugin = (ConveniencePlugin*)plugin_;
  plugin->UpdateCloseChromePromptFlag(prompt);
  return true;
}

void ConvenienceScriptObject::UpdateCloseChromePromptFlag(BOOL flag) {
  NPObject* window;
  NPN_GetValue(plugin_->get_npp(), NPNVWindowNPObject, &window);
  NPIdentifier id;
  id = NPN_GetStringIdentifier("updateCloseChromePromptFlag");
  NPVariant result;
  VOID_TO_NPVARIANT(result);
  if (id) {
    NPVariant param;
    BOOLEAN_TO_NPVARIANT(flag, param);
    NPN_Invoke(plugin_->get_npp(), window, id, &param, 1, &result);
    NPN_ReleaseVariantValue(&result);
  }
}

void ConvenienceScriptObject::OnKeyDown(bool contrl, bool alt, bool shift,
                                        WPARAM wParam, LPARAM lParam) {
  g_Log.WriteLog("msg", "OnKeyDown");
  if (is_listened_) {
    NPObject* window;
    NPN_GetValue(plugin_->get_npp(), NPNVWindowNPObject, &window);
    NPIdentifier id;
    id = NPN_GetStringIdentifier("setShortcutsToInputBox");
    NPVariant result;
    VOID_TO_NPVARIANT(result);

    string keys = "";
    if (contrl)
      keys += "17+";
    if (alt)
      keys += "18+";
    if (shift)
      keys += "16+";

    if (wParam != VK_CONTROL && wParam != VK_MENU && wParam != VK_SHIFT) {
      char key[MAX_KEY_LEN];
      _itoa(wParam, key, 10);
      keys += key;
    }

    g_Log.WriteLog("keyvalue", keys.c_str());
    NPVariant params[2];
    OBJECT_TO_NPVARIANT(input_object_, params[0]);
    STRINGZ_TO_NPVARIANT(keys.c_str(), params[1]);

    g_Log.WriteLog("msg", "NPN_Invoke Start");
    if (id)
      NPN_Invoke(plugin_->get_npp(), window, id, params, 2, &result);
    NPN_ReleaseVariantValue(&result);
    g_Log.WriteLog("msg", "NPN_Invoke End");
  }
}