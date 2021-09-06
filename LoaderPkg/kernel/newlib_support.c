#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>

#include "Global.hpp"

void __attribute__((noreturn)) _exit( int exitcode ) {
  while (1) __asm__("hlt");
}

caddr_t sbrk(int incr) {
  if( g_ProgramBreak == 0 || g_ProgramBreak + incr >= g_ProgramBreakEnd ){
      errno = ENOMEM;
      return (caddr_t)-1;
  }

  caddr_t prev_break = g_ProgramBreak;
  g_ProgramBreak += incr;

  return prev_break;
}

int getpid(void) {
  return 1;
}

int kill(int pid, int sig) {
  errno = EINVAL;
  return -1;
}
