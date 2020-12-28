//
// Created by XingfengYang on 2020/6/29.
//

#include "kernel/sched.h"
#include "arm/cpu.h"
#include "kernel/interrupt.h"
#include "kernel/log.h"
#include "kernel/percpu.h"
#include "kernel/spinlock.h"
#include "kernel/assert.h"
#include "libc/stdlib.h"

uint32_t PRIORITY_2_WEIGHT[40] = {
        88761,
        71755,
        56483,
        46273,
        36291,
        29154,
        23254,
        18705,
        14949,
        11916,
        9548,
        7620,
        6100,
        4904,
        3906,
        3121,
        2501,
        1991,
        1586,
        1277,
        1024,
        820,
        655,
        526,
        423,
        335,
        272,
        215,
        172,
        137,
        110,
        87,
        70,
        56,
        45,
        36,
        29,
        23,
        18,
        15,
};

#define PRIORITY_DEFAULT_WEIGHT 1024

extern uint64_t ktimer_sys_runtime_tick(uint64_t tickInterval);

extern void cpu_save_context(uint32_t sp);
extern void cpu_restore_context(uint32_t sp);
extern void cpu_switch_mm(uint32_t pageTable);


#define TIMER_TICK_MS 50

TimerHandler tickHandler;

uint32_t current_thread_stack = 0;
uint32_t switch_thread_stack = 0;
uint32_t switch_to_signal = 0;

Thread *prevThread = nullptr;
Thread *currentThread = nullptr;

void tick() {
    LogInfo("[Schd]: tick.\n");
    ktimer_sys_runtime_tick(TIMER_TICK_MS);
    schd_switch_next();
}

SpinLock spinlock = SpinLockCreate();

KernelStatus schd_switch_next(void) {
    uint32_t cpuid = read_cpuid();
    LogWarn("[Schd]: cpuId %d .\n", cpuid);
    PerCpu *perCpu = percpu_get(cpuid);
    Thread *thread = perCpu->operations.getNextThread(perCpu);

    spinlock.operations.acquire(&spinlock);

    thread->runtimeNs += TIMER_TICK_MS;
    thread->runtimeVirtualNs += (PRIORITY_DEFAULT_WEIGHT / PRIORITY_2_WEIGHT[thread->priority]) * thread->runtimeNs;
    schd_switch_to(thread);

    spinlock.operations.release(&spinlock);

    if (thread != perCpu->idleThread) {
        Thread *removedThread = perCpu->operations.removeThread(perCpu, thread);
        schd_add_thread(removedThread, removedThread->priority);
    }
    return OK;
}

KernelStatus schd_init() {
    if (percpu_create(CPU_EXISTS_NUM) != ERROR) {
        for (uint32_t cpuId = 0; cpuId < CPU_EXISTS_NUM; cpuId++) {
            Thread *idleThread = thread_create_idle_thread(cpuId);
            if (idleThread == nullptr) {
                return ERROR;
            }
            PerCpu *perCpu = percpu_get(cpuId);
            perCpu->operations.init(perCpu, cpuId, idleThread);
        }
    }

    LogInfo("[Schd]: Schd inited.\n");
    return OK;
}

KernelStatus schd_add_thread(Thread *thread, uint32_t priority) {
    thread->priority = priority;
    PerCpu *perCpu = percpu_min_priority(thread->cpuAffinity);
    KernelStatus threadAddStatus = perCpu->operations.insertThread(perCpu, thread);
    if (threadAddStatus != OK) {
        LogError("[Schd] thread %s add to schduler failed.\n", thread->name);
        return ERROR;
    }
    return OK;
}

KernelStatus schd_schedule(void) {
    tickHandler.node.next = nullptr;
    tickHandler.node.prev = nullptr;
    tickHandler.timer_interrupt_handler = &tick;
    register_time_interrupt(&tickHandler);
    LogInfo("[Schd]: Schd started.\n");
    return OK;
}


void schd_save_context(Thread* thread){
    cpu_save_context(thread->stack.top);
}

void schd_restore_context(Thread* thread){
    cpu_restore_context(thread->stack.top);
}

void schd_switch_mm(Thread* thread){
    cpu_switch_mm((uint32_t) thread->memoryStruct.virtualMemory.pageTable);
}

void schd_switch_context(){
    int flag = switch_to_signal;
    switch_to_signal = 0;
    if(flag==0){
        return;
    }else if(flag==1){
        schd_save_context(prevThread);
        schd_restore_context(currentThread);
        schd_switch_mm(currentThread);
    }else if(flag==2){
        schd_restore_context(currentThread);
        schd_switch_mm(currentThread);
    }
}

KernelStatus schd_switch_to(Thread *thread) {
    // push r0~r3
    if (thread == nullptr) {
        LogWarn("[Schd]: cant switch to nullptr thread.\n");
        return ERROR;
    }
    LogInfo("[Schd]: switch to: %s.\n", thread->name);

    // save current thread
    if (currentThread == nullptr) {
        switch_to_signal = 2;
        current_thread_stack = 0;
        // restore r0~r12
    } else {
        switch_to_signal = 1;
        current_thread_stack = (uint32_t) (&currentThread->stack.top);
    }
    switch_thread_stack = (uint32_t) (&thread->stack.top);
    prevThread = currentThread;
    currentThread = thread;
    percpu_get(read_cpuid())->currentThread = thread;
    // pop r0~r3
    return OK;
}

KernelStatus schd_block(void) {
    // TODO:
    return OK;
}

KernelStatus schd_yield(void) {
    // TODO:
    return OK;
}

KernelStatus schd_preempt(void) {
    // TODO:
    return OK;
}

uint32_t schd_getpid() {
    uint32_t cpuid = read_cpuid();
    PerCpu *perCpu = percpu_get(cpuid);
    return perCpu->currentThread->pid;
}

Thread *schd_get_current_thread(void) { return currentThread; }
