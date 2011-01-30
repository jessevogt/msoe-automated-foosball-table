/*
 * (C) Finite State Machine Labs Inc. 1999 <business@fsmlabs.com>
 *
 * Released under the terms of GPL 2.
 * Open RTLinux makes use of a patented process described in
 * US Patent 5,995,745. Use of this process is governed
 * by the Open RTLinux Patent License which can be obtained from
 * www.fsmlabs.com/PATENT or by sending email to
 * licensequestions@fsmlabs.com
 */

#include <linux/config.h>
#include <linux/irq.h>
#include <asm/irq.h>
#include <asm/hw_irq.h>
#include <asm/apic.h>
#include <arch/constants.h>
#include <rtl_core.h>
extern struct irq_control_s  irq_control;
static struct irq_control_s pre_patch_control;

#define RTL_NR_IRQS NR_IRQS

/* interface to the hardware controller handlers */
struct hw_interrupt_type *rtl_irq_desc[NR_IRQS];
static struct hw_interrupt_type *save_linux_irq_desc[NR_IRQS];

static inline int rtl_irq_controller_get_irq(struct pt_regs r){
	return r.orig_eax & 0xff;
}
static inline void rtl_irq_controller_ack(unsigned int irq) {
	if(rtl_irq_desc[irq]) rtl_irq_desc[irq]->ack(irq);
}
static inline void rtl_irq_controller_enable(unsigned int irq){
	if(rtl_irq_desc[irq]) 
		rtl_irq_desc[irq]->enable(irq);
}
static inline void rtl_irq_controller_disable(unsigned int irq){
	if(rtl_irq_desc[irq]) rtl_irq_desc[irq]->disable(irq);
}

void rtl_soft_cli(void);
void rtl_soft_sti(void);
void rtl_soft_save_flags(unsigned long *);
void rtl_soft_restore_flags(unsigned long );
void rtl_soft_local_irq_save(unsigned long *);
void rtl_soft_local_irq_restore(unsigned long );

/* dispatching rt and linux handlers */
asmlinkage void (*xdo_IRQ)(struct pt_regs);
struct rtl_global_handlers;

static inline void dispatch_linux_irq(struct pt_regs *regs,unsigned int irq){
	xdo_IRQ(*regs);
}

#ifdef CONFIG_X86_LOCAL_APIC
static void rtl_local_intercept(MACHDEPREGS );
static inline void dispatch_rtl_local_handler(int pnd, struct pt_regs *r)
{
	rtl_local[rtl_getcpuid()].rt_handlers[pnd](r);
}
struct {
        void (*smp_reschedule_interrupt)(void);
        void (*smp_invalidate_interrupt)(void);
        void (*smp_error_interrupt)(void);
        void (*smp_apic_timer_interrupt)(struct pt_regs *);
        void (*smp_call_function_interrupt)(void);
	void (*rtl_reschedule)(int cpu);
        void (*smp_spurious_interrupt)(void); }local_code;

#endif

#define FAKE_REGS { 0, 0, 0, 0, 0, 0, 0, __KERNEL_DS, 0, 0, (long)rtl_soft_sti, __KERNEL_CS, ARCH_DEFINED_ENABLE, 0, 0 }

	
/* Called with soft disable set */
inline void soft_dispatch_global(unsigned int irq){
	struct pt_regs r = FAKE_REGS;
	DeclareAndInit(cpu_id);
		r.orig_eax = irq;
		L_CLEAR(l_ienable);
		xdo_IRQ(r);
}
#ifdef CONFIG_X86_LOCAL_APIC
void dispatch_local_linux_irq(struct pt_regs *r, int pnd)
{
/*  soft_dispatch_local(VECTOR_TO_LOCAL_PND(r->orig_eax)); */
 soft_dispatch_local(r->orig_eax);
/*  __sti(); */
}

/* #define localdbg() do { if (test_bit(rtl_getcpuid(), &rtl_reserved_cpumask)) rtl_printf("l%x\n", vector); } while (0) */
#define localdbg() do {;} while (0)
void soft_dispatch_local(unsigned int vector)
{
/* 	unsigned int vector = LOCAL_PND_TO_VECTOR(pnd); */
	rtl_soft_cli();

/* 		conpr("d"); conprn(vector); */
	switch(vector){
	/* IPI for rescheduling */
		case RESCHEDULE_VECTOR:
			localdbg();
			local_code.smp_reschedule_interrupt();
			break;
		case INVALIDATE_TLB_VECTOR:
			/* IPI for invalidation */
			localdbg();
			local_code.smp_invalidate_interrupt();
			break;
		case CALL_FUNCTION_VECTOR:
			localdbg();
			local_code.smp_call_function_interrupt();
			break;
		case SPURIOUS_APIC_VECTOR:
			localdbg();
			local_code.smp_spurious_interrupt();
			break;
		case ERROR_APIC_VECTOR:
			localdbg();
			local_code.smp_error_interrupt();
			break;
		case LOCAL_TIMER_VECTOR:
			{
			struct pt_regs r=  FAKE_REGS;
			localdbg();
			local_code.smp_apic_timer_interrupt(&r);
			}
			break;
		default:
			printk("RTL: bad local vector %x\n",vector);
			break;
	}
}
#endif


/* Virtual functions for soft irq descriptor table */
void rtl_virt_shutdown(unsigned int irq) {
	G_DISABLE(irq) ;
	rtl_irq_desc[irq]->shutdown(irq);
}
unsigned int rtl_virt_startup (unsigned int irq){ 
	G_ENABLED(irq) ; 
	return (rtl_irq_desc[irq]->startup(irq));
}
#if LINUX_VERSION_CODE < 0x020300
void rtl_virt_handle (unsigned int irq, struct pt_regs * regs){ 
	rtl_irq_desc[irq]->handle(irq, regs);
}
#endif
static void rtl_virt_ack (unsigned int irq){ return;}

#ifdef CONFIG_SMP
static void rtl_virt_set_affinity(unsigned int irq, unsigned long mask)
{
	if(rtl_irq_desc[irq] && rtl_irq_desc[irq]->set_affinity) {
		rtl_irq_desc[irq]->set_affinity(irq, mask);
	}
}

extern unsigned long irq_affinity [NR_IRQS];
int rtl_irq_set_affinity (unsigned int irq, const unsigned long *mask, unsigned long *oldmask)
{
	rtl_irqstate_t flags;
	if (irq >= RTL_RESCHEDULE_VECTOR - 0x20) {
		return -1;
	}
	if (!rtl_irq_desc[irq] || !rtl_irq_desc[irq]->set_affinity) {
		return -1;
	}
	if (oldmask) {
		*oldmask = irq_affinity[irq];
	}
	if (! mask) {
		return 0;
	}
	if (!(*mask & cpu_online_map))
		return -EINVAL;
	rtl_no_interrupts (flags);
	rtl_spin_lock (&rtl_global.hard_irq_controller_lock);

	irq_affinity[irq] = *mask;
	rtl_irq_desc[irq]->set_affinity(irq, *mask);

	rtl_spin_unlock (&rtl_global.hard_irq_controller_lock);
	rtl_restore_interrupts (flags);
	return 0;
}
#else

static void rtl_virt_set_affinity(unsigned int irq, unsigned long mask){;}
int rtl_irq_set_affinity (unsigned int irq, const unsigned long *mask, unsigned long *oldmask){ return -1;}
#endif

void rtl_virt_enable(unsigned int);
void rtl_virt_disable(unsigned int);
struct hw_interrupt_type rtl_generic_type =  {\
	"RTLinux virtual irq",
	rtl_virt_startup,
	rtl_virt_shutdown,
	rtl_virt_enable,
	rtl_virt_disable,
	rtl_virt_ack,
	rtl_virt_enable,
	rtl_virt_set_affinity,
#if LINUX_VERSION_CODE < 0x020300
	rtl_virt_handle
#endif
};

#ifdef CONFIG_SMP
/* Michael's reschedule interrupt hack (use a global interrupt for it) */
static void rtl_ack_apic(unsigned int irq) {
	ack_APIC_irq();
}
static void rtl_null_enable(unsigned int irq) {
}
struct hw_interrupt_type rtl_resched_irq_type =  {\
	"RTLinux reschedule irq",
	rtl_virt_startup,
	rtl_virt_shutdown,
	rtl_null_enable,
	rtl_virt_disable,
	rtl_ack_apic,
	rtl_virt_enable,
	rtl_virt_set_affinity,
#if LINUX_VERSION_CODE < 0x020300
	rtl_virt_handle /* nothing */
#endif
};
#endif

void (*local_ret_from_intr)(void);

enum pfunctions {
pf___start_rtlinux_patch,
pf___stop_rtlinux_patch,
pf_rtl_emulate_iret,
pf_irq_desc,
pf_do_IRQ,
pf_ret_from_intr,
pf_rtl_exception_intercept,
#ifdef CONFIG_X86_LOCAL_APIC
pf_smp_spurious_interrupt,
#define PF_LOCAL_START pf_smp_spurious_interrupt
pf_smp_error_interrupt,
pf_smp_apic_timer_interrupt,

#ifdef CONFIG_SMP
pf_smp_reschedule_interrupt,
pf_smp_invalidate_interrupt,
pf_smp_call_function_interrupt,
pf_rtl_reschedule,
#define PF_LOCAL_END pf_smp_call_function_interrupt
#else
#define PF_LOCAL_END pf_smp_apic_timer_interrupt
#endif /* CONFIG_SMP */
#endif
};

/* #define DEBUG_RTLINUX 1 */

#ifdef DEBUG_RTLINUX_PATCH_TABLE
static void print_patch_table(void)
{

	const struct func_table *p = &__start_rtlinux_funcs;
	struct patch_table *start_rtlinux_patch = p[0].address;
	struct patch_table *stop_rtlinux_patch = p[1].address;
	struct patch_table *pt;;
	printk("RTLinux table print start=%x stop=%x\n",p,&__stop_rtlinux_funcs);

        for(; p <= &__stop_rtlinux_funcs;p++) {
		printk("RTLinux function table %x => %x\n",p,p->address);
        }
	for(pt = start_rtlinux_patch; pt < stop_rtlinux_patch; pt++)
		printk("RTLinux patch table %x => %x %x\n",pt,pt->address,pt->magic);
        return;
}
#endif

#define JUMP_SIZE 5
#define MAX_JUMPS 20
/* TODO patch_failure should do a recovery */
#define patch_failure(x) {printk(x); return; }
static char saved_jumps[MAX_JUMPS][JUMP_SIZE];

static void save_jump(char *p, enum pfunctions pf)
{
	int i;
	if(pf > MAX_JUMPS){
		patch_failure("RTLinux FATAL ERROR; too many jumps\n");
	}else for(i=0;i<5;i++)saved_jumps[pf][i] = p[i];
}

static void unpatch_jump(char *p, enum pfunctions pf)
{
	int i;
	if(pf > MAX_JUMPS){
		patch_failure("RTLinux FATAL ERROR; too many jumps in unpatch\n");
	}else for(i=0;i<5;i++)p[i] = saved_jumps[pf][i];
}

static void patch_jump(char *code_to_patch,char *target)
{
	int distance;
        distance = (int)target - ((int)code_to_patch +5) ;
        *code_to_patch++= 0xe9; /* jump */
        *code_to_patch++ = ((char *)&distance)[0];
        *code_to_patch++ = ((char *)&distance)[1];
        *code_to_patch++ = ((char *)&distance)[2];
        *code_to_patch = ((char *)&distance)[3];

}

/* IF THIS IS EVER CHANGED IN linux/arch/i386/rtlinux.h
   IT MUST BE CHANGED HERE TOO
   */
#define ACK_MAGIC 0x12344321
struct patch_table { char * address; unsigned long magic;};
struct func_table { char * address;};
extern const struct func_table *__start_rtlinux_funcs;
extern const struct func_table *__stop_rtlinux_funcs;

#ifdef CONFIG_X86_IO_APIC
/* our version of level-triggered IO APIC irq controller */
#if LINUX_VERSION_CODE >= 0x020300
static struct hw_interrupt_type *linux_ioapic_level_irq_type_ptr = 0;
static struct hw_interrupt_type rtl_ioapic_level_irq_type;

void static rtl_mask_and_ack_level_ioapic_irq(unsigned int i)
{
	linux_ioapic_level_irq_type_ptr->disable(i); /* mask */
	linux_ioapic_level_irq_type_ptr->end(i);	/* ack */
}

void static rtl_end_level_io_apic_irq(unsigned int i)
{
	linux_ioapic_level_irq_type_ptr->enable(i); /* unmask */
}
#endif

#endif /* CONFIG_X86_IO_APIC */

#ifdef CONFIG_X86_LOCAL_APIC

void start_apic_ack(void);
void end_apic_ack(void);
static void rtl_local_irq_controller_ack(void)
{
	asm __volatile__("start_apic_ack:\t\n");
        ack_APIC_irq();
	asm __volatile__("end_apic_ack:\t\n");
}

static inline char * match_ack(char *caller)
{
	const struct func_table *pfunc = (struct func_table *)&__start_rtlinux_funcs;
	struct patch_table *p = (struct patch_table *)pfunc[pf___start_rtlinux_patch].address;
	char *closest=0;
	unsigned dis=0x7fffffff; /* distance */

/* 	printk("searching %x\n", (unsigned) caller); */
       	for(; (ulong)p < (ulong)pfunc[pf___stop_rtlinux_patch].address;p++) {
		if (p->magic != ACK_MAGIC) {
			continue;
		} else {
/* 			printk("%x, %x, %d\n", p->address, caller, dis); */
			if ((unsigned) p->address >= (unsigned) caller &&
				dis > ((unsigned)p->address - (unsigned)caller)) {
			dis = (ulong)p->address - (ulong)caller;
			closest = p->address;
			}
		}
	}
	return closest;
}

static void zap_ack_apic(char *apic_caller){
	int i;
	int ack_len = (int) end_apic_ack  - (int)start_apic_ack;
	char *call_point = match_ack(apic_caller);
	if(call_point){
#ifdef DEBUG_RTLINUX
		printk("RTLinux debug zapping %x at %x\n",(unsigned int)apic_caller,(unsigned int)call_point);
#endif
		for(i=0; i<ack_len; i++)
			call_point[i]= 0x90;
	}
}

static void unzap_ack_apic(char *apic_caller){
	int i;
	int ack_len = (int) end_apic_ack  - (int)start_apic_ack;
	char *call_point = match_ack(apic_caller);
	if(call_point){
#ifdef DEBUG_RTLINUX
		printk("RTLinux debug unzapping %x at %x\n",(unsigned int)apic_caller,(unsigned int)call_point);
#endif
		for(i=0; i<ack_len; i++)
			call_point[i]= ((char *)start_apic_ack)[i];
	}
}

/* i8259.c
BUILD_SMP_INTERRUPT(reschedule_interrupt,RESCHEDULE_VECTOR)
BUILD_SMP_INTERRUPT(invalidate_interrupt,INVALIDATE_TLB_VECTOR)
BUILD_SMP_INTERRUPT(call_function_interrupt,CALL_FUNCTION_VECTOR)
BUILD_SMP_INTERRUPT(spurious_interrupt,SPURIOUS_APIC_VECTOR)
BUILD_SMP_INTERRUPT(error_interrupt,ERROR_APIC_VECTOR)
BUILD_SMP_TIMER_INTERRUPT(apic_timer_interrupt,LOCAL_TIMER_VECTOR)
*/

void init_local_code(const struct func_table *pf)
{
#ifdef CONFIG_SMP
	local_code.smp_reschedule_interrupt =\
	       	(void *)pf[pf_smp_reschedule_interrupt].address;
	local_code.smp_invalidate_interrupt = \
	       	(void *)pf[pf_smp_invalidate_interrupt].address;
	local_code.smp_call_function_interrupt = \
	       	(void *)pf[pf_smp_call_function_interrupt].address;
	local_code.rtl_reschedule = \
	       	(void *)pf[pf_rtl_reschedule].address;
#endif
	local_code.smp_spurious_interrupt=\
	       	(void *)pf[pf_smp_spurious_interrupt].address;
	local_code.smp_error_interrupt=\
	       	(void *)pf[pf_smp_error_interrupt].address;
        local_code.smp_apic_timer_interrupt = \
	       	(void *)pf[pf_smp_apic_timer_interrupt].address;
}

#endif /* CONFIG_X86_LOCAL_APIC */

static char * find_patch(unsigned long v )
{
       	const struct func_table *pfunc = (struct func_table *)&__start_rtlinux_funcs;
       	struct patch_table *p = (struct patch_table *)pfunc[pf___start_rtlinux_patch].address;
       	for(; (ulong)p < (ulong)pfunc[pf___stop_rtlinux_patch].address;p++) {
	       	if(((unsigned long)p->magic) == v) {
		       	return p->address;
		}
	}
       	return 0;
}


int rtl_request_traps(int (*rtl_exception_intercept)(int vector, struct pt_regs *regs, int error_code))
{
	const struct func_table *pfunc = (struct func_table *)&__start_rtlinux_funcs;
	*((long *)(pfunc[pf_rtl_exception_intercept].address)) = (long)rtl_exception_intercept;
	return 0;
}

static rtl_local_handler_t rtl_reschedule_handlers[NR_CPUS];

unsigned int default_reschedule_handler(struct pt_regs *r)
{
	return 0;
}


static int patch_kernel (unsigned int cpu_id){
	enum pfunctions i;
	char *p;
	irq_desc_t *h;
	const struct func_table *pfunc = (struct func_table *)&__start_rtlinux_funcs;
	/* make my func the pfunc */
	/* rtl_intercept */

	xdo_IRQ = (void *)pfunc[pf_do_IRQ].address; /*do_IRQ */
        local_ret_from_intr = (void *)pfunc[pf_ret_from_intr].address;
 	if( !(p = find_patch((ulong)pfunc[pf_do_IRQ].address)))
	{
		printk("RTLinux cannot patch intercept routine\n");
		return -1;
	}
	else {
#ifdef DEBUG_RTLINUX
		printk("RTLinux patch intercept=%x\n",(int)rtl_intercept);
#endif
		save_jump(p,pf_do_IRQ);
		patch_jump(p,(char *)rtl_intercept);
	}

	
	/* insert call to sti */
#ifdef DEBUG_RTLINUX
	printk("RTLinux patch kernel  skip emulate IRET DEBUG\n");
#endif
	*((long *)(pfunc[pf_rtl_emulate_iret].address)) = (long)rtl_soft_sti;

#ifdef CONFIG_X86_LOCAL_APIC 
	/* patch the calls to the smp handlers and zap their apic calls */
	for(i=PF_LOCAL_START; i <= PF_LOCAL_END;i++){
		p = find_patch((ulong)pfunc[i].address);
		if(!p){
			printk("RTLinux can't smp patch %d\n",i);
			return -1;
		}
		else{
			save_jump(p,i);
#ifdef DEBUG_RTLINUX
			printk("patching %x, jmp at %#x\n", (unsigned) (pfunc[i].address), (unsigned) p);
#endif
			patch_jump(p,(char *)rtl_local_intercept);
			zap_ack_apic(pfunc[i].address);
		}
	}
	init_local_code(pfunc);

#endif

	/* cli/sti etc */
	pre_patch_control = irq_control; /* save old irq_control */
	irq_control.do_save_flags = rtl_soft_save_flags;
	irq_control.do_restore_flags = rtl_soft_restore_flags;
	irq_control.do_cli = rtl_soft_cli;
	irq_control.do_sti = rtl_soft_sti;
	irq_control.do_local_irq_save = rtl_soft_local_irq_save;
	irq_control.do_local_irq_restore = rtl_soft_restore_flags;

	/* take over the descriptor table */
	h = (irq_desc_t *)pfunc[pf_irq_desc].address;
	for(i=0; i < NR_IRQS; i++){
		save_linux_irq_desc[i] = h[i] . handler;
#ifdef CONFIG_X86_IO_APIC
#if LINUX_VERSION_CODE >= 0x020300
		if (h[i].handler && !strcmp(h[i].handler->typename, "IO-APIC-level")) {
			if (!linux_ioapic_level_irq_type_ptr) {
				linux_ioapic_level_irq_type_ptr = h[i].handler;
				rtl_ioapic_level_irq_type = *linux_ioapic_level_irq_type_ptr;
				rtl_ioapic_level_irq_type . ack = rtl_mask_and_ack_level_ioapic_irq;
				rtl_ioapic_level_irq_type . end = rtl_end_level_io_apic_irq;
			}
			rtl_irq_desc[i]= &rtl_ioapic_level_irq_type;
		} else
#endif
#ifdef CONFIG_SMP
		if (i == RTL_RESCHEDULE_VECTOR - 0x20) {
			rtl_irq_desc[i] = &rtl_resched_irq_type;
		} else 
#endif
#endif
			rtl_irq_desc[i]= h[i].handler;

		h[i].handler = &rtl_generic_type;
	}

#ifdef DEBUG_RTLINUX
	printk("RTLinux  patch kernel done \n");
#endif
	return 0;


}

inline static int unpatch_kernel(void){ 
	irq_desc_t *h;
	int i;
       	const struct func_table *pfunc = (struct func_table *)&__start_rtlinux_funcs;
	char *p;

	*((long *)(pfunc[pf_rtl_exception_intercept].address)) = (long)0;

	irq_control=	pre_patch_control;
	*((long *)(pfunc[pf_rtl_emulate_iret].address)) = 0;
	/* replace the descriptor table */
	h = (irq_desc_t *)pfunc[pf_irq_desc].address;
	for(i=0; i < NR_IRQS; i++){
		h[i].handler = save_linux_irq_desc[i];
	}
/* 	rtl_hard_sti();		   */
	/*unpatch jump do_IRQ */
 	if( !(p = find_patch((ulong)pfunc[pf_do_IRQ].address)))
	{
		printk("RTLinux cannot unpatch patch intercept routine\n");
		return -1;
	}
	else { unpatch_jump(p,pf_do_IRQ); }

#ifdef CONFIG_X86_LOCAL_APIC
	for(i=PF_LOCAL_START; i <= PF_LOCAL_END;i++){
		p = find_patch((ulong)pfunc[i].address);
		if(!p){
			printk("RTLinux can't smp unpatch %d\n",i);
			return -1;
		}
		else{
#ifdef DEBUG_RTLINUX
			printk("unpatching %x, jmp at %#x\n", (unsigned) (pfunc[i].address), (unsigned) p);
#endif
			unpatch_jump(p,i);
			unzap_ack_apic(pfunc[i].address);
		}
	}
#endif

	return 0;
	
}

/* WARNING this fails on more than bits-per-word cpus machines
   Stephen, don't try this on your desktop Galaxy
 */
struct { atomic_t  waiting_with_cli;
	atomic_t done_patch;
	atomic_t waiting_for_unpatch;
	atomic_t done_unpatch;} sync_data = {{0},{0},{0},{0}};
void sync_on(void *unused)
{
	int i;
	rtl_hard_cli();
	atomic_inc(&sync_data.waiting_with_cli);
	i = 0;
	while (!atomic_read(&sync_data.done_patch) && i < 1000000000) {
		i++;
	}
	if (i == 1000000000) {
		printk("timed out on sync_data.done_patch\n");
	}
	rtl_hard_sti();/* and never hard cli again in linux mode */
}

void sync_off(void *unused){
	unsigned long f;
	int i;
	save_flags(f);
	rtl_hard_cli();
	atomic_inc(&sync_data.waiting_for_unpatch);
	i = 0;
	while(!atomic_read(&sync_data.done_unpatch) && i < 1000000000) {
		i++;
	}
	if (i == 1000000000) {
		printk("timed out on sync_data.done_unpatch\n");
	}
	mb();
	restore_flags(f); /* this is hard restore now! */
#ifdef DEBUG_RTLINUX
	printk("sync off returned\n");
#endif
}


#ifdef CONFIG_SMP
int rtl_smp_synchronize( void (*sync_func)(void *),atomic_t  *waitpt)
{
	int error;
	int cpus = smp_num_cpus - 1;
	int timeout;

	error= smp_call_function (sync_func, 0, 0 /*atomic */,0 /*don't wait*/);
#ifdef DEBUG_RTLINUX
	printk("smp_call_function returned\n");
#endif
	if(error) {
		printk("Cannot install due to smp call error %x\n",error);
		return -1;
	}
	/* everyone else is now starting to exec sync_function */
	timeout = jiffies + HZ;
	while ((atomic_read(waitpt) != cpus)
			&& time_before(jiffies, timeout));
	if(atomic_read(waitpt) != cpus){
	       printk("rtl_smp_synchronize timed out\n");
       	       return -1;
	}
	/* now all cpus have disabled interrupts and are waiting completion */
	return 0;
}
#else 
#define rtl_smp_synchronize(a,b) 0
#endif

unsigned int rtl_reschedule_interrupt(unsigned int irq, struct pt_regs *r);
/* Start the RTLinux operation */
/*TODO Must test smp synchronization!!! */
inline int arch_takeover(void)
{
int i;
DeclareAndInit(cpu_id);

rtl_hard_cli(); 
if(G_TEST_AND_SET(g_initialized)){ 
	printk("Can't lock to install RTL. Already installed?\n");
	rtl_hard_sti();
	return -1;
}
#ifdef DEBUG_RTLINUX
	printk("RTLinux takeover ok1\n");
#endif
	if( rtl_smp_synchronize(sync_on,&sync_data.waiting_with_cli)) return -1;

#ifdef DEBUG_RTLINUX
	printk("RTLinux takeover ok2\n");
#endif
/* initialize the main RTLinux structures */
rtl_global.flags = (1<<g_initialized);

for(i = 0; i < NR_CPUS; i++){
	rtl_local[i].flags = POS_TO_BIT(l_ienable) | POS_TO_BIT(l_idle);
	rtl_reschedule_handlers[i] = &default_reschedule_handler;
}

#ifdef DEBUG_RTLINUX
	printk("RTLinux takeover ok3\n");
#endif
patch_kernel(cpu_id); 
#ifdef CONFIG_SMP
barrier();
atomic_inc(&sync_data.done_patch);
mb();
#endif
rtl_hard_sti();
rtl_soft_sti();
#ifdef CONFIG_SMP
rtl_request_global_irq (RTL_RESCHEDULE_VECTOR - 0x20, rtl_reschedule_interrupt);
#endif

return 0;
}


/*TODO on a failure, we should refuse to remove the module! */
inline void arch_giveup(void)
{
	/* TODO test sync*/
unsigned long flags;

#ifdef CONFIG_SMP
rtl_free_global_irq (RTL_RESCHEDULE_VECTOR - 0x20);
#endif
save_flags(flags);
if(!G_TEST_AND_CLEAR(g_initialized)){ 
	printk("Can't uninstall RTL. Not installed?\n");
	return ;
}
#ifdef DEBUG_RTLINUX
	printk("RTLinux giveup ok1\n");
#endif
if(rtl_smp_synchronize(sync_off,&sync_data.waiting_for_unpatch) ){
       printk("Cannot synchronize RTLinux so cannot unpatch!\n\
	       Save all files, give to the poor, reboot\n");
	       return;
}
#ifdef DEBUG_RTLINUX
	printk("RTLinux giveup ok2\n");
#endif
rtl_hard_cli(); 
unpatch_kernel(); /* machine dependent */
#ifdef CONFIG_SMP
barrier();
atomic_inc(&sync_data.done_unpatch);
mb();
#endif
restore_flags(flags); /* hard restore now */
}

#ifdef __LOCAL_IRQS__
int rtl_request_local_irq(int i, unsigned int (*handler)(struct pt_regs *r),
			  unsigned int cpu_id)
{
	if (i == LOCAL_TIMER_VECTOR || i == RTL_RESCHEDULE_VECTOR) {
		rtl_local[cpu_id].rt_handlers[VECTOR_TO_LOCAL_PND(i)] = handler;
		L_SET_RTH(VECTOR_TO_LOCAL_PND(i));
		return 0;
	}
	return -EINVAL;
}

int rtl_free_local_irq(int i, unsigned int cpu_id)
{
	if (i == LOCAL_TIMER_VECTOR || i == RTL_RESCHEDULE_VECTOR) {
		L_CLEAR_RTH(VECTOR_TO_LOCAL_PND(i));
		return 0;
	}
	return -EINVAL;
}
#endif

#ifdef CONFIG_SMP

unsigned int rtl_reschedule_interrupt(unsigned int irq, struct pt_regs *r)
{
	int cpu_id = rtl_getcpuid();

/* 	rtl_printf("resched on %x\n", cpu_id); */
	rtl_reschedule_handlers[cpu_id](r);
	return 0;
}

void rtl_reschedule(unsigned int cpu)
{
	local_code.rtl_reschedule(cpu);
}

int rtl_request_ipi (unsigned int (*f)(struct pt_regs *r), int cpu)
{
	rtl_reschedule_handlers[cpu] = f;
/* 	return rtl_request_local_irq (RTL_RESCHEDULE_VECTOR, rtl_reschedule_interrupt, cpu); */
	return 0;
}

int rtl_free_ipi (int cpu)
{
	rtl_reschedule_handlers[cpu] = &default_reschedule_handler;
/* 	return rtl_free_local_irq (RTL_RESCHEDULE_VECTOR, cpu); */
	return 0;
}
#endif
