#pragma once

#include "scriptobject_base.h"

class CScriptObjectFactory
{
public:
  CScriptObjectFactory(void);
  ~CScriptObjectFactory(void);

  static CScriptObjectBase* CreateObject(NPP npp, 
                                         NPAllocateFunctionPtr allocate);

  
private:
  static NPObject* allocate(NPP npp, NPClass *aClass);
  static void deallocate(NPObject *npobj);
  static void invalidate(NPObject *npobj);
  static bool hasMethod(NPObject *npobj, NPIdentifier name);
  static bool invoke(NPObject *npobj, NPIdentifier name,
                     const NPVariant *args, uint32_t argCount,
                     NPVariant *result);
  static bool invokeDefault(NPObject *npobj,
                            const NPVariant *args,
                            uint32_t argCount,
                            NPVariant *result);
  static bool hasProperty(NPObject *npobj, NPIdentifier name);
  static bool getProperty(NPObject *npobj, NPIdentifier name,
                          NPVariant *result);
  static bool setProperty(NPObject *npobj, NPIdentifier name,
                          const NPVariant *value);
  static bool removeProperty(NPObject *npobj,NPIdentifier name);
  static bool enumerate(NPObject *npobj, NPIdentifier **value,
                        uint32_t *count);
  static bool construct(NPObject *npobj,const NPVariant *args,
                        uint32_t argCount,NPVariant *result);

private:
  static NPClass npclass_;

};
