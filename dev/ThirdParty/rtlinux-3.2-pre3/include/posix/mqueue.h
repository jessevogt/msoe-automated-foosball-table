/*
 * @File: mqueue.h
 *
 * @Contents: POSIX Message Passing related definitions and macros
 *
 * Copyright (C) 2002 Sergio Saez <ssaez@disca.upv.es>
 * Project OCERA (Open Components for Realtime Embedded Applications)
 *
 */

#ifndef MQUEUE_H
#define MQUEUE_H

/*
 * Required include files
 */

#include <rtl_conf.h>

#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>

#ifdef _RTL_POSIX_TIMEOUTS
#       include <time.h>
#endif

/*
 * Implementation specific maximum values 
 */

/* The maximum number of open message queue descriptors a process may hold. */
#ifdef CONFIG_MQ_OPEN_MAX
#define _RTL_MQ_OPEN_MAX     CONFIG_MQ_OPEN_MAX 
#else
#define _RTL_MQ_OPEN_MAX     64      /* _POSIX_MQ_OPEN_MAX   8 */
#endif
        /* RTLinux can be seen as only one process with several threads. Therefore, this
         * maximum value is also a system-wide limit. */

/* The maximum number of message priorities supported by the implementation. */
#ifdef CONFIG_MQ_PRIO_MAX
#define _RTL_MQ_PRIO_MAX     CONFIG_MQ_PRIO_MAX
#else
#define _RTL_MQ_PRIO_MAX     32      /* _POSIX_MQ_PRIO_MAX   32 */
#endif
        /* 32 priorities in order to fit the bitmap in a variable of type 'long'.  */

/* The maximum number of message queues the system may hold simultaneously. */
#ifdef CONFIG_MQ_CREATE_MAX
#define _RTL_MQ_CREATE_MAX   CONFIG_MQ_CREATE_MAX
#else
#define _RTL_MQ_CREATE_MAX   32 
#endif
        /* This maximum value is a system-wide limit. */

/*
 * Types definition
 */

/* Message queue attributes parameter */
struct mq_attr {                
    /* POSIX required fields */
    long        mq_flags;       /* Message queue flags. */
    long        mq_maxmsg;      /* Maximum number of messages. */
    long        mq_msgsize;     /* Maximum message size. */
    long        mq_curmsgs;     /* Number of messages currently queued. */
    
};     

/* mqd_t is an index in an array */
typedef long mqd_t;             /* POSIX required */

/*
 * Funtion prototypes
 */ 
int     init_mqueue (void);

int     mq_close (mqd_t);
int     mq_getattr (mqd_t, struct mq_attr *);
int     mq_notify (mqd_t, const struct sigevent *);
mqd_t   mq_open (const char *, int, ...);
/* int     mq_send (mqd_t, const char *, size_t, unsigned ); */
#define mq_send(mqdes, msg_ptr, msg_len, msg_prio) mq_timedsend(mqdes, msg_ptr, msg_len, msg_prio, NULL)
/* ssize_t mq_receive (mqd_t, char *, size_t, unsigned *); */
#define mq_receive(mqdes, msg_ptr, msg_len, msg_prio) mq_timedreceive(mqdes, msg_ptr, msg_len, msg_prio, NULL)


int     mq_setattr (mqd_t, const struct mq_attr *,
                    struct mq_attr *);
        
#ifdef _RTL_POSIX_TIMEOUTS
/* int     mq_timedsend (mqd_t, const char *, size_t, unsigned, const struct timespec * ); */
#define mq_timedsend __mq_timedsend
int     __mq_timedsend (mqd_t, const char *, size_t, unsigned, const struct timespec *);

/* ssize_t mq_timedreceive (mqd_t, char *, size_t, unsigned *, const struct timespec *); */
#define mq_timedreceive __mq_timedreceive
ssize_t __mq_timedreceive (mqd_t, char *, size_t, unsigned *, const struct timespec *);

#endif

int     mq_unlink (const char *);


#endif /* end mqueue.h */
