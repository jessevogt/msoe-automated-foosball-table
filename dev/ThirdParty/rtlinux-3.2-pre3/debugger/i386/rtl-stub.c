/*
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * RTLinux Debugger modifications are written by Michael Barabanov
 * (baraban@fsmlabs.com) and
 * are Copyright (C) 2000, Finite State Machine Labs Inc.
 *
 */
 
/****************************************************************************
 *  Header: remcom.c,v 1.34 91/03/09 12:29:49 glenne Exp $
 *
 *  Module name: remcom.c $
 *  Revision: 1.34 $
 *  Date: 91/03/09 12:29:49 $
 *  Contributor:     Lake Stevens Instrument Division$
 *
 *  Description:     low level support for gdb debugger. $
 *
 *  Considerations:  only works on target hardware $
 *
 *  Written by:      Glenn Engel $
 *  Updated by:	     David Grothe <dave@gcom.com>
 *  ModuleState:     Experimental $
 *
 *  NOTES:           See Below $
 *
 *  Modified for 386 by Jim Kingdon, Cygnus Support.
 *  Compatibility with 2.1.xx kernel by David Grothe <dave@gcom.com>
 *  Integrated into 2.2.5 kernel by Tigran Aivazian <tigran@sco.com>
 *  Modified for RTLinux by Michael Barabanov (baraban@fsmlabs.com)
 *
 *  To enable debugger support, two things need to happen.  One, a
 *  call to set_debug_traps() is necessary in order to allow any breakpoints
 *  or error conditions to be properly intercepted and reported to gdb.
 *  Two, a breakpoint needs to be generated to begin communication.  This
 *  is most easily accomplished by a call to breakpoint().  Breakpoint()
 *  simulates a breakpoint by executing an int 3.
 *
 *************
 *
 *    The following gdb commands are supported:
 *
 * command          function                               Return value
 *
 *    g             return the value of the CPU registers  hex data or ENN
 *    G             set the value of the CPU registers     OK or ENN
 *
 *    mAA..AA,LLLL  Read LLLL bytes at address AA..AA      hex data or ENN
 *    MAA..AA,LLLL: Write LLLL bytes at address AA.AA      OK or ENN
 *
 *    c             Resume at current address              SNN   ( signal NN)
 *    cAA..AA       Continue at address AA..AA             SNN
 *
 *    s             Step one instruction                   SNN
 *    sAA..AA       Step one instruction from AA..AA       SNN
 *
 *    k             kill
 *
 *    ?             What was the last sigval ?             SNN   (signal NN)
 *
 * All commands and responses are sent with a packet which includes a
 * checksum.  A packet consists of
 *
 * $<packet info>#<checksum>.
 *
 * where
 * <packet info> :: <characters representing the command or response>
 * <checksum>    :: < two hex digits computed as modulo 256 sum of <packetinfo>>
 *
 * When a packet is received, it is first acknowledged with either '+' or '-'.
 * '+' indicates a successful transfer.  '-' indicates a failed transfer.
 *
 * Example:
 *
 * Host:                  Reply:
 * $m0,10#2a               +$00010203040506070809101112131415#42
 *
 ****************************************************************************/

#include <linux/string.h>
#include <linux/kernel.h>
#include <asm/vm86.h>
#include <asm/system.h>
#include <asm/ptrace.h>			/* for linux pt_regs struct */
#include <linux/signal.h>

/* RTLinux support */
#define __NO_VERSION__
#include <linux/module.h>

#include <rtl_sync.h>


#include <rtl_sched.h>


#define strtoul simple_strtoul

#include <psc.h>


#define rtl_running_linux() (pthread_self() == &LOCAL_SCHED->rtl_linux_task)

int rtl_debug_initialized = 0;


/* end of RTLinux support */

/************************************************************************
 *
 * external low-level support routines
 */
typedef void (*Function)(void);           /* pointer to a function */

extern int putDebugChar(int);   /* write a single character      */
extern int getDebugChar(void);   /* read and return a single char */

/************************************************************************/
/* BUFMAX defines the maximum number of characters in inbound/outbound buffers*/
/* at least NUMREGBYTES*2 are needed for register packets */
#define BUFMAX 800

static int     remote_debug = 0;
/*  debug >  0 prints ill-formed commands in valid packets & checksum errors */


static const char hexchars[]="0123456789abcdef";

/* Number of bytes of registers.  */
#define NUMREGBYTES 64
/*
 * Note that this register image is in a different order than
 * the register image that Linux produces at interrupt time.
 *
 * Linux's register image is defined by struct pt_regs in ptrace.h.
 * Just why GDB uses a different order is a historical mystery.
 */
enum regnames {_EAX,		/* 0 */
	       _ECX,		/* 1 */
	       _EDX,		/* 2 */
	       _EBX,		/* 3 */
	       _ESP,		/* 4 */
	       _EBP,		/* 5 */
	       _ESI,		/* 6 */
	       _EDI,		/* 7 */
	       _PC 		/* 8 also known as eip */,
	       _PS		/* 9 also known as eflags */,
	       _CS,		/* 10 */
	       _SS,		/* 11 */
	       _DS,		/* 12 */
	       _ES,		/* 13 */
	       _FS,		/* 14 */
	       _GS};		/* 15 */



/***************************  ASSEMBLY CODE MACROS *************************/
/* 									   */


/* Put the error code here just in case the user cares.  */
int gdb_i386errcode;
/* Likewise, the vector number here (since GDB only gets the signal
   number through the usual means, and that's not very specific).  */
int gdb_i386vector = -1;





int hex(char ch)
{
  if ((ch >= 'a') && (ch <= 'f')) return (ch-'a'+10);
  if ((ch >= '0') && (ch <= '9')) return (ch-'0');
  if ((ch >= 'A') && (ch <= 'F')) return (ch-'A'+10);
  return (-1);
}


/* scan for the sequence $<data>#<checksum>     */
void getpacket(char * buffer)
{
  unsigned char checksum;
  unsigned char xmitcsum;
  int  i;
  int  count;
  char ch;

  do {
    /* wait around for the start character, ignore all other characters */
    while ((ch = (getDebugChar() & 0x7f)) != '$');
    checksum = 0;
    xmitcsum = -1;

    count = 0;

    /* now, read until a # or end of buffer is found */
    while (count < BUFMAX) {
      ch = getDebugChar() & 0x7f;
      if (ch == '#') break;
      checksum = checksum + ch;
      buffer[count] = ch;
      count = count + 1;
      }
    buffer[count] = 0;

    if (ch == '#') {
      xmitcsum = hex(getDebugChar() & 0x7f) << 4;
      xmitcsum += hex(getDebugChar() & 0x7f);
      if ((remote_debug ) && (checksum != xmitcsum)) {
        printk ("bad checksum.  My count = 0x%x, sent=0x%x. buf=%s\n",
		 checksum,xmitcsum,buffer);
      }

      if (checksum != xmitcsum) putDebugChar('-');  /* failed checksum */
      else {
	 putDebugChar('+');  /* successful transfer */
	 /* if a sequence char is present, reply the sequence ID */
	 if (buffer[2] == ':') {
	    putDebugChar( buffer[0] );
	    putDebugChar( buffer[1] );
	    /* remove sequence chars from buffer */
	    count = strlen(buffer);
	    for (i=3; i <= count; i++) buffer[i-3] = buffer[i];
	 }
      }
    }
  } while (checksum != xmitcsum);

  if (remote_debug)
    printk("R:%s\n", buffer) ;
}

/* send the packet in buffer.  */


void putpacket(char * buffer)
{
  unsigned char checksum;
  int  count;
  char ch;

  /*  $<packet info>#<checksum>. */
  do {
  if (remote_debug)
    printk("T:%s\n", buffer) ;
  putDebugChar('$');
  checksum = 0;
  count    = 0;

  while ((ch=buffer[count])) {
    if (! putDebugChar(ch)) return;
    checksum += ch;
    count += 1;
  }

  putDebugChar('#');
  putDebugChar(hexchars[checksum >> 4]);
  putDebugChar(hexchars[checksum % 16]);

  } while ((getDebugChar() & 0x7f) != '+');

}

static char  remcomInBuffer[BUFMAX];
static char  remcomOutBuffer[BUFMAX];
static short error;


void debug_error( char * format, char * parm)
{
  if (remote_debug) printk (format,parm);
}

static void print_regs(struct pt_regs *regs)
{
    printk("EAX=%08lx ", regs->eax);
    printk("EBX=%08lx ", regs->ebx);
    printk("ECX=%08lx ", regs->ecx);
    printk("EDX=%08lx ", regs->edx);
    printk("\n");
    printk("ESI=%08lx ", regs->esi);
    printk("EDI=%08lx ", regs->edi);
    printk("EBP=%08lx ", regs->ebp);
    printk("ESP=%08lx ", (long) (regs+1));
    printk("\n");
    printk(" DS=%08x ", regs->xds);
    printk(" ES=%08x ", regs->xes);
    printk(" SS=%08x ", __KERNEL_DS);
    printk(" FL=%08lx ", regs->eflags);
    printk("\n");
    printk(" CS=%08x ", regs->xcs);
    printk(" IP=%08lx ", regs->eip);
#if 0
    printk(" FS=%08x ", regs->fs);
    printk(" GS=%08x ", regs->gs);
#endif
    printk("\n");


} /* print_regs */

static void regs_to_gdb_regs(int *gdb_regs, struct pt_regs *regs)
{
    gdb_regs[_EAX] =  regs->eax;
    gdb_regs[_EBX] =  regs->ebx;
    gdb_regs[_ECX] =  regs->ecx;
    gdb_regs[_EDX] =  regs->edx;
    gdb_regs[_ESI] =  regs->esi;
    gdb_regs[_EDI] =  regs->edi;
    gdb_regs[_EBP] =  regs->ebp;
    gdb_regs[ _DS] =  regs->xds;
    gdb_regs[ _ES] =  regs->xes;
    gdb_regs[ _PS] =  regs->eflags;
    gdb_regs[ _CS] =  regs->xcs;
    gdb_regs[ _PC] =  regs->eip;
    gdb_regs[_ESP] =  (int) (&regs->esp) ;
    gdb_regs[ _SS] =  __KERNEL_DS;
    gdb_regs[ _FS] =  0xFFFF;
    gdb_regs[ _GS] =  0xFFFF;
} /* regs_to_gdb_regs */

static void gdb_regs_to_regs(int *gdb_regs, struct pt_regs *regs)
{
    regs->eax	=     gdb_regs[_EAX] ;
    regs->ebx	=     gdb_regs[_EBX] ;
    regs->ecx	=     gdb_regs[_ECX] ;
    regs->edx	=     gdb_regs[_EDX] ;
    regs->esi	=     gdb_regs[_ESI] ;
    regs->edi	=     gdb_regs[_EDI] ;
    regs->ebp	=     gdb_regs[_EBP] ;
    regs->xds	=     gdb_regs[ _DS] ;
    regs->xes	=     gdb_regs[ _ES] ;
    regs->eflags=     gdb_regs[ _PS] ;
    regs->xcs	=     gdb_regs[ _CS] ;
    regs->eip	=     gdb_regs[ _PC] ;
#if 0					/* can't change these */
    regs->esp	=     gdb_regs[_ESP] ;
    regs->xss	=     gdb_regs[ _SS] ;
    regs->fs	=     gdb_regs[ _FS] ;
    regs->gs	=     gdb_regs[ _GS] ;
#endif

} /* gdb_regs_to_regs */

/* Indicate to caller of mem2hex or hex2mem that there has been an
   error.  */
static volatile int real_mem_err [NR_CPUS];
static volatile int real_mem_err_expected [NR_CPUS];

#define mem_err (real_mem_err[rtl_getcpuid()])
#define mem_err_expected (real_mem_err_expected[rtl_getcpuid()])
static          int garbage_loc = -1 ;

int
get_char (char *addr)
{
  return *addr;
}

void
set_char ( char *addr, int val)
{
  *addr = val;
}

/* Convert the memory pointed to by mem into hex, placing result in buf.
 * Return a pointer to the last char put in buf (null), in case of mem fault,
 * return 0.
 */
char* mem2hex( char* mem, char* buf, int   count)
	       
{
      int i;
      unsigned char ch;
      int may_fault = 1;

      if (may_fault)
      {
	  mem_err_expected = 1 ;
	  mem_err = 0 ;
      }
      for (i=0;i<count;i++) {
	  /* printk("%lx = ", mem) ; */

	  ch = get_char (mem++);

	  /* printk("%02x\n", ch & 0xFF) ; */
	  if (may_fault && mem_err)
	  {
	    if (remote_debug)
		printk("Mem fault fetching from addr %lx\n", (long)(mem-1));
	    *buf = 0 ;				/* truncate buffer */
	    return 0;
	  }
          *buf++ = hexchars[ch >> 4];
          *buf++ = hexchars[ch % 16];
      }
      *buf = 0;
      if (may_fault)
	  mem_err_expected = 0 ;
      return(buf);
}

/* convert the hex array pointed to by buf into binary to be placed in mem */
/* return a pointer to the character AFTER the last byte written */
char* hex2mem( char* buf,
	       char* mem,
	       int   count)

{
      int i;
      unsigned char ch;
      int may_fault = 1;

      if (may_fault)
      {
	  mem_err_expected = 1 ;
	  mem_err = 0 ;
      }
      for (i=0;i<count;i++) {
          ch = hex(*buf++) << 4;
          ch = ch + hex(*buf++);

	  set_char (mem++, ch);

	  if (may_fault && mem_err)
	  {
	    if (remote_debug)
		printk("Mem fault storing to addr %lx\n", (long)(mem-1));
	    return 0;
	  }
      }
      if (may_fault)
	  mem_err_expected = 0 ;
      return(mem);
}


/**********************************************/
/* WHILE WE FIND NICE HEX CHARS, BUILD AN INT */
/* RETURN NUMBER OF CHARS PROCESSED           */
/**********************************************/
int hexToInt(char **ptr, int *intValue)
{
    int numChars = 0;
    int hexValue;

    *intValue = 0;

    while (**ptr)
    {
        hexValue = hex(**ptr);
        if (hexValue >=0)
        {
            *intValue = (*intValue <<4) | hexValue;
            numChars ++;
        }
        else
            break;

        (*ptr)++;
    }

    return (numChars);
}


/*
 * This function does all command procesing for interfacing to gdb.
 *
 * NOTE:  The INT nn instruction leaves the state of the interrupt
 *        enable flag UNCHANGED.  That means that when this routine
 *        is entered via a breakpoint (INT 3) instruction from code
 *        that has interrupts enabled, then interrupts will STILL BE
 *        enabled when this routine is entered.  The first thing that
 *        we do here is disable interrupts so as to prevent recursive
 *        entries and bothersome serial interrupts while we are
 *        trying to run the serial port in polled mode.
 *
 * For kernel version 2.1.xx the cli() actually gets a spin lock so
 * it is always necessary to do a restore_flags before returning
 * so as to let go of that lock.
 */

/* more RTLinux support */
static spinlock_t bp_lock = SPIN_LOCK_UNLOCKED;

#define RTL_MAX_BP 1024
static struct bp_cache_entry {
	char *mem;
	unsigned char val;
	struct bp_cache_entry *next;
} bp_cache [RTL_MAX_BP];

static struct bp_cache_entry *cache_start = 0;

int insert_bp(char *mem)
{
	int i;
	struct bp_cache_entry *e;
	int old;
	char buf[3];

	if (!mem2hex(mem, buf, 1)) {
		return EINVAL; /* memory error */
	}

	old = strtoul(buf, 0, 16);

	for (e = cache_start; e; e = e->next) {
		if (e->mem == mem) {
			return EINVAL; /* already there */
		}
	}
	for (i = 0; i < RTL_MAX_BP; i++) {
		if (bp_cache[i].mem == 0) {
			break;
		}
	}
	if (i == RTL_MAX_BP) {
		return EINVAL; /* no space */
	}
	bp_cache[i] . val = old;
	bp_cache[i] . mem = mem;
	bp_cache[i] . next = cache_start;
	cache_start = &bp_cache[i];

	set_char (mem, 0xcc);
	return 0;
}


int remove_bp (char *mem)
{
	struct bp_cache_entry *e = cache_start;
	struct bp_cache_entry *f = 0;
	if (!e) {
		return EINVAL;
	}
	if (e->mem == mem) {
		cache_start = e->next;
		f = e;
	} else {
		for (; e->next; e = e->next) {
			if (e->next->mem == mem) {
				f = e->next;
				e->next = f->next;
				break;
			}
		}
	}
	if (!f) {
		return EINVAL;
	}

	mem_err_expected = 1;
	set_char (f->mem, f->val);
  	if (mem_err) {
		return EINVAL;
	}
	mem_err_expected = 0;
	return 0;
}

#define CONFIG_RTL_DEBUGGER_THREADS
#define CONFIG_RTL_DEBUGGER_Z_PROTOCOL

extern int rtl_send_exception_info;

int handle_exception(int exceptionVector,
		     int signo,
		     int err_code,
		     struct pt_regs *linux_regs)
{
  int    addr, length;
  char * ptr;
  int    newPC;
  unsigned long flags;
  int 			gdb_regs[NUMREGBYTES/4];
#define	regs	(*linux_regs)

#ifdef CONFIG_RTL_DEBUGGER_THREADS
  pthread_t current_thread = pthread_self();
#endif
  /*
   * If the entry is not from the kernel then return to the Linux
   * trap handler and let it process the interrupt normally.
   */
  if (user_mode(linux_regs) && !(rtl_is_psc_active())) {
    return(0);
  }

  rtl_hard_savef_and_cli(flags); /* shall we get a spinlock? */

  if (remote_debug)
  {
    unsigned long	*lp = (unsigned long *) &linux_regs ;

    printk("handle_exception(exceptionVector=%d, "
           "signo=%d, err_code=%d, linux_regs=%p)\n",
	    exceptionVector, signo, err_code, linux_regs) ;
    print_regs(&regs) ;
    printk("Stk: %8lx %8lx %8lx %8lx  %8lx %8lx %8lx %8lx\n",
    	   lp[0],lp[1],lp[2],lp[3],lp[4],lp[5],lp[6],lp[7]);
    printk("     %8lx %8lx %8lx %8lx  %8lx %8lx %8lx %8lx\n",
    	   lp[8],lp[9],lp[10],lp[11],lp[12],lp[13],lp[14],lp[15]);
    printk("     %8lx %8lx %8lx %8lx  %8lx %8lx %8lx %8lx\n",
    	   lp[16],lp[17],lp[18],lp[19],lp[20],lp[21],lp[22],lp[23]);
    printk("     %8lx %8lx %8lx %8lx  %8lx %8lx %8lx %8lx\n",
    	   lp[24],lp[25],lp[26],lp[27],lp[28],lp[29],lp[30],lp[31]);
  }

  switch (exceptionVector)
  {
  case 0:			/* divide error */
  case 1:			/* debug exception */
  case 2:			/* NMI */
  case 3:			/* breakpoint */
  case 4:			/* overflow */
  case 5:			/* bounds check */
  case 6:			/* invalid opcode */
  case 7:			/* device not available */
  case 8:			/* double fault (errcode) */
  case 10:			/* invalid TSS (errcode) */
  case 12:			/* stack fault (errcode) */
  case 16:			/* floating point error */
  case 17:			/* alignment check (errcode) */
  default:			/* any undocumented */
      break ;
  case 11:			/* segment not present (errcode) */
  case 13:			/* general protection (errcode) */
  case 14:			/* page fault (special errcode) */
      if (mem_err_expected)
      {
	  /*
	   * This fault occured because of the get_char or set_char
	   * routines.  These two routines use either eax of edx to
	   * indirectly reference the location in memory that they
	   * are working with.  For a page fault, when we return
	   * the instruction will be retried, so we have to make
	   * sure that these registers point to valid memory.
	   */
	  mem_err = 1 ;		/* set mem error flag */
	  mem_err_expected = 0 ;
	  regs.eax = (long) &garbage_loc ;	/* make valid address */
	  regs.edx = (long) &garbage_loc ;	/* make valid address */
	  if (remote_debug) printk("Return after memory error\n");
	  if (remote_debug) print_regs(&regs) ;
	  rtl_hard_restore_flags(flags) ;
	  return(1) ;
      }
      break ;
  }

  if (rtl_running_linux() && exceptionVector != 1  && exceptionVector != 3) {
	  rtl_hard_restore_flags(flags) ;
	  return 0; /* let linux handle it's own faults */
  }

  debugpr("exception: %x\n", exceptionVector);

  /* can't trace with interrupts disabled or in Linux mode */
  if (exceptionVector == 1 && (!(regs.eflags & 0x200) || rtl_running_linux())) {
          regs.eflags &= 0xfffffeff; /* clear trace bit */
	  rtl_hard_restore_flags(flags) ;
	  return(1) ;
  }

  /* disable breakpoints if they're hit with interrupts disabled or in Linux mode */
  if (exceptionVector == 3 && (!(regs.eflags & 0x200) || rtl_running_linux())) {
	  spin_lock (&bp_lock);
	  if (remove_bp (((char *) regs.eip) - 1) == 0) {
		  --regs.eip;
	  }
	  spin_unlock (&bp_lock);
	  rtl_hard_restore_flags(flags) ;
	  return(1) ;
  }

  /* ok, here we know pthread_self() is an RT-thread */
  pthread_cleanup_push (&rtl_exit_debugger, 0);
  rtl_enter_debugger(exceptionVector, (void *) regs.eip);

  debugpr("starting talk");
  gdb_i386vector  = exceptionVector;
  gdb_i386errcode = err_code ;

  while (1==1) {
    error = 0;
    remcomOutBuffer[0] = 0;
    if (test_and_clear_bit(0, &rtl_send_exception_info)) {
	strcpy(remcomInBuffer, "?");
    } else {
	    getpacket(remcomInBuffer);
    }
    switch (remcomInBuffer[0]) {
      case 'q' :
	 if (!strcmp(remcomInBuffer, "qOffsets") && text && data && bss && !rtl_is_psc_active()) {
		 sprintf(remcomOutBuffer, "Text=%x;Data=%x;Bss=%x",
			 (unsigned )text, (unsigned )data, (unsigned )bss);
	 }
#ifdef CONFIG_RTL_DEBUGGER_THREADS
	 if (!strcmp(remcomInBuffer, "qC")) {
		 sprintf(remcomOutBuffer, "QC%x", (unsigned )(pthread_self()));
	 } else if (!strncmp(remcomInBuffer, "qL", 2)) {
		 /* we assume we have a limit of 31 threads -- to fit in one packet */
		 char packethead[17];
		 pthread_t task;
		 int ntasks = 0;
		 int i;

		 strcpy(remcomOutBuffer, remcomInBuffer);

		for (i = 0; i < rtl_num_cpus(); i++) {
			 int cpu_id = cpu_logical_map (i);
			 schedule_t *s = &rtl_sched [cpu_id];

			 spin_lock (&s->rtl_tasks_lock);
			 task = s->rtl_tasks;
			 while (task != &s->rtl_linux_task && ntasks < 31) {
				 sprintf((remcomOutBuffer) + strlen(remcomOutBuffer),
						 "00000000%08x", (unsigned) task);
				 task = task->next;
				 ntasks++;
			 }
			 spin_unlock (&s->rtl_tasks_lock);
		}
		sprintf(packethead, "qM%02x%01x", ntasks, 1 /* done */);
		memcpy(remcomOutBuffer, packethead, strlen(packethead));
	 }
#endif
	 break;
#ifdef CONFIG_RTL_DEBUGGER_THREADS
      case 'H': if (/* remcomInBuffer[1] == 'c' ||*/ remcomInBuffer[1] == 'g') {
		       if (remcomInBuffer[2] == '-') {
			current_thread = (pthread_t) -strtoul(remcomInBuffer + 3, 0, 16);
		       } else  {
			current_thread = (pthread_t) strtoul(remcomInBuffer + 2, 0, 16);
		       }
			debugpr("Hc/g: %x", (unsigned) current_thread);
			strcpy(remcomOutBuffer, "OK");
		}
		break;

      case 'T':{
		       pthread_t thread;
		       if (remcomInBuffer[1] == '-') {
			thread = (pthread_t) -strtoul(remcomInBuffer + 2, 0, 16);
		       } else  {
			thread = (pthread_t) strtoul(remcomInBuffer + 1, 0, 16);
		       }
		       if (!pthread_kill(thread, 0)) {
			       strcpy(remcomOutBuffer, "OK");
		       } else {
			       strcpy(remcomOutBuffer, "ERROR");
		       }

		}
		break;
#endif
#ifdef CONFIG_RTL_DEBUGGER_Z_PROTOCOL
      case 'Z': case 'z' :
		{
			int type = remcomInBuffer[1] - '0';
			unsigned address = strtoul(remcomInBuffer + 3, 0, 16);
			int res;
			if (type != 0) {
				strcpy(remcomOutBuffer, "ERROR");
				break;
			}
			spin_lock (&bp_lock);
			if (remcomInBuffer[0] == 'Z') {
				res = insert_bp((char *)address);
			} else {
				remove_bp((char *)address);
				res = 0;
			}
			spin_unlock (&bp_lock);
			if (res) {
				strcpy(remcomOutBuffer, "ERROR");
			} else {
				strcpy(remcomOutBuffer, "OK");
			}

		}
		break;
#endif
      case '?' :
#ifdef CONFIG_RTL_DEBUGGER_THREADS
		sprintf(remcomOutBuffer, "T%02xthread:%x;", signo, (unsigned) pthread_self());

#else
		remcomOutBuffer[0] = 'S';
                   remcomOutBuffer[1] =  hexchars[signo >> 4];
                   remcomOutBuffer[2] =  hexchars[signo % 16];
                   remcomOutBuffer[3] = 0;
#endif
                 break;
      case 'd' : remote_debug = !(remote_debug);  /* toggle debug flag */
		 printk("Remote debug %s\n", remote_debug ? "on" : "off");
                 break;
      case 'g' : /* return the value of the CPU registers */
		regs_to_gdb_regs(gdb_regs, &regs) ;
#ifdef CONFIG_RTL_DEBUGGER_THREADS
		/* we cheat here; we only provide correct eip and esp */
		if (current_thread != pthread_self()) {
			gdb_regs[_ESP] = (int) current_thread->stack;
			gdb_regs[_EBP] = *(current_thread->stack + 6);
			gdb_regs[_PC] = *(current_thread->stack);
			debugpr("reg read for %x", (unsigned) current_thread);

		}
#endif
                mem2hex((char*) gdb_regs, remcomOutBuffer, NUMREGBYTES);
                break;
      case 'G' : /* set the value of the CPU registers - return OK */
                hex2mem(&remcomInBuffer[1], (char*) gdb_regs, NUMREGBYTES);
		gdb_regs_to_regs(gdb_regs, &regs) ;
                strcpy(remcomOutBuffer,"OK");
                break;

      /* mAA..AA,LLLL  Read LLLL bytes at address AA..AA */
      case 'm' :
		    /* TRY TO READ %x,%x.  IF SUCCEED, SET PTR = 0 */
                    ptr = &remcomInBuffer[1];
                    if (hexToInt(&ptr,&addr))
                        if (*(ptr++) == ',')
                            if (hexToInt(&ptr,&length))
                            {
                                ptr = 0;
                                if (!mem2hex((char*) addr, remcomOutBuffer, length)) {
				    strcpy (remcomOutBuffer, "E03");
				    debug_error ("memory fault\n", NULL);
				}
                            }

                    if (ptr)
                    {
		      strcpy(remcomOutBuffer,"E01");
		      debug_error("malformed read memory command: %s\n",remcomInBuffer);
		    }
	          break;

      /* MAA..AA,LLLL: Write LLLL bytes at address AA.AA return OK */
      case 'M' :
		    /* TRY TO READ '%x,%x:'.  IF SUCCEED, SET PTR = 0 */
                    ptr = &remcomInBuffer[1];
                    if (hexToInt(&ptr,&addr))
                        if (*(ptr++) == ',')
                            if (hexToInt(&ptr,&length))
                                if (*(ptr++) == ':')
                                {
                                    if (!hex2mem(ptr, (char*) addr, length)) {
					strcpy (remcomOutBuffer, "E03");
					debug_error ("memory fault\n", NULL);
				    } else {
				        strcpy(remcomOutBuffer,"OK");
				    }

                                    ptr = 0;
                                }
                    if (ptr)
                    {
		      strcpy(remcomOutBuffer,"E02");
		      debug_error("malformed write memory command: %s\n",remcomInBuffer);
		    }
                break;

     /* cAA..AA    Continue at address AA..AA(optional) */
     /* sAA..AA   Step one instruction from AA..AA(optional) */
     case 'c' :
     case 's' :
          /* try to read optional parameter, pc unchanged if no parm */
         ptr = &remcomInBuffer[1];
         if (hexToInt(&ptr,&addr))
	 {
	     if (remote_debug)
		printk("Changing EIP to 0x%x\n", addr) ;

             regs.eip = addr;
	 }

          newPC = regs.eip ;

          /* clear the trace bit */
          regs.eflags &= 0xfffffeff;

          /* set the trace bit if we're stepping */
          if (remcomInBuffer[0] == 's') regs.eflags |= 0x100;

	  if (remote_debug)
	  {
	    printk("Resuming execution\n") ;
	    print_regs(&regs) ;
	  }
	  debugpr("cont\n");
	  goto cleanup;

      /* kill the program */
      case 'k' :
/* 	  last_module = 0; */
	  goto cleanup;
/*      default:
	  rtl_debug_handler(); */
      } /* switch */

    /* reply to the request */
    putpacket(remcomOutBuffer);
  }

cleanup: pthread_cleanup_pop(1);
  rtl_hard_restore_flags(flags) ;
  return(1) ;
#undef regs
}

static struct hard_trap_info
{
	unsigned int tt;		/* Trap type code for i386 */
	unsigned char signo;		/* Signal that we map this trap into */
} hard_trap_info[] = {
	{ 0,  SIGFPE },
	{ 1, SIGTRAP },
	{ 2, SIGSEGV },
	{ 3, SIGTRAP },
	{ 4, SIGSEGV },
	{ 5, SIGSEGV },
	{ 6, SIGILL },
	{ 7, SIGSEGV },
	{ 8, SIGSEGV },
	{ 9, SIGFPE },
	{ 10, SIGSEGV },
	{ 11, SIGBUS },
	{ 12, SIGBUS },
	{ 13, SIGSEGV },
	{ 17, SIGSEGV },
	{ 18, SIGSEGV },
	{ 19, SIGSEGV }
};

static int computeSignal(unsigned int tt)
{
	int i;

	for (i = 0; i < sizeof(hard_trap_info)/sizeof(hard_trap_info[0]); i++) {
		if (hard_trap_info[i].tt == tt) {
			return hard_trap_info[i].signo;
		}
	}

	return SIGHUP;         /* default for things we don't know about */
}

int rtl_debug_exception(int vector, struct pt_regs *regs, int error_code)
{
	int signo = computeSignal(vector);
/*	if (rtl_running_linux()) {
		rtl_cprintf("linux ");
	} else {
		rtl_cprintf("rt_thread ");
	}
	rtl_cprintf("vector=%d, eflags=%x, xcs=%x\n", vector, regs->eflags , regs->xcs);
	*/
	return handle_exception(vector, signo, error_code, regs);
}



