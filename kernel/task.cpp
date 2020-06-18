#include "task.h"
#include "memory.h"
#include "memory_flag.h"
#include "tss.h"
#include "ptrace.h"

static INIT_TASK_STATE init_task_state;
static mm_struct init_task_mm;
static TSS_STRUCT init_task_tss;
static task_struct *init_task;
extern "C" unsigned long do_exit(unsigned long code)
{
    while (1)
        ;
}
extern "C" void kernel_thread_func(void);
__asm__(
    "kernel_thread_func:	\n\t"
    "	popq	%r15	\n\t"
    "	popq	%r14	\n\t"
    "	popq	%r13	\n\t"
    "	popq	%r12	\n\t"
    "	popq	%r11	\n\t"
    "	popq	%r10	\n\t"
    "	popq	%r9	\n\t"
    "	popq	%r8	\n\t"
    "	popq	%rbx	\n\t"
    "	popq	%rcx	\n\t"
    "	popq	%rdx	\n\t"
    "	popq	%rsi	\n\t"
    "	popq	%rdi	\n\t"
    "	popq	%rbp	\n\t"
    "	popq	%rax	\n\t"
    "	movq	%rax,	%ds	\n\t"
    "	popq	%rax		\n\t"
    "	movq	%rax,	%es	\n\t"
    "	popq	%rax		\n\t"
    "	addq	$0x38,	%rsp	\n\t"
    "	movq	%rdx,	%rdi	\n\t"
    "	callq	*%rbx		\n\t"
    "	movq	%rax,	%rdi	\n\t"
    "	callq	do_exit		\n\t");

uint64_t init(uint64_t arg)
{
    printk("this is init thread\n");
    while(1);
}

static int create_kernel_thread(task_struct *task, uint64_t (*fn)(uint64_t), uint64_t arg, uint64_t flags)
{
    struct pt_regs regs;
    memset(&regs, 0, sizeof(regs));

    regs.rbx = (uint64_t)fn;
    regs.rdx = (uint64_t)arg;

    regs.ds = KERNEL_DS;
    regs.es = KERNEL_DS;
    regs.cs = KERNEL_CS;
    regs.ss = KERNEL_DS;
    regs.rflags = (1 << 9);
    regs.rip = (uint64_t)fn;

    // place thread_struct after task_struct
    auto thread = (struct thread_struct *)(task + 1);
    task->thread = thread;
    thread->fs = KERNEL_DS;
    thread->gs = KERNEL_DS;
    thread->cr2 = 0;
    thread->trap_nr = 0;
    thread->error_code = 0;
    // copy to regs to the stack end
    memcpy((void *)((uint8_t *)task + STACK_SIZE - sizeof(struct pt_regs)), &regs, sizeof(struct pt_regs));
    // stack end is equal to stack base
    thread->rsp0 = (uint64_t)task + STACK_SIZE;
    thread->rip = regs.rip;
    // the real stack points stack end - pt_regs
    thread->rsp = (uint64_t)task + STACK_SIZE - sizeof(struct pt_regs);

    task->state = TASK_RUNNING;

    return 0;
}

void task_init()
{
    auto page = alloc_pages(STACK_SIZE / PAGE_4K_SIZE, PG_PTable_Maped | PG_Kernel | PG_Active);

    auto stack_start = (uint64_t)(Phy_To_Virt(page->physical_address) + STACK_SIZE);

    init_task_tss =
        {.reserved1 = 0,
         .rsp0 = stack_start,
         .rsp1 = stack_start,
         .rsp2 = stack_start,
         .reserved2 = 0,
         .ist1 = 0xffff800000007c00,
         .ist2 = 0xffff800000007c00,
         .ist3 = 0xffff800000007c00,
         .ist4 = 0xffff800000007c00,
         .ist5 = 0xffff800000007c00,
         .ist6 = 0xffff800000007c00,
         .ist7 = 0xffff800000007c00,
         .reserved3 = 0,
         .reserved4 = 0,
         .io_map_base_addr = 0};

    init_task = (task_struct *)Phy_To_Virt(page->physical_address);
    memset(init_task, 0, STACK_SIZE);

    list_init(&init_task->list);

    init_task->state = TASK_UNINTERRUPTIBLE;
    init_task->flags = PF_KTHREAD;
    init_task->addr_limit = 0xffff800000000000;
    init_task->pid = 0;
    init_task->counter = 1;
    init_task->signal = 0;
    init_task->priority = 0;

    // set mm and thread

    init_task->mm = &init_task_mm;
    init_task_mm.page = Get_CR3();
    init_task_mm.start_stack = stack_start;

    create_kernel_thread(init_task, &init, 10, CLONE_FS | CLONE_FILES | CLONE_SIGNAL);

    set_tss(init_task_tss.rsp0, init_task_tss.rsp1, init_task_tss.rsp2, init_task_tss.ist1, init_task_tss.ist2, init_task_tss.ist3, init_task_tss.ist4, init_task_tss.ist5, init_task_tss.ist6, init_task_tss.ist7);
    
    asm __volatile__("movq	%0,	%%fs \n\t" ::"a"(init_task->thread->fs));
    asm __volatile__("movq	%0,	%%gs \n\t" ::"a"(init_task->thread->gs));
    asm __volatile__("movq	%0,	%%rsp \n\t" ::"a"(init_task->thread->rsp));
    asm __volatile__("push  %0 \n\t" ::"a"(init_task->thread->rip));
    printk_hex(init_task->thread->rip);
    asm __volatile__("retq");
    // asm __volatile__("movq	%0,	%%fs \n\t" ::"a"(init_task->thread->fs));
    // asm __volatile__("movq	%0,	%%gs \n\t" ::"a"(init_task->thread->gs));
    // asm __volatile__("movq	%0,	%%rsp \n\t" ::"a"(init_task->thread->rsp));
    // asm __volatile__("movq	push %0 \n\t" ::"m"(init_task->thread->rip));
    // switch_to(current, init_task);
}

INIT_TASK_STATE &get_init_task_state()
{
    return init_task_state;
}

TSS_STRUCT &get_init_task_tss()
{
    return init_task_tss;
}

extern "C" void __switch_to(struct task_struct *prev, struct task_struct *next)
{

    auto &task_tss = get_init_task_tss();
    task_tss.rsp0 = next->thread->rsp0;

    set_tss(task_tss.rsp0, task_tss.rsp1, task_tss.rsp2, task_tss.ist1, task_tss.ist2, task_tss.ist3, task_tss.ist4, task_tss.ist5, task_tss.ist6, task_tss.ist7);

    __asm__ __volatile__("movq	%%fs,	%0 \n\t"
                         : "=a"(prev->thread->fs));
    __asm__ __volatile__("movq	%%gs,	%0 \n\t"
                         : "=a"(prev->thread->gs));

    __asm__ __volatile__("movq	%0,	%%fs \n\t" ::"a"(next->thread->fs));
    __asm__ __volatile__("movq	%0,	%%gs \n\t" ::"a"(next->thread->gs));
}

unsigned long do_fork(struct pt_regs *regs, unsigned long clone_flags, unsigned long stack_start, unsigned long stack_size)
{
    struct task_struct *tsk = nullptr;
    struct thread_struct *thd = nullptr;
    struct Page *p = nullptr;

    p = alloc_pages(8, PG_PTable_Maped | PG_Active | PG_Kernel);

    tsk = (struct task_struct *)Phy_To_Virt(p->physical_address);

    memset(tsk, 0, sizeof(*tsk));
    *tsk = *current;

    list_init(&tsk->list);
    list_add_to_before(&init_task->list, &tsk->list);
    tsk->pid++;
    tsk->state = TASK_UNINTERRUPTIBLE;

    // place thread_struct after task_struct
    thd = (struct thread_struct *)(tsk + 1);
    tsk->thread = thd;

    // copy to regs to the stack end
    memcpy((void *)((uint8_t *)tsk + STACK_SIZE - sizeof(struct pt_regs)), regs, sizeof(struct pt_regs));

    // stack end is equal to stack base
    thd->rsp0 = (uint64_t)tsk + STACK_SIZE;
    thd->rip = regs->rip;
    // the real stack points stack end - pt_regs
    thd->rsp = (uint64_t)tsk + STACK_SIZE - sizeof(struct pt_regs);

    if (!(tsk->flags & PF_KTHREAD))
        // thd->rip = regs->rip = (unsigned long)ret_from_intr;

        tsk->state = TASK_RUNNING;

    return 0;
}
