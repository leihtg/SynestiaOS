//
// Created by XingfengYang on 2020/7/13.
//

#include <log.h>
#include <sys_call.h>

int sys_setup(void) {
  LogWarnning("[SysCall] syscall setup invoked.\n");
  return 0;
}

int sys_exit(void) {
  LogWarnning("[SysCall] syscall exit invoked.\n");
  return 0;
}

int sys_fork(void) {
  LogWarnning("[SysCall] syscall fork invoked.\n");
  return 0;
}

int sys_read(void) {
  LogWarnning("[SysCall] syscall read invoked.\n");
  return 0;
}

int sys_write(void) {
  LogWarnning("[SysCall] syscall write invoked.\n");
  return 0;
}

int sys_test1(int id){
  LogWarnning("[SysCall] syscall test1 invoked. %d \n",id);
  return 0;
}

void swi(uint32_t num) {
  __asm__ __volatile__("push {lr}\n\t"
                       "mov r0, %0\n\t"
                       "swi 0x0\n\t"
                       "pop {pc}\n\t" ::"r"(num));
}