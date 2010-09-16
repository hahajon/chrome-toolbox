#pragma once

#include "plugin_base.h"

typedef PluginBase* (*ConstructorPtr)();
#define MAX_PLUGIN_TYPE_COUNT 10

class PluginFactory
{
public:
  PluginFactory(void);
  ~PluginFactory(void);

  PluginBase* NewPlugin(NPMIMEType pluginType);

private:
  struct Plugin_Type_Item {
    char mime_type[128];
    ConstructorPtr constructor;
  };

  Plugin_Type_Item plugin_type_list_[MAX_PLUGIN_TYPE_COUNT];

};
