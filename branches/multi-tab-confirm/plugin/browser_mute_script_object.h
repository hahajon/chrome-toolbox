#pragma once
#include "script_object_base.h"

typedef void (*Pfn_SetBrowserMute)(BOOL flag);

class BrowserMuteScriptObject : public ScriptObjectBase {
public:
  BrowserMuteScriptObject(void);
  virtual ~BrowserMuteScriptObject(void);

  static NPObject* Allocate(NPP npp, NPClass *aClass); 

  void Deallocate();
  void Invalidate();
  bool Construct(const NPVariant *args, uint32_t argCount,
                 NPVariant *result);

  // Mute browser interface for frontend.
  bool MuteBrowser(const NPVariant *args, uint32_t argCount,
                   NPVariant *result);

  BOOL get_mute_flag() { return mute_flag_; }

private:
  // apihook.dll module hanlde.
  HMODULE api_hook_module_;
  // The function address of SetBrowseMute function in apihook.dll
  Pfn_SetBrowserMute set_browser_mute_;
  BOOL mute_flag_;

};
