//
// Created by XingfengYang on 2020/6/26.
//

#include "kernel/thread.h"
#include "arm/kernel_vmm.h"
#include "kernel/kheap.h"
#include "kernel/kobject.h"
#include "kernel/kstack.h"
#include "kernel/kvector.h"
#include "kernel/log.h"
#include "kernel/percpu.h"
#include "kernel/vfs_dentry.h"
#include "libc/stdlib.h"
#include "libc/string.h"

extern Heap kernelHeap;
extern PhysicalPageAllocator kernelPageAllocator;
extern PhysicalPageAllocator userspacePageAllocator;

extern uint64_t ktimer_sys_runtime();

uint32_t pidMap[2048] = {0};

uint32_t thread_alloc_pid() {
    for (uint32_t i = 0; i < 2048 / BITS_IN_UINT32; i++) {
        if (pidMap[i] != MAX_UINT_32) {
            for (uint8_t j = 0; j < BITS_IN_UINT32; j++) {
                if ((pidMap[i] & ((uint32_t) 0x1 << j)) == 0) {
                    pidMap[i] |= (uint32_t) 0x1 << j;
                    return i * BITS_IN_UINT32 + j;
                }
            }
        }
    }
}

uint32_t thread_free_pid(uint32_t pid) {
    uint32_t index = pid / BITS_IN_UINT32;
    uint8_t bitIndex = pid % BITS_IN_UINT32;
    pidMap[index] ^= (uint32_t) 0x1 << bitIndex;
}

KernelStatus thread_default_suspend(struct Thread *thread) {
    // TODO:
    return OK;
}

KernelStatus thread_default_resume(struct Thread *thread) {
    // TODO:
    return OK;
}

KernelStatus thread_default_sleep(struct Thread *thread, uint32_t deadline) {
    // TODO:
    return OK;
}

KernelStatus thread_default_detach(struct Thread *thread) {
    // TODO:
    return OK;
}

KernelStatus thread_default_join(struct Thread *thread, int *returnCode, uint32_t deadline) {
    // TODO:
    return OK;
}

KernelStatus thread_default_exit(struct Thread *thread, uint32_t returnCode) {
    // TODO:
    return OK;
}

KernelStatus thread_default_kill(struct Thread *thread) {
    KernelStatus freeStatus = OK;
    // Free stack
    freeStatus = thread->stack.operations.free(&thread->stack);
    if (freeStatus != OK) {
        LogError("[KStack]: kStack free failed.\n");
        return freeStatus;
    }
    // Free pid
    thread_free_pid(thread->pid);
    // Free FS
    freeStatus = thread->filesStruct.fileDescriptorTable.operations.free(&thread->filesStruct.fileDescriptorTable);
    if (freeStatus != OK) {
        LogError("[kVector]: kVector free failed.\n");
        return freeStatus;
    }
    // Free thread structure
    freeStatus = kernelHeap.operations.free(&kernelHeap, thread);
    if (freeStatus != OK) {
        LogError("[KStack]: kStack free failed.\n");
        return freeStatus;
    }
    LogInfo("[Thread]: thread has been freed.\n");
    return OK;
}

uint32_t filestruct_default_openfile(FilesStruct *filesStruct, DirectoryEntry *directoryEntry) {
    FileDescriptor *fileDescriptor = (FileDescriptor *) kernelHeap.operations.alloc(&kernelHeap,
                                                                                    sizeof(FileDescriptor));
    fileDescriptor->directoryEntry = directoryEntry;
    fileDescriptor->node.prev = nullptr;
    fileDescriptor->node.next = nullptr;
    fileDescriptor->pos = 0;

    KernelStatus status = filesStruct->fileDescriptorTable.operations.add(&filesStruct->fileDescriptorTable,
                                                                           &fileDescriptor->node);
    if (status != OK) {
        LogError("[Open]: file open failed, cause add fd table failed.\n");
        return 0;
    }
    // because 0,1,2 are std in, out, err use
    return (filesStruct->fileDescriptorTable.size - 1) + 3;
}

Thread *thread_default_copy(Thread *thread, CloneFlags cloneFlags, uint32_t heapStart) {
    LogInfo("[Thread]: Copy Start.\n");
    Thread *p = thread_create(thread->name, thread->entry, thread->arg, thread->priority);
    LogInfo("[Thread]: Clone VMM: '%s'.\n", p->name);
    if (p == nullptr) {
        LogError("[Thread]: copy failed: p == nullptr.\n");
        return nullptr;
    }
    if (cloneFlags & CLONE_VM) {
        LogInfo("[Thread]: Clone VMM: '%s'.\n", p->name);
        p->memoryStruct.virtualMemory.physicalPageAllocator = thread->memoryStruct.virtualMemory.physicalPageAllocator;
        p->memoryStruct.virtualMemory.operations = thread->memoryStruct.virtualMemory.operations;
        p->memoryStruct.heap = thread->memoryStruct.heap;
    } else {
        LogInfo("[Thread]: Create new vmm: '%s'.\n", p->name);
        KernelStatus vmmCreateStatus = vmm_create(&p->memoryStruct.virtualMemory, &userspacePageAllocator);
        if (vmmCreateStatus != OK) {
            LogError("[Thread]: vmm create failed for thread: '%s'.\n", p->name);
            p->memoryStruct.virtualMemory.operations.release(&p->memoryStruct.virtualMemory);
            p->operations.kill(p);
            return nullptr;
        }
        LogInfo("[Thread]: Create new heap: '%s'.\n", p->name);
        KernelStatus heapCreateStatus = heap_create(&p->memoryStruct.heap,
                                                    p->memoryStruct.sectionInfo.bssEndSectionAddr, 16 * MB);
        if (heapCreateStatus != OK) {
            LogError("[Thread]: heap create failed for thread: '%s'.\n", p->name);
            p->memoryStruct.virtualMemory.operations.release(&p->memoryStruct.virtualMemory);
            p->memoryStruct.heap.operations.free(&p->memoryStruct.heap, &p->memoryStruct.heap);
            p->operations.kill(p);
            return nullptr;
        }
    }
    if (cloneFlags & CLONE_FILES) {
        LogInfo("[Thread]: Clone FILES: '%s'.\n", p->name);
        p->filesStruct.fileDescriptorTable.size = thread->filesStruct.fileDescriptorTable.size;
        p->filesStruct.fileDescriptorTable.capacity = thread->filesStruct.fileDescriptorTable.capacity;
        p->filesStruct.fileDescriptorTable.data = thread->filesStruct.fileDescriptorTable.data;
    }
    if (cloneFlags & CLONE_FS) {
        LogInfo("[Thread]: Clone FS: '%s'.\n", p->name);
        // TODO
    }
    p->parentThread = thread;
    LogInfo("[Thread]: Copy Done.\n");
    return p;
}

Thread *thread_create(const char *name, ThreadStartRoutine entry, void *arg, uint32_t priority) {
    Thread *thread = (Thread *) kernelHeap.operations.alloc(&kernelHeap, sizeof(Thread));

    if (thread != nullptr) {
        thread->magic = THREAD_MAGIC;
        thread->threadStatus = THREAD_INITIAL;
        kstack_allocate(&thread->stack);

        thread->stack.operations.clear(&thread->stack);
        thread->stack.operations.push(&thread->stack, (uint32_t) entry);     // R15 PC
        thread->stack.operations.push(&thread->stack, (uint32_t) entry);     // R14 LR
        thread->stack.operations.push(&thread->stack, 0x12121212);// R12
        thread->stack.operations.push(&thread->stack, 0x11111111);// R11
        thread->stack.operations.push(&thread->stack, 0x10101010);// R10
        thread->stack.operations.push(&thread->stack, 0x09090909);// R09
        thread->stack.operations.push(&thread->stack, 0x08080808);// R08
        thread->stack.operations.push(&thread->stack, 0x07070707);// R07
        thread->stack.operations.push(&thread->stack, 0x06060606);// R06
        thread->stack.operations.push(&thread->stack, 0x05050505);// R05
        thread->stack.operations.push(&thread->stack, 0x04040404);// R04
        thread->stack.operations.push(&thread->stack, 0x03030303);// R03
        thread->stack.operations.push(&thread->stack, 0x02020202);// R02
        thread->stack.operations.push(&thread->stack, 0x01010101);// R01
        thread->stack.operations.push(&thread->stack, (uint32_t) arg);       // R00
        thread->stack.operations.push(&thread->stack, 0x600001d3);// cpsr

        thread->priority = priority;
        thread->currCpu = INVALID_CPU;
        thread->lastCpu = INVALID_CPU;
        thread->entry = (ThreadStartRoutine) entry;

        thread->runtimeNs = 0;
        thread->runtimeVirtualNs = 0;
        thread->startTime = ktimer_sys_runtime();

        thread->cpuAffinity = CPU_MASK_ALL;

        thread->parentThread = nullptr;
        thread->pid = thread_alloc_pid();
        strcpy(thread->name, name);
        thread->arg = arg;

        thread->threadList.prev = nullptr;
        thread->threadList.next = nullptr;

        thread->threadReadyQueue.prev = nullptr;
        thread->threadReadyQueue.next = nullptr;

        thread->rbNode.parent = nullptr;
        thread->rbNode.left = nullptr;
        thread->rbNode.right = nullptr;
        thread->rbNode.color = NODE_RED;

        thread->operations.suspend = (ThreadOperationSuspend) thread_default_suspend;
        thread->operations.resume = (ThreadOperationResume) thread_default_resume;
        thread->operations.sleep = (ThreadOperationSleep) thread_default_sleep;
        thread->operations.detach = (ThreadOperationDetach) thread_default_detach;
        thread->operations.join = (ThreadOperationJoin) thread_default_join;
        thread->operations.exit = (ThreadOperationExit) thread_default_exit;
        thread->operations.kill = (ThreadOperationKill) thread_default_kill;
        thread->operations.copy = thread_default_copy;

        thread->filesStruct.operations.openFile = (FilesStructOperationOpenFile) filestruct_default_openfile;
        kvector_allocate(&thread->filesStruct.fileDescriptorTable);
        thread->memoryStruct.sectionInfo.codeSectionAddr = 0;
        thread->memoryStruct.sectionInfo.roDataSectionAddr = 0;
        thread->memoryStruct.sectionInfo.dataSectionAddr = 0;
        thread->memoryStruct.sectionInfo.bssSectionAddr = 0;

        vmm_create(&thread->memoryStruct.virtualMemory, &kernelPageAllocator);

        thread->memoryStruct.virtualMemory.physicalPageAllocator = &kernelPageAllocator;
        thread->memoryStruct.virtualMemory.pageTable = kernel_vmm_get_page_table();
        thread->memoryStruct.heap = kernelHeap;

        thread->object.operations.init(&thread->object, KERNEL_OBJECT_THREAD, USING);

        LogInfo("[Thread]: thread '%s' created.\n", thread->name);
        return thread;
    }
    LogError("[Thread]: thread '%s' created failed.\n", name);
    return nullptr;
}

_Noreturn uint32_t *idle_thread_routine(int arg) {
    while (1) {
        LogInfo("[Thread]: IDLE: %d \n", arg);
        asm volatile("wfi");
    }
}

Thread *thread_create_idle_thread(uint32_t cpuNum) {
    Thread *idleThread = thread_create("IDLE", (ThreadStartRoutine) idle_thread_routine, (void *) cpuNum, IDLE_PRIORITY);
    idleThread->cpuAffinity = cpuNum;
    // 2. idle thread
    idleThread->pid = 0;

    char idleNameStr[10] = {'\0'};
    strcpy(idleThread->name, itoa(cpuNum, (char *) &idleNameStr, 10));
    // TODO : other properties, like list
    LogInfo("[Thread]: Idle thread for CPU '%d' created.\n", cpuNum);
    return idleThread;
}

KernelStatus thread_reschedule() {
    // TODO
    return OK;
}
