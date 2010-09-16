#pragma once
#include "script_object_base.h"
#include <comdef.h>
#include <GdiPlus.h>
#include <WinInet.h>
#include <shlobj.h>

using namespace Gdiplus;

class BackgroundScriptObject : public ScriptObjectBase {
public:
  BackgroundScriptObject(void);
  virtual ~BackgroundScriptObject(void);

  static NPObject* Allocate(NPP npp, NPClass *aClass); 

  void Deallocate();
  void Invalidate();
  bool Construct(const NPVariant *args, uint32_t argCount,
                 NPVariant *result);

  bool SetWallPaper(const NPVariant *args, uint32_t argCount,
                    NPVariant *result);

  bool ApplyWallPaper(const NPVariant *args, uint32_t argCount,
                      NPVariant *result);
  bool RestoreWallPaper(const NPVariant *args, uint32_t argCount,
                        NPVariant *result);

private:
  Image* image_;
  IStream* stream_;
  TCHAR ori_wallpaper_[MAX_PATH];
  WALLPAPEROPT ori_opt_;

};
