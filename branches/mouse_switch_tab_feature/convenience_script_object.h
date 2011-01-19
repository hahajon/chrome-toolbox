#pragma once
#include "script_object_base.h"
#include <string> 
#include <map>

using namespace std;

class ConvenienceScriptObject : public ScriptObjectBase
{
public:
  ConvenienceScriptObject(void);
  virtual ~ConvenienceScriptObject(void);

  static NPObject* Allocate(NPP npp, NPClass *aClass); 

  void Deallocate();
  void Invalidate();
  bool Construct(const NPVariant *args, uint32_t argCount,
                 NPVariant *result);

  // Some interface for frontend.

  // Update shortcuts list to memory, notify client process update from
  // memory file.
  bool UpdateShortCutList(const NPVariant *args, uint32_t argCount,
                          NPVariant *result);

  // Frontend call this function to implement boss key feature.
  bool PressBossKey(const NPVariant *args, uint32_t argCount,
                    NPVariant *result);

  // Notify plugin to trigger chrome's shortcuts.
  bool TriggerChromeShortcuts(const NPVariant *args, uint32_t argCount,
                              NPVariant *result);

  // Set state of double click close tab.
  bool SetDBClickCloseTab(const NPVariant *args, uint32_t argCount,
                          NPVariant *result);

  // Notify plugin current state is set shortcuts, and deliver
  // the input box object from frontend to plugin.
  bool AddListener(const NPVariant *args, uint32_t argCount,
                   NPVariant *result);
  // Notify plugin that set shortcuts has finished.
  bool RemoveListener(const NPVariant *args, uint32_t argCount,
                      NPVariant *result);

  // Notify plugin current chrome window has or not only one tab.
  bool UpdateTabCount(const NPVariant *args, uint32_t argCount,
                    NPVariant *result);

  // Notify plugin is prompt when chrome close with more than one tab.
  bool CloseChromePrompt(const NPVariant *args, uint32_t argCount,
                         NPVariant *result);

  bool CloseLastTab(const NPVariant *args, uint32_t argCount,
                    NPVariant *result);

  // Notify plugin a chrome window created
  bool ChromeWindowCreated(const NPVariant *args, uint32_t argCount,
                           NPVariant *result);

  // Notify plugin a chrome window removed
  bool ChromeWindowRemoved(const NPVariant *args, uint32_t argCount,
                           NPVariant *result);

  bool EnableMouseSwitchTab(const NPVariant* args, uint32_t argCount,
                            NPVariant* result);


  // For plugin object used. when plugin receive keyboard stroke key, 
  // then call this function to notify frontend some key pressed.
  void OnKeyDown(bool contrl, bool alt, bool shift, WPARAM wParam,
                 LPARAM lParam);

  void UpdateCloseChromePromptFlag(BOOL flag);

  // Notify frontend user has trigger a feature.
  void TriggerEvent(const char* shortcuts);
  // Notify frontend user has trigger a feature.
  void TriggerEvent(int index);
  // Notify frontend chrome will close.
  void TriggerChromeClose();
  // Notify frontend the last tab of current chrome window will close.
  void TriggerTabClose();
  // Notify frontend close current tab.
  void TriggerCloseCurrentTab();
  // Send shortcuts to chrome.
  void TriggerShortcuts(UINT modify, UINT vk, bool issleep = true);

  typedef map<string,ShortCut_Item> ShortCutKeyMap;
  typedef pair<string,ShortCut_Item> ShortCutPair;

  bool get_is_listened() { return is_listened_; }

private:
  // According shortcuts string to generate modify flag and virtual key code.
  void GetShortCutsKey(char* shortcuts, UINT* modify, UINT* vk);
  // Notify frontend should redefine boss key.
  void RedefineBossKey();
  
private:
  // The flag indicate current shortcut key map used.
  int shortcuts_used_flag_; //flag = 1,flag = 2
  ShortCutKeyMap map_one_;
  ShortCutKeyMap map_two_;
  // Shortcuts list store current shortcuts.
  ShortCut_Item* shortcuts_list_;
  // Shortcuts array object from frontend.
  NPObject* shortcuts_object_;
  // Indicate current state is or not set shortcuts.
  bool is_listened_;
  // Input box object.
  NPObject* input_object_;

};
