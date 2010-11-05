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

  //Chrome call this function to init object.
  static NPObject* Allocate(NPP npp, NPClass *aClass); 

  void Deallocate();
  void Invalidate();
  bool Construct(const NPVariant *args, uint32_t argCount,
                 NPVariant *result);

  //some interface for frontend.

  //notify npapi plugin that frontend will open a popup window to 
  //set wallpaper. this function save original wallpaper option for restore.
  bool SetWallPaper(const NPVariant *args, uint32_t argCount,
                    NPVariant *result);

  //apply selected type and picture as wallpaper.
  bool ApplyWallPaper(const NPVariant *args, uint32_t argCount,
                      NPVariant *result);
  //restore saved original wallpaper.
  bool RestoreWallPaper(const NPVariant *args, uint32_t argCount,
                        NPVariant *result);

private:
  //image object for generate picture file.
  Image* image_;
  //stream object for load image binary data frontend delivered
  IStream* stream_;
  //for save original wallpaper picture path.
  TCHAR ori_wallpaper_[MAX_PATH];
  //original wallpaper option.
  WALLPAPEROPT ori_opt_;

};
