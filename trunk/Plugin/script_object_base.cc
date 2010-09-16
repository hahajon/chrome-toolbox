#include "stdafx.h"
#include "script_object_base.h"

ScriptObjectBase::ScriptObjectBase(void) {
}

ScriptObjectBase::~ScriptObjectBase(void) {
}

void ScriptObjectBase::SetPlugin(PluginBase* p) {
  plugin_ = p;
}

bool ScriptObjectBase::HasMethod(NPIdentifier name) {
  bool bRet = false;
  vector<Function_Item>::iterator iter;
  char* szName = NPN_UTF8FromIdentifier(name);
  for (iter = function_list_.begin(); iter != function_list_.end(); iter++) {
    if (strcmp((const char*)iter->function_name, szName) == 0) {
      bRet = true;
      break;
    }
  }
  if (szName != NULL)
    NPN_MemFree(szName);
  return bRet;
}

bool ScriptObjectBase::Invoke(NPIdentifier name,const NPVariant *args, 
                              uint32_t argCount,NPVariant *result) {
  bool bRet = false;
  vector<Function_Item>::iterator iter;
  char* szName = NPN_UTF8FromIdentifier(name);
  for (iter = function_list_.begin(); iter != function_list_.end(); iter++) {
   if (strcmp((const char*)iter->function_name, szName) == 0) {
     bRet = (this->*(iter->function_pointer))(args, argCount, result);
     break;
   }
  }
  if (szName != NULL)
   NPN_MemFree(szName);
  return bRet;
}

bool ScriptObjectBase::InvokeDefault(const NPVariant *args,uint32_t argCount,
                                     NPVariant *result) {
  return false;
}

bool ScriptObjectBase::HasProperty(NPIdentifier name) {
  bool bRet = false;
  vector<Property_Item>::iterator iter;
  char* szName = NPN_UTF8FromIdentifier(name);
  for (iter = property_list_.begin(); iter != property_list_.end(); iter++) {
    if (strcmp((const char*)iter->property_name, szName) == 0) {
      bRet = true;
      break;
    }
  }
  if (szName != NULL)
    NPN_MemFree(szName);
  return bRet;
}

bool ScriptObjectBase::GetProperty(NPIdentifier name, NPVariant *result) {
  bool bRet = false;
  vector<Property_Item>::iterator iter;
  char* szName = NPN_UTF8FromIdentifier(name);
  for (iter = property_list_.begin(); iter != property_list_.end(); iter++) {
    if (strcmp((const char*)iter->property_name, szName) == 0) {
      bRet = true;
      *result = iter->value;
      break;
    }
  }
  if (szName != NULL)
    NPN_MemFree(szName);
  return bRet;
}

bool ScriptObjectBase::SetProperty(NPIdentifier name,
                                   const NPVariant *value) {
  bool bRet = false;
  vector<Property_Item>::iterator iter;
  char* szName = NPN_UTF8FromIdentifier(name);
  for (iter = property_list_.begin(); iter != property_list_.end(); iter++) {
    if (strcmp((const char*)iter->property_name, szName) == 0) {
      bRet = true;
      iter->value = *value;
      break;
    }
  }
  if (szName != NULL)
    NPN_MemFree(szName);
  return bRet;
}
bool ScriptObjectBase::RemoveProperty(NPIdentifier name) {
  bool bRet = false;
  vector<Property_Item>::iterator iter;
  char* szName = NPN_UTF8FromIdentifier(name);
  for (iter = property_list_.begin(); iter != property_list_.end(); iter++) {
    if (strcmp((const char*)iter->property_name, szName) == 0) {
      bRet = true;
      property_list_.erase(iter);
      break;
    }
  }
  if (szName != NULL)
    NPN_MemFree(szName);
  return bRet;
}
bool ScriptObjectBase::Enumerate(NPIdentifier **value, uint32_t *count) {
//  *count = m_PropList.size() + m_FunList.size();
  return true;
}

void ScriptObjectBase::AddProperty(Property_Item& item) {
  property_list_.push_back(item);
}

void ScriptObjectBase::AddFunction(Function_Item& item) {
  function_list_.push_back(item);
}