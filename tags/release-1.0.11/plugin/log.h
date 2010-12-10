#pragma once

#include <stdio.h>
#include <windows.h>

class Log {
public:
  Log(void);
  ~Log(void);

  // Open log with header_YYYYMMDD_processid format.
  bool OpenLog(LPCSTR header);
  // Write log
  bool WriteLog(LPCSTR title, LPCSTR contents);
  // Close log.
  bool CloseLog();

private:
  // 
  FILE* file_;
  char buffer_[2048];
  SYSTEMTIME time_;
};