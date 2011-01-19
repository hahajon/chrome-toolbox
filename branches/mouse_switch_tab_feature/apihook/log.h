#pragma once

#include <stdio.h>
#include <windows.h>

class Log {
public:
  Log(void);
  ~Log(void);

  bool OpenLog(const char* header);
  bool WriteLog(const char* title, const char* contents);
  bool CloseLog();

private:
  FILE* file_;
  SYSTEMTIME time_;
};