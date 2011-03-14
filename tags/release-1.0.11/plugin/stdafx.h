#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <windowsx.h>

#define MAX_KEY_NUM 0xFF
#define MAX_FUNCTION_LEN  64
#define MAX_KEY_LEN 32
#define MAX_BUFFER_LEN  1024

#define WM_TRAYICON_NOTIFY            WM_USER+400

#define WM_CHROMEHWND                 WM_USER+100
#define WM_CHROMECLOSE                WM_USER+101
#define WM_TRIGGER_CHROME_SHORTCUTS   WM_USER+102
#define WM_TABCLOSE                   WM_USER+103
#define WM_CLOSE_CURRENT_TAB          WM_USER+104
#define WM_UPDATE_CLOSECHROME_PROMPT  WM_USER+105

#define EVENTID_FRESH   3456

#define OFFSET_LEN    97
#define VISTA_OFFSET_LEN    111
#define TIP_BUTTON_WIDTH  26
#define TIP_BUTTON_HEIGHT 18
#define CONST_FRAME_BORDER  4
#define CONST_FRAME_CAPTION_HEIGHT  40

#define MSG_BASE                      1000
#define MSG_BOSSKEY_DEFINED           MSG_BASE
#define MSG_ALWAYS_ON_TOP             MSG_BASE+1
#define MSG_CLOSECHROME_TITLE         MSG_BASE+2
#define MSG_CLOSECHROME_MESSAGE       MSG_BASE+3
#define MSG_CLOSECHROME_OK            MSG_BASE+4
#define MSG_CLOSECHROME_CANCEL        MSG_BASE+5
#define MSG_CLOSECHROME_NOALERT       MSG_BASE+6

struct KeyStoke_Item {
  WORD virual_key;
  BOOL is_pressed;
};

struct Local_Message_Item {
  TCHAR msg_bosskey_defined[256];
  TCHAR msg_always_on_top[256]; 
  TCHAR msg_closechrome_title[256]; 
  TCHAR msg_closechrome_message[256]; 
  TCHAR msg_closechrome_ok[256]; 
  TCHAR msg_closechrome_cancel[256]; 
  TCHAR msg_closechrome_noalert[256];
};

struct ShortCut_Item {
  char shortcuts_key[MAX_KEY_LEN];
  char function[MAX_FUNCTION_LEN];
  BOOL ishotkey;
  UINT index;
  void* object;
};

enum Cmd_Msg_Type {
  Cmd_Update_Shortcuts,
  Cmd_Request_Update,
  Cmd_Response_Update,
  Cmd_Update_DBClick_CloseTab,
  Cmd_Update_Is_Listening,
  Cmd_Update_Is_Only_One_Tab,
  Cmd_Update_Local_Message,
  Cmd_Update_CloseChrome_Prompt,
  Cmd_KeyDown,
  Cmd_KeyUp,
  Cmd_Event,
  Cmd_ChromeClose,
  Cmd_TabClose,
  Cmd_DBClick_CloseTab,
  Cmd_ServerShutDown,
  Cmd_ClientShutDown,
};

struct Cmd_Msg_Item {
  Cmd_Msg_Type cmd;
  union Cmd_Msg_Value{
    UINT shortcuts_Id;
    bool double_click_closetab;
    bool is_listening;
    bool is_only_on_tab;
    bool is_closechrome_prompt;
    struct KeyDown {
      WPARAM wparam;
      LPARAM lparam;
    }key_down;
  }value;
};