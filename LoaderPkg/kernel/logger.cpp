#include "logger.hpp"

#include <cstddef>
#include <cstdio>

#include "Console.hpp"

namespace {
  LogLevel log_level = kWarn;
}

extern Console* g_Console;

int Printk( const char* format, ... )
{
    va_list ap;
    int result;
    char s[1024];

    va_start( ap, format );
    result = vsprintf( s, format, ap );
    va_end( ap );

    if( g_Console ){
        g_Console->PutString( s );
    }

    return result;
}

void SetLogLevel(LogLevel level) {
  log_level = level;
}

int Log(LogLevel level, const char* format, ...) {
  if (level > log_level) {
    return 0;
  }

  va_list ap;
  int result;
  char s[1024];

  va_start(ap, format);
  result = vsprintf(s, format, ap);
  va_end(ap);

  g_Console->PutString(s);
  return result;
}
