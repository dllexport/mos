#include "task.h"
#include "memory.h"
#include "memory_flag.h"
#include "tss.h"
#include "ptrace.h"
#include "lib/printk.h"
#include "lib/debug.h"
static mm_struct init_task_mm;
static task_struct *init_task;
static task_struct *current_task;
task_struct *get_current_task()
{
    return current_task;
}

extern "C" unsigned long do_exit(unsigned long code)
{
    printk("init2 finished\n");

    while (1)
        ;
}

// we pop all pt_regs out
// then restore the stack to rsp0(stack base)
// then call the fn
// then do_exit
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

static int create_kernel_thread(uint64_t (*fn)(uint64_t), uint64_t arg, uint64_t flags);

extern "C" void ret_syscall();
void user_level_function()
{
    while (1)
        ;
}

extern "C" uint64_t do_execve(struct pt_regs *regs)
{
    regs->rdx = 0x500000; //RIP
    regs->rcx = 0x400000; //RSP
    // regs->rax = 1;
    regs->cs = KERNEL_CS;
    regs->ss = KERNEL_DS;
    regs->ds = KERNEL_DS;
    regs->es = KERNEL_DS;

    memcpy((void *)0x400000, (uint8_t *)&user_level_function, 1024);
    auto i = *(uint64_t *)0x0000000000400000;
    return 0;
}

void schedule()
{
    auto next = list_prev(&current->list);
    auto p = (task_struct *)next;
    // printk("from %d to %d\n", current->pid, p->pid);
    switch_to(current, p);
}

uint64_t init2(uint64_t arg)
{
    auto nextt = list_next(&current->list);
    auto pp = (task_struct *)nextt;
    printk("this is init 2\n");
    while (1)
    {
        printk("2");
    }
    current->thread->rip = uint64_t(&ret_syscall);
    current->thread->rsp = uint64_t((uint8_t *)current + STACK_SIZE - sizeof(pt_regs));
    auto regs = (pt_regs *)current->thread->rsp;
    asm __volatile__("movq	%1,	%%rsp	\n\t"
                     "pushq	%2		\n\t"
                     "jmp	do_execve	\n\t" ::"D"(regs),
                     "m"(current->thread->rsp), "m"(current->thread->rip)
                     : "memory");

    return 0;
}

uint64_t init(uint64_t arg)
{

    printk("this is init thread\n");

    create_kernel_thread(&init2, 1, CLONE_FS | CLONE_FILES | CLONE_SIGNAL);

    auto next = list_next(&current->list);
    auto p = (task_struct *)next;
    printk("current rsp : %x\n", p->thread->rsp0);
    printk("next rsp : %x\n", p->thread->rip);
    current_task = current;
    asm volatile("sti");
    while (1)
    {
        printk("1");
    }
}

unsigned long do_fork(struct pt_regs *regs, unsigned long clone_flags)
{

    auto page = alloc_pages(1, PG_PTable_Maped | PG_Kernel | PG_Active);

    auto stack_start = (uint64_t)(Phy_To_Virt(page->physical_address) + 1);

    auto task = (task_struct *)Phy_To_Virt(page->physical_address);

    memset(task, 0, sizeof(*task));
    // *task = *current;

    list_init(&task->list);
    list_add_to_behind(&init_task->list, &task->list);

    task->pid++;
    task->state = TASK_UNINTERRUPTIBLE;

    auto next = list_next(&init_task->list);
    auto p = (task_struct *)next;
    auto p_cur = current;

    // place thread_struct after task_struct
    auto thread = (struct thread_struct *)(task + 1);
    task->thread = thread;

    // copy to regs to the stack end
    memcpy((void *)((uint8_t *)task + STACK_SIZE - sizeof(struct pt_regs)), regs, sizeof(struct pt_regs));

    // stack end is equal to stack base
    thread->rsp0 = (uint64_t)task + STACK_SIZE;
    thread->rip = regs->rip;
    // the real stack points stack end - pt_regs
    thread->rsp = (uint64_t)task + STACK_SIZE - sizeof(struct pt_regs);

    if (!(clone_flags & PF_KTHREAD))
    {
        thread->rip = (uint64_t)Phy_To_Virt(&ret_syscall);
    }

    task->state = TASK_RUNNING;

    return 0;
}

static int create_kernel_thread(uint64_t (*fn)(uint64_t), uint64_t arg, uint64_t flags)
{

    struct pt_regs regs;
    memset(&regs, 0, sizeof(regs));

    regs.rbx = (uint64_t)fn;
    regs.rdx = (uint64_t)arg;

    regs.ds = KERNEL_DS;
    regs.es = KERNEL_DS;
    regs.cs = KERNEL_CS;
    regs.ss = KERNEL_DS;
    regs.rflags = (1 << 9); // interrupt enable
    regs.rip = (uint64_t)Phy_To_Virt(kernel_thread_func);

    return do_fork(&regs, flags | PF_KTHREAD);
}

void task_init()
{

    wrmsr(MSR_STAR, ((uint64_t)0x0020) << 48);

    auto page = alloc_pages(1, PG_PTable_Maped | PG_Kernel | PG_Active);

    auto stack_start = (uint64_t)(Phy_To_Virt(page->physical_address) + PAGE_4K_SIZE);

    auto ist = (uint64_t)Phy_To_Virt(0x0000000000007c00);

    tss_struct init_task_tss =
        {.reserved1 = 0,
         .rsp0 = stack_start,
         .rsp1 = stack_start,
         .rsp2 = stack_start,
         .reserved2 = 0,
         .ist1 = ist,
         .ist2 = ist,
         .ist3 = ist,
         .ist4 = ist,
         .ist5 = ist,
         .ist6 = ist,
         .ist7 = ist,
         .reserved3 = 0,
         .reserved4 = 0,
         .io_map_base_addr = 0};

    set_tss(init_task_tss);

    init_task = (task_struct *)Phy_To_Virt(page->physical_address);

    printk("init task: %x\n", init_task);

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

    auto thread = (struct thread_struct *)(init_task + 1);
    init_task->thread = thread;
    thread->fs = KERNEL_DS;
    thread->gs = KERNEL_DS;
    thread->rsp0 = (uint64_t)init_task + STACK_SIZE;
    thread->rsp = (uint64_t)init_task + STACK_SIZE;
    thread->rip = uint64_t(&init);
    // the real stack points stack end - pt_regs
    init_task->state = TASK_RUNNING;

    asm __volatile__("movq	%0,	%%fs \n\t" ::"a"(init_task->thread->fs));
    asm __volatile__("movq	%0,	%%gs \n\t" ::"a"(init_task->thread->gs));
    asm __volatile__("movq	%0,	%%rsp \n\t" ::"a"(init_task->thread->rsp));
    asm __volatile__("push  %0 \n\t" ::"a"(init_task->thread->rip));
    asm __volatile__("retq");
}

extern "C" void __switch_to(struct task_struct *prev, struct task_struct *next)
{

    auto &task_tss = get_tss();
    task_tss.rsp0 = next->thread->rsp0;
    task_tss.rsp1 = next->thread->rsp0;
    task_tss.rsp2 = next->thread->rsp0;

    set_tss(task_tss);

    __asm__ __volatile__("movq	%%fs,	%0 \n\t"
                         : "=a"(prev->thread->fs));
    __asm__ __volatile__("movq	%%gs,	%0 \n\t"
                         : "=a"(prev->thread->gs));

    __asm__ __volatile__("movq	%0,	%%fs \n\t" ::"a"(next->thread->fs));
    __asm__ __volatile__("movq	%0,	%%gs \n\t" ::"a"(next->thread->gs));

    current_task = next;

    __asm__ __volatile__("sti");
}
