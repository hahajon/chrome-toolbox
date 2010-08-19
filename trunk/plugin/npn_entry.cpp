#include "stdafx.h"
#include "npfunctions.h"

NPNetscapeFuncs* g_NpnFuncs;

void NP_LOADDS NPN_Version(int* plugin_major, int* plugin_minor,
                           int* netscape_major, int* netscape_minor) {
  *plugin_major = 1;
  *plugin_minor = 1;
  *netscape_major = g_NpnFuncs->version >> 8;
  *netscape_minor = g_NpnFuncs->version & 0x00FF;
}

NPError NP_LOADDS NPN_GetURLNotify(NPP instance, const char* url,
                                   const char* target, void* notifyData) {
  return g_NpnFuncs->geturlnotify(instance,url,target,notifyData);
}

NPError NP_LOADDS NPN_GetURL(NPP instance,const char* url,const char* target) {
  return g_NpnFuncs->geturl(instance,url,target);
}

NPError NP_LOADDS NPN_PostURLNotify(NPP instance, const char* url,
                                    const char* target, uint32_t len,
                                    const char* buf, NPBool file,
                                    void* notifyData) {
  return g_NpnFuncs->posturlnotify(instance,url,target,len,buf,file,notifyData);
}

NPError NP_LOADDS NPN_PostURL(NPP instance, const char* url,const char* target,
                              uint32_t len,const char* buf, NPBool file) {
  return g_NpnFuncs->posturl(instance,url,target,len,buf,file);
}

NPError NP_LOADDS NPN_RequestRead(NPStream* stream, NPByteRange* rangeList) {
  return g_NpnFuncs->requestread(stream,rangeList);
}

NPError NP_LOADDS NPN_NewStream(NPP instance, NPMIMEType type,
                                const char* target, NPStream** stream) {
  return g_NpnFuncs->newstream(instance,type,target,stream);
}

int32_t NP_LOADDS NPN_Write(NPP instance, NPStream* stream, int32_t len,
                            void* buffer) {
  return g_NpnFuncs->write(instance,stream,len,buffer);
}

NPError NP_LOADDS NPN_DestroyStream(NPP instance, NPStream* stream,
                                    NPReason reason) {
  return g_NpnFuncs->destroystream(instance,stream,reason);
}

void NP_LOADDS NPN_Status(NPP instance, const char* message) {
  g_NpnFuncs->status(instance,message);
}

const char* NP_LOADDS NPN_UserAgent(NPP instance) {
  return g_NpnFuncs->uagent(instance);
}

void* NP_LOADDS NPN_MemAlloc(uint32_t size) {
  return g_NpnFuncs->memalloc(size);
}

void NP_LOADDS NPN_MemFree(void* ptr) {
  return g_NpnFuncs->memfree(ptr);
}

uint32_t NP_LOADDS NPN_MemFlush(uint32_t size) {
  return g_NpnFuncs->memflush(size);
}

void NP_LOADDS NPN_ReloadPlugins(NPBool reloadPages) {
  g_NpnFuncs->reloadplugins(reloadPages);
}

NPError NP_LOADDS NPN_GetValue(NPP instance, NPNVariable variable,
                               void *value) {
  return g_NpnFuncs->getvalue(instance,variable,value);
}

NPError NP_LOADDS NPN_SetValue(NPP instance,NPPVariable variable,
                               void *value) {
  return g_NpnFuncs->setvalue(instance,variable,value);
}

void NP_LOADDS NPN_InvalidateRect(NPP instance, NPRect *invalidRect) {
  g_NpnFuncs->invalidaterect(instance,invalidRect);
}

void NP_LOADDS NPN_InvalidateRegion(NPP instance, NPRegion invalidRegion) {
  g_NpnFuncs->invalidateregion(instance,invalidRegion);
}

void NP_LOADDS NPN_ForceRedraw(NPP instance) {
  g_NpnFuncs->forceredraw(instance);
}

void NP_LOADDS NPN_PushPopupsEnabledState(NPP instance, NPBool enabled) {
  g_NpnFuncs->pushpopupsenabledstate(instance,enabled);
}

void NP_LOADDS NPN_PopPopupsEnabledState(NPP instance) {
  g_NpnFuncs->poppopupsenabledstate(instance);
}
void NP_LOADDS NPN_PluginThreadAsyncCall(NPP instance,void (*func) (void *),
                                         void *userData) {
  g_NpnFuncs->pluginthreadasynccall(instance,func,userData);
}

NPError NP_LOADDS NPN_GetValueForURL(NPP instance, NPNURLVariable variable,
                                     const char *url, char **value,
                                     uint32_t *len) {
  return g_NpnFuncs->getvalueforurl(instance,variable,url,value,len);
}

NPError NP_LOADDS NPN_SetValueForURL(NPP instance, NPNURLVariable variable,
                                     const char *url, const char *value,
                                     uint32_t len) {
  return g_NpnFuncs->setvalueforurl(instance,variable,url,value,len);
}
NPError NP_LOADDS NPN_GetAuthenticationInfo(NPP instance,
                                            const char *protocol,
                                            const char *host, int32_t port,
                                            const char *scheme,
                                            const char *realm,
                                            char **username, uint32_t *ulen,
                                            char **password,
                                            uint32_t *plen) {
  return g_NpnFuncs->getauthenticationinfo(instance,protocol,host,port,scheme,
    realm,username,ulen,password,plen);
}

NPObject *NPN_CreateObject(NPP npp, NPClass *aClass) {
  return g_NpnFuncs->createobject(npp,aClass);
}

NPObject *NPN_RetainObject(NPObject *npobj) {
  return g_NpnFuncs->retainobject(npobj);
}

void NPN_ReleaseObject(NPObject *npobj) {
  g_NpnFuncs->releaseobject(npobj);
}

bool NPN_Invoke(NPP npp, NPObject *npobj, NPIdentifier methodName,
                const NPVariant *args, uint32_t argCount, NPVariant *result) {
  return g_NpnFuncs->invoke(npp,npobj,methodName,args,argCount,result);
}

bool NPN_InvokeDefault(NPP npp, NPObject *npobj, const NPVariant *args,
                       uint32_t argCount, NPVariant *result) {
  return g_NpnFuncs->invokeDefault(npp,npobj,args,argCount,result);
}

bool NPN_Evaluate(NPP npp, NPObject *npobj, NPString *script,
                  NPVariant *result) {
  return g_NpnFuncs->evaluate(npp,npobj,script,result);
}

bool NPN_GetProperty(NPP npp, NPObject *npobj, NPIdentifier propertyName,
                     NPVariant *result) {
  return g_NpnFuncs->getproperty(npp,npobj,propertyName,result);
}

bool NPN_SetProperty(NPP npp, NPObject *npobj, NPIdentifier propertyName,
                     const NPVariant *value) {
  return g_NpnFuncs->setproperty(npp,npobj,propertyName,value);
}

bool NPN_RemoveProperty(NPP npp, NPObject *npobj, NPIdentifier propertyName) {
  return g_NpnFuncs->removeproperty(npp,npobj,propertyName);
}

bool NPN_HasProperty(NPP npp, NPObject *npobj, NPIdentifier propertyName) {
  return g_NpnFuncs->hasproperty(npp,npobj,propertyName);
}

bool NPN_HasMethod(NPP npp, NPObject *npobj, NPIdentifier methodName) {
  return g_NpnFuncs->hasmethod(npp,npobj,methodName);
}

bool NPN_Enumerate(NPP npp, NPObject *npobj, NPIdentifier **identifier,
                   uint32_t *count) {
  return g_NpnFuncs->enumerate(npp,npobj,identifier,count);
}

bool NPN_Construct(NPP npp, NPObject *npobj, const NPVariant *args,
                   uint32_t argCount, NPVariant *result) {
  return g_NpnFuncs->construct(npp,npobj,args,argCount,result);
}

void NPN_SetException(NPObject *npobj, const NPUTF8 *message) {
  g_NpnFuncs->setexception(npobj,message);
}

NPIdentifier NPN_GetStringIdentifier(const NPUTF8 *name) {
  return g_NpnFuncs->getstringidentifier(name);
}

void NPN_GetStringIdentifiers(const NPUTF8 **names, int32_t nameCount,
                              NPIdentifier *identifiers) {
  g_NpnFuncs->getstringidentifiers(names,nameCount,identifiers);
}

NPIdentifier NPN_GetIntIdentifier(int32_t intid) {
  return g_NpnFuncs->getintidentifier(intid);
}

bool NPN_IdentifierIsString(NPIdentifier identifier) {
  return g_NpnFuncs->identifierisstring(identifier);
}

NPUTF8 *NPN_UTF8FromIdentifier(NPIdentifier identifier) {
  return g_NpnFuncs->utf8fromidentifier(identifier);
}

int32_t NPN_IntFromIdentifier(NPIdentifier identifier) {
  return g_NpnFuncs->intfromidentifier(identifier);
}

void NPN_ReleaseVariantValue(NPVariant *variant) {
  g_NpnFuncs->releasevariantvalue(variant);
}