#pragma once

#include <windows.h>
#include <GdiPlus.h>

using namespace Gdiplus;

class NCButton {
public:
  NCButton(void);
  ~NCButton(void);

  void Init(HWND parenthwnd);

  void OnPaint(HDC paintdc = NULL);
  void OnMouseOver(POINT pt);
  void OnMouseDown(POINT pt);
  void OnMouseUp(POINT pt);
  void OnMouseLeave();

private:
  enum BUTTON_STATE {
    BUTTON_STATE_INVALID = 0,
    BUTTON_STATE_NORMAL = 1,
    BUTTON_STATE_MOUSEOVER  = 2,
    BUTTON_STATE_MOUSEDOWN  = 3,
  };

  void GetButtonRect();

private:
  RECT rect_;
  HWND parent_hwnd_;
  BUTTON_STATE button_state_;
  bool is_topmost_;
  Image* normal_image_;
  Image* mouse_over_image_;
  Image* mouse_down_image_;
  Image* notip_normal_image_;
  Image* notip_mouseover_image_;
  Image* notip_mousedown_image_;
  HDC mask_dc_;
  HBITMAP mask_bitmap_;
  Graphics* grph_;
  ULONG_PTR token_;
  GdiplusStartupInput start_input_;

};