 /*
 * RTLinux signal support
 *
 * Written by Michael Barabanov
 * Copyright (C) Finite State Machine Labs Inc., 2000
 * Released under the terms of the GPL Version 2
 *
 *  Added user signals support - Dec, 2002 Josep Vidal <jvidal@disca.upv.es> (OCERA)
 * 
 */

#include <signal.h>
#include <errno.h>
#include <rtl_core.h>
#include <asm/irq.h>

/* Thread signals */ /* F. González & J. Vidal */
/* Array of sigactions. Thread signals from (7..31) */
struct sigaction rtl_sigact [RTL_SIGIRQMAX];
/* for compatibility issues */
#define sigact rtl_sigact

unsigned int rtl_sig_interrupt (unsigned int irq, struct pt_regs *regs)
{
	int sig = irq + RTL_SIGIRQMIN;
	if (sigact[sig].sa_handler != SIG_IGN && sigact[sig].sa_handler != SIG_DFL) {
		sigact[sig].sa_sigaction(sig, NULL, NULL);
	}
	/* shall we reenable here? */
	return 0;
}

/* Thread signals added. */ 
int sigaction(int sig, const struct sigaction *act, struct sigaction *oact)
{
#ifdef CONFIG_OC_PSIGNALS 
  int flags;
#endif

	int irq = -1;
	if (act && (act->sa_flags & SA_IRQ)) {
		irq = sig;
	} else {
	  /* for now, only hard global interrupts 
	     & thread signals from RTL_SIGNAL_READY+1 to  
	     RTL_MAX_SIGNAL 
	  */
	  if (sig <= RTL_SIGNAL_READY || (sig > RTL_MAX_SIGNAL && sig < RTL_SIGIRQMIN) || sig >= RTL_SIGIRQMIN + NR_IRQS) {
			errno = EINVAL;
			return -1;
		}
		irq = sig - RTL_SIGIRQMIN;
	}
	if (oact) {
		*oact = sigact[sig];
	  /*hard global interrupts */
	  if (sig > RTL_MAX_SIGNAL)
		rtl_irq_set_affinity(irq, NULL, &oact->sa_focus);
	}
	if (!act) {
		return 0;
	}

	/*hard global interrupts */
	if (sig > RTL_MAX_SIGNAL){
	if (sigact[sig].sa_handler != act->sa_handler) {
		/* free old irq first if needed */
		if (sigact[sig].sa_handler != SIG_IGN
				&& sigact[sig].sa_handler != SIG_DFL) {
	      /* 			rtl_printf("freeing %d\n", sig - RTL_SIGIRQMIN); */
			rtl_free_global_irq (irq);
		}
		/* now request */
		if (act->sa_handler != SIG_IGN && act->sa_handler != SIG_DFL) {
			sigact[sig] = *act;
	      /* 			rtl_printf("requesting %d\n", sig - RTL_SIGIRQMIN); */
			rtl_request_global_irq (irq, rtl_sig_interrupt);
		}
	}

	  if ( act->sa_flags & SA_FOCUS) {
		rtl_irq_set_affinity (irq, &act->sa_focus, NULL);
	}
	}	/*End hard global interrupts */

#ifdef CONFIG_OC_PSIGNALS 
	rtl_no_interrupts(flags);
#endif
	sigact[sig].sa_handler = act->sa_handler;

#ifdef CONFIG_OC_PSIGNALS 
	if (sig > RTL_SIGNAL_READY && sig <= RTL_MAX_SIGNAL ){
	  if (sigact[sig].sa_handler == SIG_IGN
	      || sigact[sig].sa_handler == SIG_DFL){
	    sigact[sig].sa_handler = NULL;
	  }
	}
#endif
	sigact[sig].sa_flags = act->sa_flags;
	sigact[sig].sa_focus = act->sa_focus;
	sigact[sig].sa_mask = act->sa_mask;
#ifdef CONFIG_OC_PSIGNALS 
	sigact[sig].owner = pthread_self();
	rtl_restore_interrupts(flags);
#endif	
	return 0;
}

#ifdef CONFIG_OC_PSIGNALS 
int pthread_sigmask(int how, const rtl_sigset_t *set, rtl_sigset_t *oset){
  pthread_t self=pthread_self();
  int err=0,flags;
  
  if (oset) *oset=self->blocked;

  if (!set) return EFAULT;
 
  /*
    With pthread_sigmask RTLINUX scheduler signals can't be blocked
    or unblocked. For this reason the RTL_THREAD_SIGNALS_MASK is 
    applied.
  */

  rtl_no_interrupts(flags);

  switch (how){
  case SIG_SETMASK:
    self->blocked=( self->blocked & ~RTL_THREAD_SIGNALS_MASK) | 
      (RTL_THREAD_SIGNALS_MASK & *set);
    break;
  case SIG_BLOCK:
    self->blocked=( self->blocked & ~RTL_THREAD_SIGNALS_MASK) | 
      (RTL_THREAD_SIGNALS_MASK & (self->blocked | *set));
    break;
  case SIG_UNBLOCK:
    self->blocked=( self->blocked & ~RTL_THREAD_SIGNALS_MASK) | 
      (RTL_THREAD_SIGNALS_MASK & (self->blocked & ~(*set)));
    break;
  default:
    rtl_restore_interrupts(flags);
    return EINVAL;
  }

  rtl_restore_interrupts(flags);
  return err;
} 


int sigsuspend(const rtl_sigset_t *sigmask){
  rtl_sigset_t oset;
  int flags;
  pthread_t self=pthread_self();

  if (!sigmask) {
    errno=EFAULT;
    return -1;
  }

  rtl_no_interrupts(flags);

  // Backup old mask.
  oset=self->blocked;

  /*
    Instaure new.

    With pthread_sigmask RTLINUX scheduler signals can't be blocked
    or unblocked.     
    0 .. 6 from oset, 7 .. 31 to 0 OR 
    0 .. 6 to 0, 7 .. 31 from sigmask    
  */
  self->blocked= (oset & ~RTL_THREAD_SIGNALS_MASK) | (*sigmask & RTL_THREAD_SIGNALS_MASK) ;
  // Mark as interrumpible by a signal.
  set_bit(RTL_THREAD_SIGNAL_INTERRUMPIBLE,&self->threadflags);
  // Suspend calling thread until a signal arrives.
    RTL_MARK_SUSPENDED (self);
    rtl_restore_interrupts(flags);
    rtl_schedule();
    pthread_testcancel();
  
  // do_user_signal clears the bit RTL_THREAD_SIGNAL_INTERRUMPIBLE from threadflags.
  if (!test_and_clear_bit(RTL_THREAD_SIGNAL_INTERRUMPIBLE,&self->threadflags)){
    errno=EINTR;
  }

  // Restore thread blocked set.
  self->blocked=oset;
 
  return -1;
}

int sigpending(rtl_sigset_t *set){
  if (set){
    *set=pthread_self()->pending;
	return 0;
  } else {
    errno=EFAULT;
    return -1;
  }
    
}

#endif	
















































































