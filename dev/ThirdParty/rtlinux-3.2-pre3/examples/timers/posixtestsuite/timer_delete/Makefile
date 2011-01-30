SOURCE:=$(shell (ls *.c))
MODULES=$(shell for i in `ls *.c | awk -F. '{print $$1}' `; do echo $$i".o" ; done )
MODNAMES=$(shell for i in `ls *.c | awk -F. '{print $$1}' `; do echo $$i; done )
include ../../../../rtl.mk

clean:
	rm -f *.o
all: 
	$(MAKE) $(MODULES)
help:
	@echo "/***************************************************************/"
	@echo "/*  Open POSIX test suite addapted to RTLinux.                 */"
	@echo "/***************************************************************/"	
test: all help
	@echo "First we remove any existing rtl-modules"
	@echo "You may see error warnings from \"make\" - ignore them"
	@echo "Type <return> to continue"
	@read junk
	-rmmod sound
	-rmmod rt_process
	-rmmod frank_module
	-(for i in $(MODNAMES) ; do rmmod $$i ; done)	 	
	(cd ../../../../; scripts/rmrtl)
	@echo "Now insert the fifo and scheduler"
	@echo "Type <return> to continue"
	@read junk
	(cd ../../../../; scripts/insrtl)
	@echo "Now start the real-time test tasks modules"
	@echo "Type <return> to continue"
	@read junk
	@dmesg -c
	(for i in $(MODNAMES) ; do \
	echo ">------------------------------------------<" ; \
	echo "          TESTING "$$i".o MODULE            " ; \
	echo ">------------------------------------------<" ; \
	cat $$i.c | awk '{ if ( $$1 == "*" ) print $0 }' ; \
	insmod $$i.o; \
	echo "Sleeping for 2 seconds" ; \
	sleep 2 ; \
	echo ">------------------------------------------<" ; \
	echo "               TEST RESULTS:                " ; \
	echo ">------------------------------------------<" ; \
	dmesg -c ;\
	rmmod $$i ; \
	dmesg -c ; \
	echo "Type <return> to continue" ; \
	read junk ; \
	 done)
include ../../../../Rules.make
