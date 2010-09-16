#pragma once

#include <stdio.h>
#include <windows.h>

class Log
{
public:
  Log(void);
  ~Log(void);

  bool OpenLog(LPCSTR header);
  bool WriteLog(LPCSTR title, LPCSTR contents);
  bool CloseLog();

private:
  FILE* file_;
  char buffer_[2048];
  SYSTEMTIME time_;
};