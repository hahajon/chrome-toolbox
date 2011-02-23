#pragma once
#include "script_object_base.h"
#include <map>

using namespace std;

class VideoAloneScriptObject : public ScriptObjectBase {
public:
  VideoAloneScriptObject(void);
  virtual ~VideoAloneScriptObject(void);

  static NPObject* Allocate(NPP npp, NPClass *aClass); 

  void Deallocate();
  void Invalidate();
  bool Construct(const NPVariant *args, uint32_t argCount,
                 NPVariant *result);

  bool ShowVideoAlone(const NPVariant *args, uint32_t argCount,
                      NPVariant *result);

  //get caption height, border width and height of system window
  bool GetWindowMetric(const NPVariant* args, uint32_t argCount,
                        NPVariant* result);

public:
  struct WindowID_Item{
    int parent_window_id;
    int window_id;
    int tab_id;
  };

  typedef map<HWND, WindowID_Item> WindowMap;
  typedef std::pair<HWND, WindowID_Item> WindowMapPair;

public:
  WindowMap window_list_;

};
