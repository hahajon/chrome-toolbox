#pragma once
#include "plugin_base.h"
#include "script_object_base.h"
#include <queue>

using namespace std;

class ConveniencePlugin : public PluginBase {
public:
  ConveniencePlugin(void);
  virtual ~ConveniencePlugin(void);

  NPError Init(NPP instance, uint16_t mode, int16_t argc, char* argn[],
               char* argv[], NPSavedData* saved);
  NPError UnInit(NPSavedData** save);
  NPError GetValue(NPPVariable variable, void *value);
  NPError SetWindow(NPWindow* window);

  static PluginBase* CreateObject() { return new ConveniencePlugin; }

  // Write shortcuts list to memory file.(server side)
  void SetShortcutsToMemory(ShortCut_Item* list, int count);
  // Send double click close tab to client.
  void UpdateDBClick_CloseTab(bool double_click_closetab);
  // Send current state of set shortcuts to client.
  void UpdateIsListening(bool is_listening);
  // Notify client current chrome window has or not only one tab.
  void UpdateIsOnlyOneTab(bool is_only_one_tab);
  void UpdateCloseChromePromptFlag(bool flag);
  void GetLocalMessage();

private:
  // The plugin's new window process.
  static LRESULT WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
  // Server thread, communicate with client use named pipe.
  static DWORD WINAPI Server_Thread(void* param);
  void WriteToClient(const Cmd_Msg_Item& item);
  // Get message from frontend, the index is a code correspond to the message.
  bool GetNPMessage(int index, TCHAR* msg, int msglen);

private:
  // Script object interface.
  ScriptObjectBase* scriptobject_;
  // The plugin's old window process.
  static WNDPROC old_proc_;
  // Memory file handle, memory file used for update shortcuts data.
  HANDLE memory_file_handle_;
  HANDLE server_thread_handle_;
  HANDLE server_pipe_handle_;
  // The queue for save triggered shortcuts list index.
  queue<int> shortcuts_queue_;
  // for mutli-thread access shortcuts_queue_ variable.
  CRITICAL_SECTION cs_;
  bool get_message_flag_;

};