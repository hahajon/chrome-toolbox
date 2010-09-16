#pragma once
#include "plugin_base.h"
#include "script_object_base.h"
#include <GdiPlus.h>

using namespace Gdiplus;

class BackgroundPlugin : public PluginBase {
public:
  BackgroundPlugin(void);
  virtual ~BackgroundPlugin(void);

  NPError Init(NPP instance, uint16_t mode, int16_t argc, char* argn[],
               char* argv[], NPSavedData* saved);
  NPError UnInit(NPSavedData** save);
  NPError GetValue(NPPVariable variable, void *value);

  static PluginBase* CreateObject() { return new BackgroundPlugin; }

private:
  ScriptObjectBase* script_object_;

};
