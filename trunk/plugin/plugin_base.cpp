#include "stdafx.h"
#include "plugin_base.h"

CPluginBase::CPluginBase(void) {
}

CPluginBase::~CPluginBase(void) {
}

NPError CPluginBase::Init(NPP instance, uint16_t mode, int16_t argc,
                          char *argn[], char *argv[], NPSavedData *saved) {
  m_NPP = instance;
  return NPERR_NO_ERROR;
}

NPError CPluginBase::UnInit(NPSavedData** save) {
  return NPERR_NO_ERROR;
}

NPError CPluginBase::SetWindow(NPWindow* window) {
  m_Hwnd = (HWND)window->window;
  return NPERR_NO_ERROR;
}

NPError CPluginBase::NewStream(NPMIMEType type,NPStream* stream,
                               NPBool seekable,uint16_t* stype) {
  return NPERR_NO_ERROR;
}

NPError CPluginBase::DestroyStream(NPStream* stream,NPReason reason) {
  return NPERR_NO_ERROR;
}

int32_t CPluginBase::WriteReady(NPStream* stream) {
  return 0;
}

int32_t CPluginBase::Write(NPStream* stream, int32_t offset,int32_t len,
                           void* buffer) {
  return 0;
}

void CPluginBase::StreamAsFile(NPStream* stream,const char* fname) {
  
}

void CPluginBase::Print(NPPrint* platformPrint) {
}

int16_t CPluginBase::HandleEvent(void* event) {
  return 0;
}

void CPluginBase::URLNotify(const char* url,NPReason reason, void* notifyData) {
  
}

NPError CPluginBase::GetValue(NPPVariable variable, void *value) {
  return NPERR_NO_ERROR;
}

NPError CPluginBase::SetValue(NPNVariable variable, void *value) {
  return NPERR_NO_ERROR;
}

