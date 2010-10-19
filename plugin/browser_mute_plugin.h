#pragma once
#include "plugin_base.h"
#include "script_object_base.h"

class BrowserMutePlugin : public PluginBase {
public:
  BrowserMutePlugin(void);
  virtual ~BrowserMutePlugin(void);

  NPError Init(NPP instance, uint16_t mode, int16_t argc, char* argn[],
               char* argv[], NPSavedData* saved);
  NPError UnInit(NPSavedData** save);
  NPError GetValue(NPPVariable variable, void *value);

  static PluginBase* CreateObject() { return new BrowserMutePlugin; }

private:
  ScriptObjectBase* script_object_;

};
