#pragma once

#include "npapi.h"
#include "npruntime.h"
#include "plugin_base.h"
#include <vector>

using namespace std;

#define FUNCTION_NAME_LEN 64

class ScriptObjectBase :
  public NPObject
{
public:
  ScriptObjectBase(void);
  virtual ~ScriptObjectBase(void);
  
  typedef bool (ScriptObjectBase::*InvokePtr)(const NPVariant *args, 
                                              uint32_t argCount,
                                              NPVariant *result);

  struct Function_Item {
    char function_name[FUNCTION_NAME_LEN];
    InvokePtr function_pointer;
  };

  struct Property_Item {
    char property_name[FUNCTION_NAME_LEN];
    NPVariant value;
  };

  virtual void Deallocate() = 0;
  virtual void Invalidate() = 0;
  virtual bool HasMethod(NPIdentifier name);
  virtual bool Invoke(NPIdentifier name, const NPVariant *args, 
                      uint32_t argCount, NPVariant *result);
  virtual bool InvokeDefault(const NPVariant *args, uint32_t argCount,
                             NPVariant *result);
  virtual bool HasProperty(NPIdentifier name);
  virtual bool GetProperty(NPIdentifier name, NPVariant *result);
  virtual bool SetProperty(NPIdentifier name, const NPVariant *value);
  virtual bool RemoveProperty(NPIdentifier name);
  virtual bool Enumerate(NPIdentifier **value, uint32_t *count);
  virtual bool Construct(const NPVariant *args, uint32_t argCount,
                         NPVariant *result) = 0;

  void AddProperty(Property_Item& item);
  void AddFunction(Function_Item& item);
  
  void SetPlugin(PluginBase* p);

protected:
  PluginBase* plugin_;

private:
  vector<Function_Item> function_list_;
  vector<Property_Item> property_list_;

};

#define ON_INVOKEHELPER(_funPtr) \
  static_cast<bool (ScriptObjectBase::*)(const NPVariant *,uint32_t , \
                                         NPVariant *)>(_funPtr)

