#pragma once
#include "plugin_base.h"
#include "script_object_base.h"
#include <queue>

using namespace std;

class ConveniencePlugin : public PluginBase
{
public:
  ConveniencePlugin(void);
  virtual ~ConveniencePlugin(void);

  NPError Init(NPP instance, uint16_t mode, int16_t argc, char* argn[],
               char* argv[], NPSavedData* saved);
  NPError UnInit(NPSavedData** save);
  NPError GetValue(NPPVariable variable, void *value);
  NPError SetWindow(NPWindow* window);

  static PluginBase* CreateObject() { return new ConveniencePlugin; }

  void SetShortcutsToMemory(ShortCut_Item* list, int count);
  void UpdateDBClick_CloseTab(bool double_click_closetab);
  void UpdateIsListening(bool is_listening);
  void UpdateIsOnlyOneTab(bool is_only_one_tab);
  void UpdateShortcutsFromMemory();
  void TriggerEvent(const char* shortcuts);

private:
  static LRESULT WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
  static DWORD WINAPI Server_Thread(void* param);

private:
  ScriptObjectBase* scriptobject_;
  static WNDPROC old_proc_;
  HANDLE memory_file_handle_;
  HANDLE server_thread_handle_;
  HANDLE server_pipe_handle_;
  queue<int> shortcuts_queue_;
  CRITICAL_SECTION cs_;

};