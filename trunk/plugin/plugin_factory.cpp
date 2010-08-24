#include "stdafx.h"
#include "plugin_factory.h"
#include "convenience_plugin.h"

PluginFactory::PluginFactory(void) {
  memset(plugin_type_list_, sizeof(plugin_type_list_), 0);
  strcpy_s(plugin_type_list_[0].mime_type, "application/x-npconvenience");
  plugin_type_list_[0].constructor = &ConveniencePlugin::CreateObject;
}

PluginFactory::~PluginFactory(void) {

}

PluginBase* PluginFactory::NewPlugin(NPMIMEType pluginType) {
  PluginBase* pPlugin = NULL;
  for(int i = 0; i < MAX_PLUGIN_TYPE_COUNT; i++) {
    if (plugin_type_list_[i].mime_type == NULL)
      break;
    else if (_stricmp(pluginType, plugin_type_list_[i].mime_type) == 0) {
      pPlugin = (*plugin_type_list_[i].constructor)();
      break;
    }
  }
  return pPlugin;
}
