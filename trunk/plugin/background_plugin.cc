#include "stdafx.h"
#include "background_plugin.h"
#include "script_object_factory.h"
#include "background_script_object.h"
#include "log.h"

extern Log g_Log;

BackgroundPlugin::BackgroundPlugin(void) {
}

BackgroundPlugin::~BackgroundPlugin(void) {
}

NPError BackgroundPlugin::Init(NPP instance, uint16_t mode, int16_t argc,
                               char *argn[], char *argv[], 
                               NPSavedData *saved) {
  instance->pdata = this;
  script_object_= NULL;
  return PluginBase::Init(instance, mode, argc, argn, argv, saved);
}

NPError BackgroundPlugin::UnInit(NPSavedData **save) {
  PluginBase::UnInit(save);
  script_object_= NULL;
  return NPERR_NO_ERROR;
}

NPError BackgroundPlugin::GetValue(NPPVariable variable, void *value) {
  switch(variable) {
    case NPPVpluginScriptableNPObject:
      if (!script_object_) {
        script_object_ = ScriptObjectFactory::CreateObject(npp_,
            BackgroundScriptObject::Allocate);
        g_Log.WriteLog("GetValue", "GetValue");
      }
      if (script_object_ != NULL) {
        *(NPObject**)value = script_object_;
      }
      else
        return NPERR_OUT_OF_MEMORY_ERROR;
      break;
    default:
      return NPERR_GENERIC_ERROR;
  }

  return NPERR_NO_ERROR;
}
