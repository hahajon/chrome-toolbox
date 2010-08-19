#pragma once

#include <windows.h>
#include <windowsx.h>

#define MAX_KEY_NUM 0xFF
#define MAX_FUNCTION_LEN  64
#define MAX_KEY_LEN 32
#define MAX_BUFFER_LEN  1024

#define WM_TRAYICON_NOTIFY  WM_USER+400

struct KeyStoke_Item {
  WORD VK;
  BOOL bIsPressed;
};

struct ShortCut_Item {
  char szShortCutKey[MAX_KEY_LEN];
  char szFunction[MAX_FUNCTION_LEN];
  BOOL IsHotKey;
  UINT Index;
};

enum Cmd_Msg_Type {
  Cmd_Update,
  Cmd_Request_Update,
  Cmd_Response_Update,
  Cmd_Event,
};

struct Cmd_Msg_Item {
  Cmd_Msg_Type Cmd;
  UINT ShortCuts_Id;
};