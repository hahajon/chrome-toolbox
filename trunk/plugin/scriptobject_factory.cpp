#include "stdafx.h"
#include "scriptobject_factory.h"

NPClass CScriptObjectFactory::npclass_ = {
  NP_CLASS_STRUCT_VERSION,
  CScriptObjectFactory::allocate,
  CScriptObjectFactory::deallocate,
  CScriptObjectFactory::invalidate,
  CScriptObjectFactory::hasMethod,
  CScriptObjectFactory::invoke,
  CScriptObjectFactory::invokeDefault,
  CScriptObjectFactory::hasProperty,
  CScriptObjectFactory::getProperty,
  CScriptObjectFactory::setProperty,
  CScriptObjectFactory::removeProperty,
  CScriptObjectFactory::enumerate,
  CScriptObjectFactory::construct
};

CScriptObjectFactory::CScriptObjectFactory(void) {
}

CScriptObjectFactory::~CScriptObjectFactory(void) {
}

CScriptObjectBase* CScriptObjectFactory::CreateObject(NPP npp,
                                             NPAllocateFunctionPtr allocate) {
  npclass_.allocate = allocate;
  return (CScriptObjectBase*)NPN_CreateObject(npp,&npclass_);
}

NPObject* CScriptObjectFactory::allocate(NPP npp, NPClass *aClass) {
  return NULL;
}

void CScriptObjectFactory::deallocate(NPObject *npobj) {
  CScriptObjectBase* pObject = (CScriptObjectBase*)npobj;
  pObject->Deallocate();
}

void CScriptObjectFactory::invalidate(NPObject *npobj) {
  CScriptObjectBase* pObject = (CScriptObjectBase*)npobj;
  pObject->Invalidate();
}

bool CScriptObjectFactory::hasMethod(NPObject *npobj, NPIdentifier name) {
  CScriptObjectBase* pObject = (CScriptObjectBase*)npobj;
  return pObject->HasMethod(name);
}

bool CScriptObjectFactory::invoke(NPObject *npobj, NPIdentifier name,
                                  const NPVariant *args, uint32_t argCount,
                                  NPVariant *result) {
  CScriptObjectBase* pObject = (CScriptObjectBase*)npobj;
  return pObject->Invoke(name,args,argCount,result);
}

bool CScriptObjectFactory::invokeDefault(NPObject *npobj,
                                         const NPVariant *args,
                                         uint32_t argCount,
                                         NPVariant *result) {
  CScriptObjectBase* pObject = (CScriptObjectBase*)npobj;
  return pObject->InvokeDefault(args,argCount,result);
}

bool CScriptObjectFactory::hasProperty(NPObject *npobj, NPIdentifier name) {
  CScriptObjectBase* pObject = (CScriptObjectBase*)npobj;
  return pObject->HasProperty(name);
}

bool CScriptObjectFactory::getProperty(NPObject *npobj, NPIdentifier name,
                                       NPVariant *result) {
  CScriptObjectBase* pObject = (CScriptObjectBase*)npobj;
  return pObject->GetProperty(name,result);
}

bool CScriptObjectFactory::setProperty(NPObject *npobj, NPIdentifier name,
                                       const NPVariant *value) {
  CScriptObjectBase* pObject = (CScriptObjectBase*)npobj;
  return pObject->SetProperty(name,value);
}

bool CScriptObjectFactory::removeProperty(NPObject *npobj,NPIdentifier name) {
  CScriptObjectBase* pObject = (CScriptObjectBase*)npobj;
  return pObject->RemoveProperty(name);
}

bool CScriptObjectFactory::enumerate(NPObject *npobj, NPIdentifier **value,
                                     uint32_t *count) {
  CScriptObjectBase* pObject = (CScriptObjectBase*)npobj;
  return pObject->Enumerate(value,count);
}

bool CScriptObjectFactory::construct(NPObject *npobj,const NPVariant *args,
                                     uint32_t argCount,NPVariant *result) {
  CScriptObjectBase* pObject = (CScriptObjectBase*)npobj;
  return pObject->Construct(args,argCount,result);
}