#pragma once
#include "lib/stdint.h"
#include "lib/list.h"
#include "tss.h"

//GDT selector
#define KERNEL_CS (0x08)
#define KERNEL_DS (0x10)
#define USER_CS (0x28)
#define USER_DS (0x30)

#define PF_KTHREAD (1 << 0)

// task states
#define TASK_RUNNING (1 << 0)
#define TASK_INTERRUPTIBLE (1 << 1)
#define TASK_UNINTERRUPTIBLE (1 << 2)
#define TASK_ZOMBIE (1 << 3)
#define TASK_STOPPED (1 << 4)

// options for creating task
#define CLONE_FS (1 << 0)
#define CLONE_FILES (1 << 1)
#define CLONE_SIGNAL (1 << 2)

struct mm_struct
{
    uint64_t page; //page table point

    // all addresses below are virtual
    uint64_t start_code, end_code;
    uint64_t start_data, end_data;
    uint64_t start_rodata, end_rodata;
    uint64_t start_brk, end_brk;
    uint64_t start_stack;
};

struct thread_struct
{
    // kernel base stack
    // also saved in tss
    uint64_t rsp0;

    // kernel rip
    uint64_t rip;
    // kernel rsp
    uint64_t rsp;

    uint64_t fs;
    uint64_t gs;

    uint64_t cr2;
    uint64_t trap_nr;
    uint64_t error_code;
};

struct task_struct
{
    List list;
    volatile uint8_t state;
    uint8_t flags;

    mm_struct *mm;
    thread_struct *thread;

    uint64_t addr_limit; /*0x0000,0000,0000,0000 - 0x0000,7fff,ffff,ffff user*/
                         /*0xffff,8000,0000,0000 - 0xffff,ffff,ffff,ffff kernel*/

    uint64_t pid;
    uint64_t counter;
    uint64_t signal;
    uint64_t priority;
};

constexpr uint64_t STACK_SIZE = 4096;

union task_union {
    struct task_struct task;
    uint64_t stack[STACK_SIZE / sizeof(uint64_t)];
} __attribute__((aligned(8))); //8 bytes align

struct INIT_TASK_STATE
{
    mm_struct init_mm;
    thread_struct init_thread;
};

void task_init();

INIT_TASK_STATE &get_init_task_state();
TSS_STRUCT &get_init_task_tss();

#define INIT_TASK(tsk)                                  \
    {                                                   \
        .state = TASK_UNINTERRUPTIBLE,                  \
        .flags = PF_KTHREAD,                            \
        .mm = INIT_TASK_STATE::get_mm_struct(),         \
        .thread = INIT_TASK_STATE::get_thread_struct(), \
        .addr_limit = 0xffff800000000000,               \
        .pid = 0,                                       \
        .counter = 1,                                   \
        .signal = 0,                                    \
        .priority = 0                                   \
    }

inline struct task_struct *get_current()
{
    struct task_struct *current = nullptr;
    __asm__ __volatile__("andq %%rsp,%0	\n\t"
                         : "=r"(current)
                         : "0"(~4095UL));
    return current;
}

#define current get_current()

#define GET_CURRENT        \
    "movq	%rsp,	%rbx	\n\t" \
    "andq	$-4095,%rbx	\n\t"

// params rdi, rsi
#define switch_to(prev, next)                                                                       \
    do                                                                                              \
    {                                                                                               \
        __asm__ __volatile__("pushq	%%rbp	\n\t"                                                     \
                             "pushq	%%rax	\n\t"                                                     \
                             "movq	%%rsp,	%0	\n\t"                                                  \
                             "movq	%2,	%%rsp	\n\t"                                                  \
                             "leaq	1f(%%rip),	%%rax	\n\t"                                           \
                             "movq	%%rax,	%1	\n\t"                                                  \
                             "pushq	%3		\n\t"                                                       \
                             "jmp	__switch_to	\n\t"                                                 \
                             "1:	\n\t"                                                              \
                             "popq	%%rax	\n\t"                                                      \
                             "popq	%%rbp	\n\t"                                                      \
                             : "=m"(prev->thread->rsp), "=m"(prev->thread->rip)                     \
                             : "m"(next->thread->rsp), "m"(next->thread->rip), "D"(prev), "S"(next) \
                             : "memory");                                                           \
    } while (0)
