#pragma once
#include "script_object_base.h"
#include <string> 
#include <map>

using namespace std;

class ConvenienceScriptObject : public ScriptObjectBase
{
public:
  ConvenienceScriptObject(void);
  virtual ~ConvenienceScriptObject(void);

  static NPObject* Allocate(NPP npp, NPClass *aClass); 

  void Deallocate();
  void Invalidate();
  bool Construct(const NPVariant *args, uint32_t argCount,
                 NPVariant *result);

  bool UpdateShortCutList(const NPVariant *args, uint32_t argCount,
                          NPVariant *result);

  bool PressBossKey(const NPVariant *args, uint32_t argCount,
                    NPVariant *result);

  void TriggerEvent(const char* shortcuts);
  void TriggerEvent(int index);

  typedef map<string,ShortCut_Item> ShortCutKeyMap;
  typedef pair<string,ShortCut_Item> ShortCutPair;

private:
  void GetShortCutsKey(char* shortcuts,UINT& modify,UINT& vk);
  
public:
  int shortcuts_used_flag_; //flag = 1,flag = 2
  ShortCutKeyMap map_one_;
  ShortCutKeyMap map_two_;
  ShortCut_Item* shortcuts_list_;

};
