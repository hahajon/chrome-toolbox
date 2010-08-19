#include "stdafx.h"
#include "plugin_factory.h"
#include "convenience_plugin.h"

CPluginFactory::CPluginFactory(void) {
  memset(plugin_type_list_,sizeof(plugin_type_list_),0);
  strcpy_s(plugin_type_list_[0].szMIMEType,"application/x-npconvenience");
  plugin_type_list_[0].constructor = &CConveniencePlugin::CreateObject;
}

CPluginFactory::~CPluginFactory(void) {

}

CPluginBase* CPluginFactory::NewPlugin(NPMIMEType pluginType) {
  CPluginBase* pPlugin = NULL;
  for(int i=0;i<MAX_PLUGIN_TYPE_COUNT;i++) {
    if (plugin_type_list_[i].szMIMEType == NULL)
      break;
    else if (_stricmp(pluginType,plugin_type_list_[i].szMIMEType) == 0) {
      pPlugin = (*plugin_type_list_[i].constructor)();
      break;
    }
  }
  return pPlugin;
}
