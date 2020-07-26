//
// Created by XingfengYang on 2020/7/13.
//
#ifndef __KERNEL_SYSCALL_H__
#define __KERNEL_SYSCALL_H__
#include <stdint.h>
#include <type.h>


// Below would move to libC
void swi(uint32_t num);

#define asmlinkage __attribute__((regparm(0)))

#define __SYS_write 4

#define _syscall0(type,name)  \
asmlinkage type name(void){     \
    __asm__ __volatile__("push {lr}\n\t"    \
                       "mov r0, %0\n\t"     \
                       "swi 0x0\n\t"        \
                       "pop {pc}\n\t" ::"r"(__SYSCALL_##name));  \
}

#define _syscall1(type,name,type1,name1)  \
asmlinkage type name(type1 name1){     \
    __asm__ __volatile__("push {lr}\n\t"    \
                       "mov r0, %0\n\t"     \
                       "mov r1, %1\n\t"     \
                       "swi 0x0\n\t"        \
                       "pop {pc}\n\t" ::"r"(__SYSCALL_##name),"r"(name1));  \
}

#define _syscall2(type,name,type1,name1,type2,name2)  \
asmlinkage type name(type1 name1){     \
    __asm__ __volatile__("push {lr}\n\t"    \
                       "mov r0, %0\n\t"     \
                       "mov r1, %1\n\t"     \
                       "mov r2, %2\n\t"     \
                       "swi 0x0\n\t"        \
                       "pop {pc}\n\t" ::"r"(__SYSCALL_##name),"r"(name1),"r"(name2));  \
}

#define _syscall3(type,name,type1,name1,type2,name2,type3,name3)  \
asmlinkage type name(type1 name1){     \
    __asm__ __volatile__("push {lr}\n\t"    \
                       "mov r0, %0\n\t"     \
                       "mov r1, %1\n\t"     \
                       "mov r2, %2\n\t"     \
                       "mov r3, %3\n\t"     \
                       "swi 0x0\n\t"        \
                       "pop {pc}\n\t" ::"r"(__SYSCALL_##name),"r"(name1),"r"(name2),"r"(name3));  \
}

#define _syscall4(type,name,type1,name1,type2,name2,type3,name3,type4,name4)  \
asmlinkage type name(type1 name1){     \
    __asm__ __volatile__("push {lr}\n\t"    \
                       "mov r0, %0\n\t"     \
                       "mov r1, %1\n\t"     \
                       "mov r2, %2\n\t"     \
                       "mov r3, %3\n\t"     \
                       "mov r4, %4\n\t"     \
                       "swi 0x0\n\t"        \
                       "pop {pc}\n\t" ::"r"(__SYSCALL_##name),"r"(name1),"r"(name2),"r"(name3),"r"(name4));  \
}

#define _syscall4(type,name,type1,name1,type2,name2,type3,name3,type4,name4,type5,name5)  \
asmlinkage type name(type1 name1){     \
    __asm__ __volatile__("push {lr}\n\t"    \
                       "mov r0, %0\n\t"     \
                       "mov r1, %1\n\t"     \
                       "mov r2, %2\n\t"     \
                       "mov r3, %3\n\t"     \
                       "mov r4, %4\n\t"     \
                       "mov r5, %5\n\t"     \
                       "swi 0x0\n\t"        \
                       "pop {pc}\n\t" ::"r"(__SYSCALL_##name),"r"(name1),"r"(name2),"r"(name3),"r"(name4),"r"(name5));  \
}


#define __SYSCALL_setup 0
#define __SYSCALL_exit 1
#define __SYSCALL_fork 2
#define __SYSCALL_read 3
#define __SYSCALL_write 4

_syscall0(int,setup)
_syscall0(int,exit)
_syscall0(int,fork)
_syscall0(int,read)
_syscall0(int,write)


#define __SYSCALL_test1 5
_syscall1(int,test1,int,id)

// Below in kernel
int sys_setup(void);
int sys_exit(void);
int sys_fork(void);
int sys_read(void);
int sys_write(void);

int sys_test1(int id);

funcPtr sys_call_table[]={
    sys_setup,
    sys_exit,
    sys_fork,
    sys_read,
    sys_write,
    sys_test1
};

#endif // __KERNEL_SYSCALL_H__
