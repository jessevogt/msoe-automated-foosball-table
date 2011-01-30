#!/bin/bash

OUTFILE=regression.log

if [ ! -z "$1" ] ; then
    OUTFILE=$1
else
    rm -f ${OUTFILE}
fi

SETCOLOR_SUCCESS="echo -en \\033[1;32m"
SETCOLOR_FAILURE="echo -en \\033[1;31m"
SETCOLOR_WARNING="echo -en \\033[1;33m"
SETCOLOR_NORMAL="echo -en \\033[0;39m"

HOSTARCH=`uname -m | sed -e s/i.86/i386/ -e s/sun4u/sparc64/ -e s/arm.*/arm/ -e s/sa110/arm/`
if [ -d linux ]; then cd linux;RTLINUX=`pwd`;cd ..;\
	  else if [ -d /sys/linux ]; then RTLINUX=/sys/linux; \
	  else if [ -d /usr/src/linux ]; then RTLINUX=/usr/src/linux; \
	  else if [ -d /util/2.3/rtlinux ]; then RTLINUX=/util/2.3/rtlinux; \
	  else echo "No directory for linux 2.3"; fi ; fi; fi; fi
KPL=`grep UTS_RELEASE ${RTLINUX}/include/linux/version.h 2>> ${OUTFILE} | \
	cut -d\" -f2 | awk -F \. '{print $2}'`

PATH=$PATH:/sbin

MOD_LISTF="\
	rtl \
	rtl_time \
	rtl_sched \
	"

#
# check for configuration dependant modules
#
if [ -f modules/rtl_posixio.o ] ; then
	MOD_LISTF="${MOD_LISTF} rtl_posixio"
	if [ -f modules/rtl_fifo.o ] ; then
		MOD_LISTF="${MOD_LISTF} rtl_fifo"
	fi
elif [ -f modules/rtl_fifo.o ] ; then
	MOD_LISTF="${MOD_LISTF} rtl_fifo"
fi
if [ -f modules/psc.o ] ; then
	MOD_LISTF="${MOD_LISTF} psc"
fi
if [ -f modules/rtl_mqueue.o ] ; then
	MOD_LISTF="${MOD_LISTF} rtl_mqueue"
fi

# what are we woried about ;)
sync;sync;sync

cleanup()
{
    	for x in periodic_test rtl_mqueue rtl_fifo rtl_posixio rtl_sched rtl_time psc rtl 
    	do
		(rmmod $x 2>> ${OUTFILE})
    	done
}

fatal()
{
    echo -n '[ '
    $SETCOLOR_FAILURE
    echo -n FAILED
    $SETCOLOR_NORMAL
    echo ' ]'
    echo 'Fatal Error.  Exiting.'
    cleanup
    exit 1
}

failure()
{
    echo -n '[ '
    $SETCOLOR_FAILURE
    echo -n FAILED
    $SETCOLOR_NORMAL
    echo ' ]'
}

success()
{
    echo -n '[ '
    $SETCOLOR_SUCCESS
    echo -n "  OK  "
    $SETCOLOR_NORMAL
    echo ' ]'
}

echo '----------------------------'
echo '- Installing basic modules -'
echo '----------------------------'
cleanup

for x in $MOD_LISTF
do
    echo -n "Testing multiple loads of $x.o...	 	"
    l=`echo $x|wc -c`
    if [ `expr $l \< 10` = "1" ] ; then
    	echo -n "	"
    fi
    
    CNT=70
    #
    # special case for timer on x86 - it's slow -- Cort
    #
    if [ "${HOSTARCH}" = "i386" -a "${x}" = "rtl_time" ] ; then
	CNT=10
    fi
    while [ "${CNT}" != 0 ]; do
	CNT=`expr $CNT - 1`
	if [ "$x" = "rtl" ] ; then
		insmod modules/$x.o quiet=1 2>> ${OUTFILE}
	else
		insmod modules/$x.o 2>> ${OUTFILE}
	fi
	if [ $? != "0" ] ; then
	    fatal
	    exit -1
	fi
	# on the last run, leave the module so others can load
	if [ "$CNT" != 0 ]; then
		rmmod $x 2>> ${OUTFILE}
	fi
    done
    success
done

sleep 1

echo -n 'Testing RTLinux fifos...				'
(regression/fifo_app 2>> ${OUTFILE})
if [ $? != "0" ] ; then
	failure
else
	success
fi
(rmmod fifo_module 2>> ${OUTFILE})

echo -n 'Testing thread wait times...				'
(regression/thread_app 2>> ${OUTFILE})
if [ $? != "0" ] ; then
	failure
else
	success
fi

echo -n 'Testing that Linux time progresses...			'
s=`date`
CNT=1000
while [ ${CNT} != 0 ]; do
	CNT=`expr $CNT - 1`
	/bin/true
done
if [ "${s}" = "`date`" ]; then
	failure
else
	success
fi

echo -n 'Testing that Linux time is monotonically increasing...	'
(regression/linuxtime 2>> ${OUTFILE})
if [ $? != "0" ] ; then
	failure
else
	success
fi

echo -n 'Testing ping flood...					'
sync;sync;sync
(ping -f -c 100000 localhost 2>> ${OUTFILE} > /dev/null )
success # if it doesn't crash - it passes

if [ -f modules/psc.o ] ; then

	echo -n 'Testing User-Level IRQ signals...			'
	IRQ=`fgrep -v ' 0' /proc/interrupts | awk '{print $2 " " $1}' | sort -n | tail -1 | sed 's/:$//' | awk '{print $2}'`
	(regression/rtlsigirq_app $IRQ 2>> ${OUTFILE})
	if [ $? != "0" ] ; then
		failure
	else
	  	success
	fi

	echo -n 'Testing User-Level Timer signals...			'
	(regression/rtlsigtimer_app 2>> ${OUTFILE})
	if [ $? != "0" ] ; then
		failure
	else
 		success
	fi
	
	echo -n 'Testing User-Level gethrtime()...			'
	(regression/rtlgethrtime_test 2>> ${OUTFILE})
	if [ $? != "0" ] ; then
		failure
	else
		success
	fi

	echo -n 'Testing User-Level FIFO...				'
	(regression/psc_fifo_test 2>> ${OUTFILE})
	if [ $? != "0" ] ; then
		failure
	else
		success
	fi

	echo -n 'Testing multiple User-Level Threads...			'
	(regression/psc_multi 2>> ${OUTFILE})
	if [ $? != "0" ] ; then
		failure
	else
		success
	fi

	# if this doesn't hard lock the system, it passes. -Nathan

	# i have a dream -- and that dream is that someday psc debugging will
	# work on SMP, and then we can uncomment all of this.  i also have

	# Nathan, this dream of yours has come true -- MB.
	# But, need to fix some mm issues

	# another dream -- and that dream is that someday psc will take care
	# of cleaning up any messes anyone might make, and we won't need to
	# unload and load all of the modules for every single one of these
	# tests. -Nathan

	# Yep, that would be great -- Michael

#	echo -n 'Testing User-Level debugging...			'
#	(insmod debugger/rtl_debug.o >& /dev/null)
#	(insmod modules/psc.o >& /dev/null)
#	(regression/psc_dbg_div0 >& /dev/null)
#	(rmmod psc >& /dev/null)
#	(rmmod rtl_debug >& /dev/null)
#	(scripts/rmrtl >& /dev/null)
#	(scripts/insrtl >& /dev/null)

#	(insmod debugger/rtl_debug.o >& /dev/null)
#	(insmod modules/psc.o >& /dev/null)
#	(regression/psc_dbg_oob >& /dev/null)
#	(rmmod psc >& /dev/null)
#	(rmmod rtl_debug >& /dev/null)
#	(scripts/rmrtl >& /dev/null)
#	(scripts/insrtl >& /dev/null)

#	(rmmod psc >& /dev/null)
#	(rmmod rtl_debug >& /dev/null)
#	(insmod debugger/rtl_debug.o >& /dev/null)
#	(insmod modules/psc.o >& /dev/null)
#	(regression/psc_dbg_libcall >& /dev/null)
#	success

	echo -n 'Removing psc.o...					'
	(rmmod psc 2>> ${OUTFILE})
	if [ $? != "0" ] ; then
		fatal
	else
		success
        fi
fi

(insmod regression/fp_test.o >& /dev/null)
if [ $? = "0" ] ; then
	echo -n 'Testing floating-point support...			'
	(regression/fp_app 2>> ${OUTFILE})
	if [ $? != "0" ] ; then
		failure
	else
		success
	fi
fi

if lsmod | grep -q rtl_mqueue; then
	echo -n 'Removing rtl_mqueue.o...				'
	(rmmod rtl_mqueue 2>> ${OUTFILE})
	if [ $? != "0" ] ; then 
		fatal
	else
		success
	fi
fi

echo -n 'Removing rtl_sched.o...					'
(rmmod rtl_sched 2>> ${OUTFILE})
if [ $? != "0" ] ; then
	fatal
else
	success
fi

echo -n 'Testing periodic timer...				'
(insmod regression/periodic_test.o 2>> ${OUTFILE})
if [ $? != "0" ] ; then
	failure
else
	(regression/periodic_monitor 2>> ${OUTFILE})
	if [ $? != "0" ] ; then
  		failure
	else
   		success
	fi
fi
(rmmod periodic_test 2>> ${OUTFILE})

echo -n 'Testing oneshot timer...				'
(insmod regression/oneshot_test.o 2>> ${OUTFILE})
if [ $? != "0" ] ; then
	failure
else
	(regression/oneshot_monitor 2>> ${OUTFILE})
	if [ $? != "0" ] ; then
  		failure
	else
   		success
	fi
fi
(rmmod oneshot_test 2>> ${OUTFILE})

cleanup

