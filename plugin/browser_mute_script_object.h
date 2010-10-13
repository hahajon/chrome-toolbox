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

  bool MuteBrowser(const NPVariant *args, uint32_t argCount,
                   NPVariant *result);

private:
  HMODULE api_hook_module_;
  Pfn_SetBrowserMute set_browser_mute_;

};
