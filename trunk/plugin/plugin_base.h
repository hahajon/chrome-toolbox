#pragma once

#include "npapi.h"

class CPluginBase
{
public:
  CPluginBase(void);
  virtual ~CPluginBase(void);

  virtual NPError Init(NPP instance,uint16_t mode, int16_t argc, char* argn[],
                       char* argv[], NPSavedData* saved);
  virtual NPError UnInit(NPSavedData** save);
  virtual NPError SetWindow(NPWindow* window);
  virtual NPError NewStream(NPMIMEType type,NPStream* stream, NPBool seekable,
                            uint16_t* stype);
  virtual NPError DestroyStream(NPStream* stream,NPReason reason);
  virtual int32_t WriteReady(NPStream* stream);
  virtual int32_t Write(NPStream* stream, int32_t offset,int32_t len,
                        void* buffer);
  virtual void StreamAsFile(NPStream* stream,const char* fname);
  virtual void Print(NPPrint* platformPrint);
  virtual int16_t HandleEvent(void* event);
  virtual void URLNotify(const char* url,NPReason reason, void* notifyData);
  virtual NPError GetValue(NPPVariable variable, void *value);
  virtual NPError SetValue(NPNVariable variable, void *value);


  NPP m_NPP;
  HWND m_Hwnd;

};
