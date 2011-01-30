/*
 *  rt_ipc.c -- intertask communication primitives for Real-Time Linux
 *
 *  Copyright (C) 1997 Jerry Epplin.  All rights reserved.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  History:
 *   17-Jul-97 jhe  V0.1 Original.
 *   28-Jul-97 jhe  V0.2 Timeouts on semaphores.  Message queues.
 *   15-Aug-97 jhe  V0.3 rt_ipc fifos.  Modified semantics of timeouts.
 */

#define IPC_VERSION "0.3"

#include <linux/module.h>
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/errno.h>

#include <asm/system.h>
#include <linux/malloc.h>
#include <rtl_sched.h>
#include <rtl_sync.h>
#include <rtl_fifo.h>
#include <asm/rt_irq.h>

#include "rt_ipc.h"

extern int rtl_schedule(void);

#include <rtl_sync.h>
#ifdef CONFIG_SMP
#error rt_ipc does not work on SMP yet
extern spinlock_t fifo_spinlock;
#define RTL_SPIN_LOCK fifo_spinlock
#endif

#define IPC_DATA_INDEX 0
/*************************************************************************
 * rt_sem_init -- initialize a real-time semaphore
 *
 * Called to initialize a real-time semaphore.  'sem' must point to a
 * statically allocated structure.  'type' is RT_SEM_BINARY or 
 * RT_SEM_COUNTING.  'init_val' is the initial value of the semaphore
 * (usually 0).
 *
 * Returns 0 if successful, -EINVAL if called incorrectly.
 *************************************************************************/
int rt_sem_init(rt_sem_t *sem, RT_SEMTYPE type, int init_val)
{
  int ret = 0;
  if (init_val < 0 || (type == RT_SEM_BINARY && init_val > 1))
    ret = -EINVAL;		/* binary sem must have 0 or 1 */
  else
  {
    sem->magic = RT_SEM_MAGIC;
    sem->val = init_val;
    sem->type = type;
    sem->wait_list = NULL;
  }
  return ret;
}

/*************************************************************************
 * rt_sem_destroy -- remove a real-time semaphore
 *
 * Removes a semaphore previously created with rt_sem_init().  Semaphore
 * deletion safety is implemented; i.e., any tasks blocked on this 
 * semaphore when it is destroyed are allowed to run.
 *
 * Returns 0 if successful, -EINVAL if 'sem' is not a valid rt_sem_t.
 *************************************************************************/
int rt_sem_destroy(rt_sem_t *sem)
{
  int ret = 0;
  if (sem->magic != RT_SEM_MAGIC)
    ret = -EINVAL;
  else
    /* unblock any tasks blocked on this sem */
    while (sem->val < 0)
      rt_sem_post(sem);
  return ret;
}

/*************************************************************************
 * unlink_sem_task -- remove a task from a wait list
 *
 * Removes a task from the list of tasks waiting on a semaphore.
 *************************************************************************/
static void unlink_sem_task(RT_TASK_ENTRY *to_unlink, rt_sem_t *sem)
{
  if (to_unlink->next != NULL)
    to_unlink->next->prev = to_unlink->prev;
  if (to_unlink->prev == NULL)
    sem->wait_list = to_unlink->next;
  else
    to_unlink->prev->next = to_unlink->next;
}

/*************************************************************************
 * unlink_mq_task -- remove a task from a wait list
 *
 * Removes a task from the list of tasks waiting on a message queue.
 *************************************************************************/
static void unlink_mq_task(RT_TASK_ENTRY *to_unlink, rt_mq_t *mq)
{
  if (to_unlink->next != NULL)
    to_unlink->next->prev = to_unlink->prev;
  if (to_unlink->prev == NULL)
    mq->wait_list = to_unlink->next;
  else
    to_unlink->prev->next = to_unlink->next;
}

/* cope with the changed priority system */
#define GET_PRIO(task) (sched_get_priority_max(0) - (*(task))->sched_param.sched_priority)

#define rtl_current (pthread_self()->user[IPC_DATA_INDEX])
/* #define rtl_current ((LOCAL_SCHED)-> rtl_current); */
/*************************************************************************
 * rt_sem_post -- semaphore post operation
 *
 * The semaphore post (sometimes known as 'give', 'signal', or 'V') operation.
 * If tasks are waiting for the semaphore, the one with the highest priority
 * is allowed to run.
 *
 * Returns 0 if successful, or -EINVAL if the semaphore is not valid.
 *************************************************************************/
int rt_sem_post(rt_sem_t *sem)
{
  int ret = 0;
  int flags;
  if (sem->magic != RT_SEM_MAGIC)
    ret = -EINVAL;		/* invalid rt_sem_t structure */
  else
  {
    RT_TASK_ENTRY *to_run = NULL;
    rtl_critical(flags);
    if (sem->val < 0)	/* one or more tasks are waiting for this sem */
    {
      /* find the waiting task with the highest priority */
      RT_TASK_ENTRY *t;
      /* search exhaustively all waiting tasks.  I don't want to keep */
      /* the list in priority order because I don't want to assume    */
      /* the task priorities won't change.                            */
      for (t=sem->wait_list ; t!=NULL ; t=t->next)
        if (to_run == NULL || GET_PRIO(t->task) < GET_PRIO(to_run->task))
          to_run = t;
      /* remove the task to be run from the wait_list */
      unlink_sem_task(to_run, sem);
      /* mark that task as no longer waiting at sem */
      ((RT_TASK_IPC *)(to_run->task))->sem_at = NULL;
    }
    /* binary semaphores never exceed 1 */
    if (sem->val < 1 || sem->type == RT_SEM_COUNTING)
      ++sem->val;
    if (to_run != NULL)
    {
      /* rt_sem_wait() returned because of a post, not */
      /* because of a timeout */
      ((RT_TASK_IPC *)(to_run->task))->timed_out = 0;
      rt_task_wakeup(to_run->task);
    }
    rtl_end_critical(flags);
  }
  return ret;
}

/*************************************************************************
 * rt_sem_wait -- semaphore wait operation (blocking)
 *
 * The semaphore wait (sometimes known as 'take' or 'P') operation.
 * If the semaphore is not available, the calling task blocks until
 * it is.  'timeout' is an optional timeout period.  If 'timeout' is
 * RT_WAIT_FOREVER, the function does not time out.  If 'timeout' is
 * RT_NO_WAIT and the semaphore is not available, rt_sem_wait() returns
 * immediately.  If 'timeout' is any other value, it represent a time 
 * at which the call to rt_sem_wait() should time out.  If that time
 * is reached, rt_sem_wait() returns with -ETIME.
 *
 * Returns 0 if successful, -ETIME if the operation timed out, -EAGAIN if
 * RT_NO_WAIT was specified and the semaphore was not available, or -EINVAL
 * if the semaphore is not valid.
 *************************************************************************/
int rt_sem_wait(rt_sem_t *sem, RTIME timeout)
{
  int ret = 0;
  int flags;
  if (sem->magic != RT_SEM_MAGIC)
    ret = -EINVAL;		/* invalid rt_sem_t structure */
  else
  {
	  rtl_critical(flags);
    if (sem->val <= 0)		/* sem not available -- task must wait */
    {
      if (timeout == RT_NO_WAIT)
        ret = -EAGAIN;
      else
      {
        RT_TASK_ENTRY *to_add = &(((RT_TASK_IPC *)rtl_current)->rte);
        /* put task on wait_list */
        to_add->task = rtl_current;
        to_add->prev = NULL;
        to_add->next = sem->wait_list;
        if (to_add->next != NULL)
          to_add->next->prev = to_add;
        sem->wait_list = to_add;
        /* indicate which sem the task is blocked at */
        ((RT_TASK_IPC *)rtl_current)->sem_at = sem;
        /* and decrement sem value */
        --sem->val;
        /* and finally, block */
        if (timeout == RT_WAIT_FOREVER)
          rt_task_suspend(rtl_current);	/* suspend until post */
        else
        {
          /* assume call timed out.  If this is not the case, */
          /* rt_sem_post() will clear this flag */
          ((RT_TASK_IPC *)rtl_current)->timed_out = 1;
          /* delay until either post occurs or timeout occurs */
          rt_task_delay(timeout);
          if (((RT_TASK_IPC *)rtl_current)->timed_out)
          {
            /* timeout occurred -- undo everything and return */
            unlink_sem_task(to_add, sem);
            ++sem->val;
            ret = -ETIME;
          }
        }
      }
    }
    else
      --sem->val;
   rtl_end_critical(flags);
  }
  return ret;
}

/*************************************************************************
 * rt_sem_trywait -- semaphore wait operation (unblocking)
 *
 * The semaphore wait (sometimes known as 'take' or 'P') operation.
 * The function returns immediately whether or not the semaphore is
 * available.
 *
 * Returns 0 if successful, -EAGAIN if the semaphore is not available,
 * or -EINVAL if the semaphore is not valid.
 *************************************************************************/
int rt_sem_trywait(rt_sem_t *sem)
{
  int ret = 0;
  int flags;
  if (sem->magic != RT_SEM_MAGIC)
    ret = -EINVAL;		/* invalid rt_sem_t structure */
  else
  {
	  rtl_critical(flags);
    if (sem->val <= 0)		/* sem not available -- task must wait */
      ret = -EAGAIN;
    else
      --sem->val;
    rtl_end_critical(flags);
  }
  return ret;
}

/*************************************************************************
 * rt_task_ipc_init -- rt_ipc version of rt_task_init()
 *
 * RT-Linux programs using rt_ipc should use rt_task_ipc_init instead of
 * rt_task_init().  It initializes some rt_ipc variables, then calls
 * rt_task_init().  Note that all parameters are the same as in rt_task_init()
 * except 'task', which is an RT_TASK_IPC instead of an RT_TASK.
 *
 * Returns 0 if successful, -EINVAL if the 'task' structure is already in
 * use by another task, or -ENOMEM if a memory allocation error occurred.
 *************************************************************************/
int rt_task_ipc_init(RT_TASK_IPC *task, void (*fn)(int data), int data,
  int stack_size, int priority)
{
  /* initially task is not blocked on a semaphore */
  int ret;
  task->sem_at = NULL;
  task->magic = RT_TASK_IPC_MAGIC;
  ret = rt_task_init(MAKE_RT_TASK(task), fn, data, stack_size, priority);
  if (ret < 0) {
	  return ret;
  }
  (*MAKE_RT_TASK(task))->user[IPC_DATA_INDEX] = task;
  return 0;
}

/*************************************************************************
 * rt_task_ipc_delete -- rt_ipc version of rt_task_delete()
 *
 * RT-Linux programs using rt_ipc should use rt_task_ipc_delete instead of
 * rt_task_delete().  It removes the task from any semaphore or message queue
 * it is in, then calls rt_task_delete().  Note that its parameter is an
 * RT_TASK_IPC instead of an RT_TASK.
 *
 * Returns 0 if successful, or -EINVAL if 'task' does not refer to a valid
 * task.
 *************************************************************************/
int rt_task_ipc_delete(RT_TASK_IPC *task)
{
  int ret = 0;
  if (task->magic != RT_TASK_IPC_MAGIC)
    ret = EINVAL;
  else
  {
    /* for task deletion safety, must remove task from any sem or mq list */
    int flags;

    rtl_critical(flags);
    if (task->sem_at != NULL)
      unlink_sem_task(&(task->rte), task->sem_at);
    else if (task->mq_at != NULL)
      unlink_mq_task(&(task->rte), task->mq_at);
    rtl_end_critical(flags);
    ret = rt_task_delete(MAKE_RT_TASK(task));
  }
  return ret;
}

/*************************************************************************
 * rt_task_delay -- delay task 
 *
 * Delays the calling task until the time specified in 'duration'.
 *
 * Always returns 0.
 *************************************************************************/
int rt_task_delay(RTIME duration)
{
  int ret = 0;
  int flags;
	  rtl_critical(flags);
  /* mark the task as delayed */
  pthread_self()->period = 0;
  RTL_MARK_SUSPENDED(pthread_self());
  __rtl_setup_timeout(pthread_self(), HRT_FROM_8254(duration));
  /* set the time at which execution may resume */
  rtl_schedule();
  rtl_end_critical(flags);
  return ret;
}

/*************************************************************************
 * rt_mq_init -- initialize a real-time message queue
 *
 * Called to initialize a real-time message queue.  'mq' must point to a
 * statically allocated structure.  'max_msgs' is the maximum number of
 * messages allowed, and 'msg_size' is the size of each message.
 *
 * Returns 0 if successful, -ENOMEM if space for the queue could not be
 * allocated, or -EINVAL if called incorrectly.
 *************************************************************************/
int rt_mq_init(rt_mq_t *mq, int max_msgs, int msg_size)
{
  int ret = 0;
  if (max_msgs <= 0 || msg_size < 0)
    ret = -EINVAL;		/* must be positive */
  else
  {
    mq->magic = RT_MQ_MAGIC;
    mq->wait_list = NULL;
    mq->max_msgs = max_msgs;
    mq->msg_size = msg_size;
    /* for efficiency, the max size of the queue data is allocated */
    /* all in one piece at init time */
    if ((mq->q = kmalloc(max_msgs * msg_size, GFP_KERNEL)) == NULL)
      ret = -ENOMEM;
    else
    {
      mq->status = RT_MQ_EMPTY;
      mq->f = mq->r = mq->q;		/* initialize queue pointers */
    }
  }
  return ret;
}

/*************************************************************************
 * rt_mq_destroy -- remove a real-time message queue
 *
 * Removes a message queue previously created with rt_mq_create().  Message
 * queue deletion safety is implemented; i.e., any tasks blocked on this 
 * message queue when it is destroyed are allowed to run.
 *
 * Returns 0 if successful, -EINVAL if 'sem' is not a valid rt_sem_t.
 *************************************************************************/
int rt_mq_destroy(rt_mq_t *mq)
{
  int ret = 0;
  if (mq->magic != RT_MQ_MAGIC)
    ret = -EINVAL;
  else
  {
    /* unblock any tasks blocked on this message queue */
    while (rt_mq_send(mq, NULL, RT_MQ_NORMAL, RT_NO_WAIT) != 0)
      ;
    while (rt_mq_receive(mq, NULL, RT_NO_WAIT) != 0)
      ;
    kfree_s(mq->q, mq->max_msgs * mq->msg_size);
  }
  return ret;
}

/*************************************************************************
 * enqueue -- enqueue data
 *
 * Enqueues a block of data on the queue 'mq' with priority 'prio'.
 * An RT_MQ_NORMAL block goes to the rear of the queue, while an RT_MQ_URGENT
 * block goes to the front.  The status is set appropriately as RT_MQ_FULL
 * or RT_MQ_NEITHER.  enqueue() should not be called when RT_MQ_FULL.
 *************************************************************************/
static void enqueue(rt_mq_t *mq, char *msg, RT_MQ_PRIO prio)
{
  if (prio == RT_MQ_NORMAL)
  {
    if (msg != NULL)
      memcpy(mq->r, msg, mq->msg_size);
    /* check for wraparound */
    if ((mq->r += mq->msg_size) == mq->q + mq->msg_size * mq->max_msgs)
      mq->r = mq->q;
  }
  else	/* prio == RT_MQ_URGENT */
  {
    /* check for wraparound */
    if (mq->f == mq->q)
      mq->f = mq->q + mq->msg_size * (mq->max_msgs- 1) ;
    else
      mq->f -= mq->msg_size;
    if (msg != NULL)
      memcpy(mq->f, msg, mq->msg_size);
  }
  if (mq->f == mq->r)		/* queue is now full */
    mq->status = RT_MQ_FULL;
  else
    mq->status = RT_MQ_NEITHER;
}

/*************************************************************************
 * dequeue -- dequeue data
 *
 * Dequeues a block of data from the queue 'mq'.  The status is set
 * appropriately as RT_MQ_EMPTY or RT_MQ_NEITHER.  dequeue() should not
 * be called when RT_MQ_EMPTY.
 *************************************************************************/
static void dequeue(rt_mq_t *mq, char *msg)
{
  if (msg != NULL)
    memcpy(msg, mq->f, mq->msg_size);
  /* check for wraparound */
  if ((mq->f += mq->msg_size) == mq->q + mq->msg_size * mq->max_msgs)
    mq->f = mq->q;
  if (mq->r == mq->f)		/* queue is now empty */
    mq->status = RT_MQ_EMPTY;
  else
    mq->status = RT_MQ_NEITHER;
}

/*************************************************************************
 * rt_mq_send -- message queue send operation
 *
 * Enqueues the data 'msg' on the message queue 'mq'.  The data is assumed
 * to be of the size with which rt_mq_init() was called.  If 'prio' is
 * RT_MQ_NORMAL, the data is queued at the end. If 'prio' is RT_MQ_URGENT,
 * the data is forced to the front of the queue.  'wait' specifies an
 * optional timeout period.  If 'wait' is RT_NO_WAIT, rt_mq_send()
 * returns immediately even if no space for the message is present.  If
 * 'wait' is RT_WAIT_FOREVER, no timeout occurs.  If 'wait' is any other
 * value, it reflects the time at which rt_mq_send() will wake up and
 * return with -ETIME.
 *
 * Returns 0 if successful, -ETIME if the operation timed out, -EAGAIN if
 * RT_NO_WAIT was specified and the operation could not be completed
 * immediately, or -EINVAL if the rt_mq_t is not valid.
 *************************************************************************/
int rt_mq_send(rt_mq_t *mq, char *msg, RT_MQ_PRIO prio, RTIME wait)
{
  int ret = 0;
  if (mq->magic != RT_MQ_MAGIC)
    ret = -EINVAL;		/* invalid rt_mq_t structure */
  else
  {
    int flags;

    rtl_critical(flags);
    switch (mq->status)
    {
    case RT_MQ_FULL:	/* q full -- this task must wait */
    {
      if (wait == RT_NO_WAIT)
        ret = -EAGAIN;	/* can't queue it -- just report error */
      else	/* wait is allowed */
      {
        RT_TASK_ENTRY *to_add = &(((RT_TASK_IPC *)rtl_current)->rte);
        /* put task on wait_list */
        to_add->task = rtl_current;
        to_add->prev = NULL;
        to_add->next = mq->wait_list;
        if (to_add->next != NULL)
          to_add->next->prev = to_add;
        mq->wait_list = to_add;
        /* indicate which mq the task is blocked at */
        ((RT_TASK_IPC *)rtl_current)->mq_at = mq;
        if (wait == RT_WAIT_FOREVER)
          rt_task_suspend(rtl_current);	/* suspend until receive */
        else
        {
          /* assume call timed out.  If this is not the case, */
          /* rt_mq_receive() will clear this flag */
          ((RT_TASK_IPC *)rtl_current)->timed_out = 1;
          /* delay until either receive occurs or timeout occurs */
          rt_task_delay(wait);
          if (((RT_TASK_IPC *)rtl_current)->timed_out)
          {
            /* timed out -- undo everything and return */
            unlink_mq_task(to_add, mq);
            ret = -ETIME;
            break;
          }
        }
        /* when again allowed to run, enqueue the data */
        enqueue(mq, msg, prio);		/* finally, enqueue the data */
      }
      break;
    }
    case RT_MQ_EMPTY:	/* q empty -- this operation might unblock a task */
    {
      RT_TASK_ENTRY *t, *to_run;
      enqueue(mq, msg, prio);	/* first, go ahead and enqueue the data */
      /* find the waiting task with the highest priority */
      /* search exhaustively all waiting tasks.  I don't want to keep */
      /* the list in priority order because I don't want to assume    */
      /* the task priorities won't change.                            */
      for (t=mq->wait_list, to_run=NULL ; t!=NULL ; t=t->next)
        if (to_run == NULL || GET_PRIO(t->task) < GET_PRIO(to_run->task))
          to_run = t;
      if (to_run != NULL)
      {
        /* remove the task to be run from the wait_list */
        unlink_mq_task(to_run, mq);
        /* mark that task as no longer waiting at mq */
        ((RT_TASK_IPC *)(to_run->task))->mq_at = NULL;
        /* rt_mq_receive() will return because of a send, not */
        /* because of a timeout */
        ((RT_TASK_IPC *)(to_run->task))->timed_out = 0;
        rt_task_wakeup(to_run->task);
      }
      break;
    }
    case RT_MQ_NEITHER:		/* space exists for new entry -- put it in */
      enqueue(mq, msg, prio);
      break;
    }
    rtl_end_critical(flags);
  }
  return ret;
}

/*************************************************************************
 * rt_mq_receive -- message queue receive operation
 *
 * Dequeues the data 'msg' from the message queue 'mq'.  The data will 
 * be of the size with which rt_mq_init() was called.  'wait' specifies an
 * optional timeout period.  If 'wait' is RT_NO_WAIT, rt_mq_receive()
 * returns immediately even if no messages are present.  If
 * 'wait' is RT_WAIT_FOREVER, no timeout occurs.  If 'wait' is any other
 * value, it reflects the time at which rt_mq_receive() will wake up
 * and return with -ETIME.
 *
 * Returns 0 if successful, -ETIME if the operation timed out, -EAGAIN if
 * RT_NO_WAIT was specified and the operation could not be completed
 * immediately, or -EINVAL if the rt_mq_t is not valid.
 *************************************************************************/
int rt_mq_receive(rt_mq_t *mq, char *msg, RTIME wait)
{
  int ret = 0;
  if (mq->magic != RT_MQ_MAGIC)
    ret = -EINVAL;		/* invalid rt_mq_t structure */
  else
  {
    int flags;

    rtl_critical(flags);
    switch (mq->status)
    {
    case RT_MQ_EMPTY:	/* q empty -- this task must wait */
    {
      if (wait == RT_NO_WAIT)
        ret = -EAGAIN;	/* can't dequeue it -- just report error */
      else	/* wait is allowed */
      {
        RT_TASK_ENTRY *to_add = &(((RT_TASK_IPC *)rtl_current)->rte);
        /* put task on wait_list */
        to_add->task = rtl_current;
        to_add->prev = NULL;
        to_add->next = mq->wait_list;
        if (to_add->next != NULL)
          to_add->next->prev = to_add;
        mq->wait_list = to_add;
        /* indicate which mq the task is blocked at */
        ((RT_TASK_IPC *)rtl_current)->mq_at = mq;
        if (wait == RT_WAIT_FOREVER)
          rt_task_suspend(rtl_current);	/* suspend until receive */
        else
        {
          /* assume call timed out.  If this is not the case, */
          /* rt_mq_send() will clear this flag */
          ((RT_TASK_IPC *)rtl_current)->timed_out = 1;
          /* delay until either send occurs or timeout occurs */
          rt_task_delay(wait);
          if (((RT_TASK_IPC *)rtl_current)->timed_out)
          {
            /* timed out -- undo everything and return */
            unlink_mq_task(to_add, mq);
            ret = -ETIME;
            break;
          }
        }
        /* when again allowed to run, enqueue the data */
        dequeue(mq, msg);		/* finally, dequeue the data */
      }
      break;
    }
    case RT_MQ_FULL:	/* q full -- this operation might unblock a task */
    {
      RT_TASK_ENTRY *t, *to_run;
      dequeue(mq, msg);		/* first, go ahead and dequeue the data */
      /* find the waiting task with the highest priority */
      /* search exhaustively all waiting tasks.  I don't want to keep */
      /* the list in priority order because I don't want to assume    */
      /* the task priorities won't change.                            */
      for (t=mq->wait_list, to_run=NULL ; t!=NULL ; t=t->next)
        if (to_run == NULL || GET_PRIO(t->task) < GET_PRIO(to_run->task))
          to_run = t;
      if (to_run != NULL)
      {
        /* remove the task to be run from the wait_list */
        unlink_mq_task(to_run, mq);
        /* mark that task as no longer waiting at mq */
        ((RT_TASK_IPC *)(to_run->task))->mq_at = NULL;
        /* rt_mq_send() will return because of a receive, not */
        /* because of a timeout */
        ((RT_TASK_IPC *)(to_run->task))->timed_out = 0;
        rt_task_wakeup(to_run->task);
      }
      break;
    }
    case RT_MQ_NEITHER:		/* data is present -- take it out */
      dequeue(mq, msg);
      break;
    }
    rtl_end_critical(flags);
  }
  return ret;
}

typedef struct
{
  rt_sem_t sem;
} RT_IPC_FIFO;		/* structure holding rt_ipc rt-fifo-specific data */

static RT_IPC_FIFO ipc_fifos[IPC_RTF_NO];	/* rt_ipc-specific fifo data */

/*************************************************************************
 * rtf_ipc_handler -- handler for read/write operations on rt_ipc rt-fifo
 *
 * Called when a read or write operation is performed on an rt_ipc rt-fifo
 * by a Linux process.
 *
 * Returns 0.
 *************************************************************************/
static int rtf_ipc_handler(unsigned int fifo)
{
  return rt_sem_post(&ipc_fifos[fifo].sem);
}

/*************************************************************************
 * rtf_ipc_create -- create an rt_ipc rt-fifo
 *
 * Creates the rt_ipc rt-fifo 'fifo'.  This has the same capabilities as
 * the standard RT-Linux rt-fifos but adds blocking.  'size' is the size
 * of the fifo in bytes.  If 'rtl_to_linux' is 1, the fifo is used for
 * transferring data from RT-Linux tasks to a Linux process.  If 'rtl_to_linux'
 * is 0, the fifo is used for transferring data from a Linux process to
 * one or more RT-Linux tasks.
 *
 * Returns 0 if successful, -ENODEV if 'fifo' is not less than IPC_RTF_NO
 * and RTF_NO, -EBUSY if 'fifo' is in use, or -ENOMEM if 'size' bytes could
 * not be allocated.
 *************************************************************************/
int rtf_ipc_create(unsigned int fifo, int size, int rtl_to_linux)
{
  int ret = 0;
  if (fifo >= IPC_RTF_NO)
    ret = -ENODEV;
  else if ((ret = rtf_create(fifo, size)) >= 0)
  {
    /* init the semaphores -- initially no data is ready to receive, but */
    /* data can be sent */
    if ((ret = rt_sem_init(&ipc_fifos[fifo].sem, RT_SEM_BINARY, rtl_to_linux)) >= 0)
      ret = rtf_create_handler(fifo, &rtf_ipc_handler);
  }
  return ret;
}

/*************************************************************************
 * rtf_ipc_destroy -- destroy an rt_ipc rt-fifo
 *
 * Removes the rt_ipc rt-fifo 'fifo' previously created with rt_ipc_create().
 *
 * Returns 0 if successful, -EINVAL if 'fifo' is not a valid fifo identifier.
 *************************************************************************/
int rtf_ipc_destroy(unsigned int fifo)
{
  int ret;
  if ((ret = rt_sem_destroy(&ipc_fifos[fifo].sem)) >= 0)
    ret = rtf_destroy(fifo);
  return ret;
}

/*************************************************************************
 * rtf_receive -- get data from an rt_ipc rt-fifo
 *
 * Gets data from the rt-fifo 'fifo'.  The data, up to size 'count', is put
 * into 'buf'.  If 'timeout' is RT_NO_WAIT, the function returns immediately
 * even if 'count' bytes are not available.  If 'timeout' is RT_WAIT_FOREVER,
 * the function blocks until 'count' bytes are available.  If 'timeout' is
 * any other value, it represents the time at which the function will return
 * with a timeout after unsuccessfully waiting for data.  In any case, as
 * many bytes as possible are returned, even if a timeout occurs or
 * RT_NO_WAIT is specified.
 *
 * Returns -ENODEV if 'fifo' is greater than or equal to RTF_NO, -EINVAL if
 * 'fifo' is not a valid fifo identifier.  If the return value is greater
 * than or equal to zero, it represents the number of bytes received.
 * This might be less than 'count' if the function timed out or RT_NO_WAIT
 * was specified and less than 'count' bytes were available.
 *************************************************************************/
int rtf_receive(unsigned int fifo, void *buf, int count, RTIME timeout)
{
  int ret = 0, bytes_still_to_get=count;
  for ( ; bytes_still_to_get > 0 ; )
  {
    if ((ret = rtf_get(fifo, buf, bytes_still_to_get)) < 0)
      break;
    bytes_still_to_get -= ret;
    buf += ret;
    if (bytes_still_to_get != 0)
      if ((ret = rt_sem_wait(&ipc_fifos[fifo].sem, timeout)) < 0)
        break;
  }
  if (ret == -ETIME || ret == -EAGAIN)
    return count - bytes_still_to_get;
  else if (ret < 0)
    return ret;
  else
  {
    /* Note the questionable assumption -- I signal that data is available    */
    /* for receiving any time rtf_receive() returns successfully.  Thus the   */
    /* assumption is that if 'count' bytes are available, there must be more. */
    /* Obviously wrong if the fifo has exactly 'count' bytes available when   */
    /* called.  I have to make this assumption because I have no way of       */
    /* knowing how many bytes are available in the fifo.  The only effect of  */
    /* this is that a task waiting on the semaphore may go one more time      */
    /* through the loop before again waiting at the semaphore.                */
    if ((ret = rt_sem_post(&ipc_fifos[fifo].sem)) < 0)
      return ret;
    else
      return count;
  }
}

/*************************************************************************
 * rtf_send -- send data to an rt_ipc rt-fifo
 *
 * Sends data to the rt-fifo 'fifo'.  The data, of size 'count', is taken
 * from 'buf'.  If 'timeout' is RT_NO_WAIT, the function returns immediately
 * even if 'count' bytes cannot be sent.  If 'timeout' is RT_WAIT_FOREVER,
 * the function blocks until 'count' bytes can be sent.  If 'timeout' is
 * any other value, it represents the time at which the function will return
 * with a timeout after unsuccessfully waiting to send the data.  If 
 * rtf_send() cannot send the entire block, no data is sent.
 *
 * Returns -ENODEV if 'fifo' is greater than or equal to RTF_NO, -EINVAL if
 * 'fifo' is not a valid fifo identifier.  If the return value is greater
 * than or equal to zero, it represents the number of bytes sent.
 * This might be zero if the function timed out or RT_NO_WAIT
 * was specified and less than 'count' bytes could be sent.
 *************************************************************************/
int rtf_send(unsigned int fifo, void *buf, int count, RTIME timeout)
{
  int ret = 0, bytes_still_to_send=count;
  for ( ; bytes_still_to_send > 0 ; )
  {
    /* Note the asymmetry between rtf_send() and rtf_receive() due to the   */
    /* fact that rtf_put() returns -ENOSPC if it cannot place all bytes on  */
    /* the fifo, whereas rtf_get() returns as many bytes as it can, even if */
    /* it cannot return all bytes requested.  Also, rtf_put() returns the   */
    /* number of bytes not written, whereas rtf_get() returns the number of */
    /* bytes successfully read. */
    ret = rtf_put(fifo, buf, bytes_still_to_send);
    if (ret == -ENOSPC)
      ret = 0;
    else if (ret < 0)
      break;
    /* correct for fact that ret is number of bytes NOT yet sent */
    ret = (bytes_still_to_send - ret);
    bytes_still_to_send -= ret;
    buf += ret;
    if (bytes_still_to_send != 0)
      if ((ret = rt_sem_wait(&ipc_fifos[fifo].sem, timeout)) < 0)
        break;
  }
  if (ret == -ETIME || ret == -EAGAIN)
    return count - bytes_still_to_send;
  else if (ret < 0)
    return ret;
  else
  {
    /* Note the questionable assumption -- I signal that space is available   */
    /* for sending any time rtf_send() returns successfully.  Thus the        */
    /* assumption is that if 'count' bytes could be sent, there must be room  */
    /* for more.  Obviously wrong if the fifo has exactly 'count' bytes empty */
    /* when called.  I have to make this assumption because I have no way of  */
    /* knowing how many bytes are available in the fifo.  The only effect of  */
    /* this is that a task waiting on the semaphore may go one more time      */
    /* through the loop before again waiting at the semaphore.                */
    if ((ret = rt_sem_post(&ipc_fifos[fifo].sem)) < 0)
      return ret;
    else
      return count;
  }
}

int init_module(void)
{
  printk("rt_ipc V" IPC_VERSION " -- IPC primitives for use with Real-Time Linux\n");
  printk("Copyright (C) 1997 Jerry Epplin.  All rights reserved.\n");
  return 0;
}

void cleanup_module(void)
{
  printk("rt_ipc -- removed.\n");
}
