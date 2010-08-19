#pragma once

#include "plugin_base.h"

typedef CPluginBase* (*ConstructorPtr)();
#define MAX_PLUGIN_TYPE_COUNT 10

class CPluginFactory
{
public:
  CPluginFactory(void);
  ~CPluginFactory(void);

  CPluginBase* NewPlugin(NPMIMEType pluginType);

private:
  struct Plugin_Type_Item {
    char szMIMEType[128];
    ConstructorPtr constructor;
  };

  Plugin_Type_Item plugin_type_list_[MAX_PLUGIN_TYPE_COUNT];

};
