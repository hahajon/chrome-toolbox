#pragma once
#include "plugin_base.h"
#include "script_object_base.h"
#include <GdiPlus.h>

using namespace Gdiplus;

class VideoAlonePlugin : public PluginBase {
public:
  VideoAlonePlugin(void);
  virtual ~VideoAlonePlugin(void);

  NPError Init(NPP instance, uint16_t mode, int16_t argc, char* argn[],
               char* argv[], NPSavedData* saved);
  NPError UnInit(NPSavedData** save);
  NPError GetValue(NPPVariable variable, void *value);
  NPError SetWindow(NPWindow* window);

  static PluginBase* CreateObject() { return new VideoAlonePlugin; }

private:
  static LRESULT WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

private:
  ScriptObjectBase* script_object_;
  static WNDPROC old_proc_;
  ULONG_PTR token_;
  GdiplusStartupInput start_input_;

};
