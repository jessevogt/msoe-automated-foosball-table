default:	all

VERSION		= 3.2
PRERELEASE	= pre3
RELEASE		= 
RELVER		= $(VERSION)
ifneq ($(RELEASE),)
RELVER		= $(VERSION)-$(RELEASE)
endif

MOD_DIR 	= ../modules
RTLINUX = $(shell if [ -d linux ]; then cd linux >/dev/null; pwd; \
	  else if [ -d /sys/linux ]; then echo /sys/linux; \
	  else if [ -d /usr/src/linux ]; then echo /usr/src/linux; \
	  else if [ -d /util/3.2/rtlinux ]; then echo /util/3.1/rtlinux; \
	  else echo "No directory for rtlinux 3.2"; fi ; fi; fi; fi)

RT_COM = $(shell if [ ! -d drivers/rt_com ]; then cd drivers ;\
	  if [ -d rt_com.0.5.5 ]; then ln -s rt_com.0.5.5 rt_com; \
		echo "rt_com.0.5.5" ; \
	  else if [ -d rt_com.0.3 ]; then ln -s rt_com.0.3 rt_com; \
		echo "rt_com.0.3" ; \
	  else echo "No directory for rtl_com"; fi; fi; fi)

HOSTARCH := $(shell uname -m | sed -e s/i.86/i386/ -e s/sun4u/sparc64/ -e s/arm.*/arm/ -e s/sa110/arm/)
ARCH		= $(shell ls -ld ${RTLINUX}/include/asm| sed 's/^.*-> asm-//g')

ifeq ($(RTLINUX)/.config,$(wildcard $(RTLINUX)/.config))
LINUX_CONFIG:=$(RTLINUX)/.config
include $(LINUX_CONFIG)
endif

RTL 		= $(shell pwd)
RTL_DIR		= $(shell pwd)
HPATH 		= $(RTL)/include
EXAMPLE_DIRS    := frank measurements hello hello_dev_mem mutex \
		misc parallel kernel_resources large_mem pwm

ifeq ($(ARCH),i386)
EXAMPLE_DIRS += sound
endif

ifdef CONFIG_RTL_NFIFOS
EXAMPLE_DIRS    += fifos
endif

ifdef CONFIG_PROC_FS
EXAMPLE_DIRS    += proc
endif

ifdef CONFIG_OC_PBARRIERS
EXAMPLE_DIRS    += pbarriers
endif


include schedulers/$(ARCH)/Makefile

ifeq ($(ARCH),i386)
# for the x86, take it directly from kernel (too many cases) -- Michael
LINUXCFLAGS := $(shell cd $(RTLINUX); make -s -f $(RTL)/scripts/cflags.mk)
LINUXCC := $(shell cd $(RTLINUX); make -s -f $(RTL)/scripts/cc.mk)
CFLAGS = $(LINUXCFLAGS)
CC = $(LINUXCC)
else

ifdef CONFIG_SMP
CFLAGS += -D__SMP__
AFLAGS += -D__SMP__
endif

ifdef CONFIG_MODVERSIONS
CFLAGS += -DMODVERSIONS -include $(RTLINUX)/include/linux/modversions.h
endif

CFLAGS += -DMODULE -D__KERNEL__ -Wall -Wstrict-prototypes
CC		= ${CROSS_COMPILE}gcc
endif

HOSTCC		= gcc
LD		= ${CROSS_COMPILE}ld

KERNELRELEASE=$(shell echo `grep UTS_RELEASE $(RTLINUX)/include/linux/version.h 2>/dev/null | cut -d\" -f2`)
# KPL = Kernel Patch Level
KPL:=$(shell echo $(KERNELRELEASE) | awk -F \. '{print $$2}')
RPMKERNELRELEASE=$(shell echo $(KERNELRELEASE) | sed 's/-/_/g')

# include $(RTL)/Rules.make

MODULE_DIRS =  main fifos schedulers system measurement

NEEDS_PATHS_DIRS =  $(EXAMPLE_DIRS) $(MODULE_DIRS)
RTLINUX_PATH_COMMAND := "RTLINUX_DIR = $(RTLINUX)"
INCLUDE := -I$(RTLINUX)/include -I$(RTL)/include -I$(RTL)/include/compat 
DEPINCLUDE = -I$(RTLINUX)/include -I$(RTL)/include
INCLUDE_EMU = "-I$(RTLINUX)/include -I$(RTL)/include"
INCLUDE_COMMAND = "INCLUDE= $(INCLUDE)"

IDIR=/usr/rtlinux-$(RELVER)
INSTDIR=$(DESTDIR)/$(IDIR)
INC_INSTDIR=$(INSTDIR)/include
LIB_INSTDIR=$(INSTDIR)/lib
BIN_INSTDIR=$(INSTDIR)/bin
EXAMPLE_INSTDIR=$(INSTDIR)/examples
MOD_INSTDIR=$(INSTDIR)/modules
DOC_INSTDIR=$(INSTDIR)/doc
#MOD_INSTDIR=$(DESTDIR)/lib/modules/$(KERNELRELEASE)/misc
DEV_INSTDIR=$(DESTDIR)/dev

.EXPORT_ALL_VARIABLES:

ifneq ($(wildcard .config),)
include .config
all: .depend include/rtl_version.h schedulers modules drivers examples
	@echo
	@echo 'Now do "sh scripts/insrtl" to install the modules'
else
all: checklinux .config
	@echo
	@echo 'Now do "make" to build the RTLinux modules'
endif

# ifdef CONFIG_RTL_TRACER
# INCLUDE := -I$(RTL)/tracer $(INCLUDE)
# endif

ifeq ($(CONFIG_MBUFF),y)
DRIVER_DIRS += mbuff 
endif

ifeq ($(CONFIG_RTSOCK),y)
NETWORK_DIRS += rtsock
endif

ifeq ($(CONFIG_RTL_LWIP),y)
NETWORK_DIRS += lwIP
endif

ifeq ($(CONFIG_RT_COMM),y)

DRIVER_DIRS += rt_com
endif

ifdef CONFIG_RTL_DEBUG
CFLAGS += -g
else
CFLAGS += -fomit-frame-pointer
endif

EXAMPLE_DIRS = frank measurements hello hello_dev_mem mutex \
		misc parallel kernel_resources large_mem pwm 

ifdef CONFIG_RTL_NFIFOS
EXAMPLE_DIRS    += fifos
endif

ifeq ($(CONFIG_OC_PBARRIERS),y)
EXAMPLE_DIRS += pbarriers
endif

ifeq ($(CONFIG_RTSOCK),y)
EXAMPLE_DIRS += rtsock
endif

ifeq ($(CONFIG_MBUFF),y)
EXAMPLE_DIRS += hello_mbuff
endif

ifdef CONFIG_PROC_FS
EXAMPLE_DIRS    += proc
endif

ifdef CONFIG_RTL_FP_SUPPORT
EXAMPLE_DIRS += fp
endif

ifdef CONFIG_RTL_USE_V1_API
EXAMPLE_DIRS += v1api/frank v1api/measurements v1api/sound v1api/parallel
ifdef CONFIG_RTL_FP_SUPPORT
EXAMPLE_DIRS += v1api/fp
endif
endif

ifeq ($(CONFIG_PSC),y)
MODULE_DIRS += psc
endif

ifeq ($(CONFIG_OC_PMQUEUE),y)
MODULE_DIRS += pmqueue
endif

ifdef CONFIG_RTL_TRACER
MODULE_DIRS += tracer
endif

ifeq ($(CONFIG_RTL_DEBUGGER),y)
MODULE_DIRS += debugger
endif

ifdef CONFIG_RTL_V1_API
ifndef CONFIG_SMP
MODULE_DIRS += semaphores
endif
endif

MODULE_DIRS += regression

CFLAGS += -D__RTL__ -D_LOOSE_KERNEL_NAMES

CXXFLAGS := $(CFLAGS) $(INCLUDE) -I$(RTL)/include/posix -fno-exceptions -fno-rtti
CFLAGS += -O2

# Start RTL-CONFIG This section is used to create the rtl.config file
# as SPECIFICALLY tailored for RTLinux v. 3.0.  This will/may change
# for v3.1. Note that the CONFIG_*** series of flags are intended to
# be sent out to the users via "rtl-config --xxx".  Thus, do not put
# anything here that is not intended to be seen by the user during
# normal operation of RTLinux.
CONFIG_CFLAGS   := -O2
CONFIG_CXXFLAGS := -fno-exceptions -fno-rtti
CONFIG_LIBS_DIR := $(LIB_INSTDIR)
CONFIG_INCLUDE  := $(INCLUDE) -I$(RTL)/include/posix 
CONFIG_FLAGS    := $(CFLAGS)
RTL_VERSION_MAJOR=$(shell echo $(VERSION) | cut -d\. -f1 -s)
RTL_VERSION_MINOR=$(shell echo $(VERSION) | cut -d\. -f2 -s)
LINUX_VERSION=$(shell echo $(KERNELRELEASE) | cut -d\. -f1 -s)
LINUX_PATCHLEVEL=$(shell echo $(KERNELRELEASE) | cut -d\. -f2 -s)
LINUX_SUBLEVEL=$(shell echo $(KERNELRELEASE)|cut -d\. -f3 -s|cut -d\- -f1)
LINUX_EXTRAVERSION=$(shell echo $(KERNELRELEASE) | cut -d\- -f2 -s) 
PARSED_CONFIG_INCLUDE=$(shell echo `echo $(CONFIG_INCLUDE) | \
	sed -e s/-I// | \
	sed -e s/-I/:/g -e s/\ //g`)
PARSED_CONFIG_LIBS=$(shell echo `echo $(CONFIG_LIBS_DIR) | \
	sed -e s/-L// | \
	sed -e s/-L/:/g -e s/\ //g`)
# End of RTL-CONFIG

CFLAGS += $(INCLUDE) -I$(RTL)/include/posix

include/rtl_version.h: ./Makefile
#	@echo \#define RTLINUX_VERSION_MAJOR $(RTL_VERSION_MAJOR) > .ver
#	@echo \#define RTLINUX_VERSION_MINOR $(RTL_VERSION_MINOR) >> .ver
#	@echo \#define RTLINUX_VERSION_PRERELEASE $(PRERELEASE) >> .ver
	@echo '#define RTLINUX_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))' >>.ver
	@echo \#define RTLINUX_VERSION_CODE RTLINUX_VERSION\($(RTL_VERSION_MAJOR),$(RTL_VERSION_MINOR),$(PRERELEASE)\) >> .ver
	@mv -f .ver $@

devices: dummy
	@echo
	@echo -n "Testing for the mbuff device... "
	@mkdir -p $(DEV_INSTDIR)
	@if [ ! -e $(DEV_INSTDIR)/mbuff ]; then \
	   mknod $(DEV_INSTDIR)/mbuff c 10 254 && echo "mknod $(DEV_INSTDIR)/mbuff c 10 254"; \
	else \
	   echo "already exists."; \
	fi
	@echo -n "Testing for FIFOs... "
	@if [ ! -e $(DEV_INSTDIR)/rtf10 ]; then \
		rm -f $(DEV_INSTDIR)/rtf*; \
		for i in `seq 0 63`; do \
			mknod $(DEV_INSTDIR)/rtf$$i c 150 $$i; \
		done; \
		echo "created $(DEV_INSTDIR)/rtf0 - 63 (major 150)"; \
	else \
	   echo "already exist."; \
	fi
	@echo -n "Testing for RTSOCKETS... "
	@if [ ! -e $(DEV_INSTDIR)/rtsock0 ]; then \
		rm -f $(DEV_INSTDIR)/rtf*; \
		for i in `seq 0 5`; do \
			mknod $(DEV_INSTDIR)/rtsock$$i c 121 $$i; \
		done; \
		echo "created $(DEV_INSTDIR)/rtsock0 - 5 (major 121)"; \
	else \
	   echo "already exist."; \
	fi
	@echo

modules: rtl.mk dummy
	@mkdir -p modules
	@set -e;
	@for i in $(MODULE_DIRS); do \
	$(MAKE) -C $$i; \
	done;

examples: dummy
	@set -e
	@echo
	@for i in $(EXAMPLE_DIRS); do\
	$(MAKE) -C  examples/$$i; \
	done

regression: modules
	./scripts/regression.sh

drivers: dummy
	@set -e
	@echo
ifdef DRIVER_DIRS
	@for i in $(DRIVER_DIRS); do\
	$(MAKE) -C  drivers/$$i; \
	done
endif
ifdef NETWORK_DIRS
	@for i in $(NETWORK_DIRS); do\
	$(MAKE) -C  network/$$i; \
	done
endif

# the touch are necessary for CVS - as CVS dischars empty files...
symlinks: dummy
	rm -f include/arch main/arch schedulers/arch
	ln -s $(ARCH) include/arch
	ln -s $(ARCH) main/arch
	ln -s $(ARCH) schedulers/arch
	touch schedulers/i386/sw.S
	touch schedulers/i386/Makefile
	touch main/arch/arch.c

checklinux:
	@echo Using RT-Linux kernel source tree in $(RTLINUX)
	@if [ ${ARCH} != ppc ] ; then \
		if [ -z "`fgrep EXTRAVERSION $${RTLINUX}/Makefile | fgrep rtl`" ]; then \
		echo Please patch the kernel with the Real-Time Linux patch; \
		exit 1; \
	fi; fi
	@if [ ! -e $(RTLINUX)/.config ]; then \
		echo Please configure the Linux kernel first; \
		exit 1; \
	fi
ifdef CONFIG_MODVERSIONS
	@if [ ! -e $(RTLINUX)/include/linux/modversions.h ]; then \
		echo Please run make dep in the Linux kernel source first; \
		exit 1; \
	fi
endif
# ifdef CONFIG_APM
# 	@echo Warning: APM support is enabled.
# endif
ifndef CONFIG_RTLINUX
	@if [ ${ARCH} != ppc ] ; then \
		@echo 'You need to enable hard realtime support (CONFIG_RTLINUX) in the kernel'; \
	fi
	@exit 1
endif

.config:
	$(MAKE) config
#	@if [ -z $$DISPLAY ]; then \
#		$(MAKE) menuconfig; \
#	else \
#		$(MAKE) xconfig; \
#	fi

# click-through license rule -Nathan
${RTL}/.rtlinuxaccepted:
	$(SHELL) scripts/click_through.sh ${RELVER}

config: ${RTL}/.rtlinuxaccepted symlinks
	$(SHELL) scripts/Configure scripts/config.in

oldconfig: ${RTL}/.rtlinuxaccepted symlinks
	$(SHELL) scripts/Configure -d scripts/config.in

menuconfig: ${RTL}/.rtlinuxaccepted symlinks
	$(MAKE) -C scripts ncurses
	$(MAKE) -C scripts lxdialog
	$(SHELL) scripts/Menuconfig scripts/config.in

xconfig: ${RTL}/.rtlinuxaccepted symlinks
	$(MAKE) -C scripts tkparse
	@if [ -f /usr/local/bin/wish ];	then \
		echo '#!'"/usr/local/bin/wish -f" > kconfig.tk; \
	else \
		echo '#!'"/usr/bin/wish -f" > kconfig.tk; \
	fi
	cat scripts/header.tk >> ./kconfig.tk
	scripts/tkparse < scripts/config.in >> kconfig.tk
	echo "set defaults \"schedulers/${ARCH}/defconfig\"" >> kconfig.tk
	echo "set ARCH \"${ARCH}\"" >> kconfig.tk
	cat scripts/tail.tk >> kconfig.tk
	chmod 755 kconfig.tk
	touch include/rtl_conf.h
	wish -f kconfig.tk
	rm -f kconfig.tk

dep: .config rtl.mk rtl.config
	rm -f .depend
	$(MAKE) .depend

.depend: scripts/mkdep
# dummy symlinks .config scripts/mkdep
	@echo Kernel version $(KERNELRELEASE)
	@if [ ! -f $(RTLINUX)/.config -o ! -f $(RTLINUX)/include/linux/version.h ]; then\
	echo You must do a make config and make dep in $(RTLINUX); \
	exit 1; \
	fi
	@mkdir -p modules
	for x in `find . -name Makefile`; do \
		F=`echo $$x|sed 's/Makefile$$//g'`; \
		(cd $$F; $$RTL/scripts/mkdep *.c *.h *.S > .depend 2>/dev/null); \
	done
	@rm -f .hdepend
	@find $(subst -I,,$(DEPINCLUDE)) -name SCCS -prune -or -follow -name \*.h ! -name modversions.h -print |xargs scripts/mkdep >> .hdepend

rtl.mk: .config rtl.config
	@echo "#Automatically generated by RTLinux Makefile"  > rtl.mk;
	@echo  RTL_DIR = $(RTL) >>rtl.mk
	@echo  $(RTLINUX_PATH_COMMAND) >> rtl.mk;
	@echo  $(INCLUDE_COMMAND) >> rtl.mk;
	@echo  CFLAGS = $(CFLAGS) >> rtl.mk;
	@echo  ARCH = $(ARCH) >> rtl.mk;
	@echo  CC = $(CC) >> rtl.mk;
	@echo  CXXFLAGS = $(CXXFLAGS) >> rtl.mk;
	@cp -f rtl.mk debugger
	@cp -f rtl.mk tracer

rtl.config: .config
	@echo "#Automatically generated by RTLinux Makefile" >rtl.config;
	@echo ""                                            >>rtl.config;
	@echo "# Version information"                       >>rtl.config;
	@echo  RTL_VERSION_MAJOR=\"$(RTL_VERSION_MAJOR)\"   >>rtl.config;
	@echo  RTL_VERSION_MINOR=\"$(RTL_VERSION_MINOR)\"   >>rtl.config;
	@echo  RTL_VERSION_EXTRA=\"$(RELEASE)\"             >>rtl.config;
	@echo  LINUX_VERSION=\"$(LINUX_VERSION)\"           >>rtl.config;
	@echo  LINUX_PATCHLEVEL=\"$(LINUX_PATCHLEVEL)\"     >>rtl.config;
	@echo  LINUX_SUBLEVEL=\"$(LINUX_SUBLEVEL)\"         >>rtl.config;
	@echo  LINUX_EXTRAVERSION=\"$(LINUX_EXTRAVERSION)\" >>rtl.config;
	@echo ""                                            >>rtl.config;
	@echo "# Directory information"                     >>rtl.config;
	@echo  RTL_DIR=\"$(INSTDIR)\"                       >>rtl.config;
	@echo  RTLINUX_DIR=\"$(RTLINUX)\"                   >>rtl.config;
	@echo  MODULES_DIR=\"$(MOD_INSTDIR)\"               >>rtl.config;
	@echo  USER_INC_DIR=\"$(INC_INSTDIR)\"              >>rtl.config;
	@echo  DOC_DIRS=\"$(DOC_INSTDIR)\"                  >>rtl.config;
	@echo  LIBS_DIR=\"$(PARSED_CONFIG_LIBS)\"           >>rtl.config;
	@echo  INCLUDE_DIR=\"$(PARSED_CONFIG_INCLUDE)\"     >>rtl.config;
	@echo ""                                            >>rtl.config;
	@echo "# Flag information"                          >>rtl.config;
	@echo  CFLAGS=\"$(CFLAGS)\"                         >>rtl.config;
	@echo  CXXFLAGS=\"$(CXXFLAGS)\"                     >>rtl.config;
	@echo ""                                            >>rtl.config;
	@echo "# System information"                        >>rtl.config;
	@echo  ARCH=\"$(ARCH)\"                             >>rtl.config;
	@echo  CC=\"$(CC)\"                                 >>rtl.config;

modules_install:
	@-mkdir -p $(MOD_INSTDIR)
	@-mkdir -p $(DESTDIR)/lib/modules/$(KERNELRELEASE)/misc/
	@echo "Instaling modules in" $(MOD_INSTDIR)
	@for mod in modules/*.o ; do \
	   install -c -m 644 $$mod $(MOD_INSTDIR); \
	   ln -sf $(IDIR)/modules/`basename $$mod` $(DESTDIR)/lib/modules/$(KERNELRELEASE)/misc/`basename $$mod`; \
	done
#
# Don't bother doing a depmod if we're building an rpm since it will
# not be up-to-date and will need to be re-run by the rpm itself  -- Cort
#
ifndef DESTDIR
	-depmod -a
endif


#
# Only install Linux if there's a DESTDIR since we're creating a RPM
#  -- Cort
#
linux_install:
ifdef DESTDIR
	@echo "Installing Linux headers to" $(DESTDIR)/usr/rtlinux_kernel_$(KERNELRELEASE)
	@mkdir -p $(DESTDIR)/usr/rtlinux_kernel_$(KERNELRELEASE)/include/linux
	@mkdir -p $(DESTDIR)/usr/rtlinux_kernel_$(KERNELRELEASE)/include/$(ARCH)
	@ln -sf /usr/rtlinux_kernel_$(KERNELRELEASE)/include/$(ARCH) $(DESTDIR)/usr/rtlinux_kernel_$(KERNELRELEASE)/include/asm
	@for head in `cd $(RTLINUX);find include -type d | fgrep -v SCCS`; do \
		mkdir -p $(DESTDIR)/usr/rtlinux_kernel_$(KERNELRELEASE)/$$head; \
	done
	@for head in `cd $(RTLINUX);find include -name '*.h' | fgrep -v SCCS`; do \
		install -m 644 $(RTLINUX)/$$head $(DESTDIR)/usr/rtlinux_kernel_$(KERNELRELEASE)/$$head; \
	done
endif

examples_install:
	@echo "Installing examples to" $(EXAMPLE_INSTDIR)
	@cd examples; tar cf - `find . -name '*.c' -o -name 'Makefile' -o -name '*.h' -o -name README -o -name '*.au'| grep -v SCCS`| tar -C $(EXAMPLE_INSTDIR) -xf -

#
# this is a littl more genereic, it does not require to pass all files that 
# need to be installed explicidly , it simply does a make clean , and what
# is left after make clean is then copied to /usr/rtlinux/
#
#examples_install:
#	@echo "Installing examples to" $(EXAMPLE_INSTDIR)
#	@mkdir -p $(EXAMPLE_INSTDIR) 
#	@set -e; \
#	for dir in $(EXAMPLE_DIRS); do \
#		$(MAKE) -C examples/$$dir clean; \
#	done
#	@cd examples ; tar -cf - . | tar -xf - -C  $(EXAMPLE_INSTDIR) 

dirs_install:
	@mkdir -p $(INC_INSTDIR)
	@mkdir -p $(BIN_INSTDIR)
	@mkdir -p $(LIB_INSTDIR)
	@mkdir -p $(MOD_INSTDIR)
	@mkdir -p $(DEV_INSTDIR)
	@mkdir -p $(EXAMPLE_INSTDIR)
	@mkdir -p $(DESTDIR)/usr/lib

install: dirs_install modules_install devices all linux_install examples_install
	@echo "Installing headers to" $(INC_INSTDIR)
	@mkdir -p $(INC_INSTDIR)/$(ARCH)
	@for head in include/*.h; do \
	   install -c -m 644 $$head $(INC_INSTDIR); \
	done
	@for head in include/$(ARCH)/*.h; do \
	   install -c -m 644 $$head $(INC_INSTDIR)/$(ARCH); \
	done
	@mkdir -p $(INC_INSTDIR)/posix/sys
	@for head in include/posix/*.h; do \
	   install -c -m 644 $$head $(INC_INSTDIR)/posix/; \
	done
	@for head in include/posix/sys/*.h; do \
	   install -c -m 644 $$head $(INC_INSTDIR)/posix/sys/; \
	done
	@mkdir -p $(INC_INSTDIR)/compat/linux
	@mkdir -p $(INC_INSTDIR)/compat/asm
	@for head in include/compat/linux/*.h; do \
	   install -c -m 644 $$head $(INC_INSTDIR)/compat/linux/; \
	done
	@for head in include/compat/asm/*.h; do \
	   install -c -m 644 $$head $(INC_INSTDIR)/compat/asm/; \
	done
	@mkdir -p $(DESTDIR)/usr/include
	ln -sf $(IDIR)/include/ $(DESTDIR)/usr/include/rtlinux
	@rm -f $(INC_INSTDIR)/arch
	@ln -s $(ARCH) $(INC_INSTDIR)/arch
	@rm -f $(INC_INSTDIR)/linux $(INC_INSTDIR)/asm
ifeq ($(CONFIG_PSC),y)
	@echo "Installing libraries to" $(LIB_INSTDIR)
	install -c -m 644 psc/librtlinux.a $(LIB_INSTDIR)
	install -c -m 644 psc/librtlinuxpSOS.a $(LIB_INSTDIR)
	ln -sf $(IDIR)/lib/librtlinux.a $(DESTDIR)/usr/lib/librtlinux.a
	ln -sf $(IDIR)/lib/librtlinuxpSOS.a $(DESTDIR)/usr/lib/librtlinuxpSOS.a
endif
	$(MAKE) -C regression install
	install -c -m 755 scripts/regression.sh $(INSTDIR)
	install -c -m 755 scripts/rtl-config $(BIN_INSTDIR)
	@mkdir -p $(DESTDIR)/etc/rc.d/init.d
	install -c -m 755 scripts/rtlinux $(DESTDIR)/etc/rc.d/init.d/rtlinux
	install -c -m 644 rtl.config $(INC_INSTDIR)/rtl.config
	install -c -m 644 rtl.mk $(INC_INSTDIR)/rtl.mk
	install -c -m 644 Rules.make $(INSTDIR)
	ln -fs include/rtl.mk $(INSTDIR)/rtl.mk
	@mkdir -p $(DESTDIR)/usr/bin
	ln -fs $(IDIR)/bin/rtl-config $(DESTDIR)/usr/bin/rtl-config
	ln -fs $(IDIR)/bin/rtlinux $(DESTDIR)/usr/bin/rtlinux
	ln -fs $(DESTDIR)/etc/rc.d/init.d/rtlinux $(BIN_INSTDIR)/rtlinux
	rm -f /usr/rtlinux
	ln -fs $(IDIR) $(DESTDIR)/usr/rtlinux
#
# If we have a DESTDIR then we're building a rpm, so
# install the kernel/system.map, too.  However, the .spec
# needs the /boot directory so create that anyway.
#   -- Cort
#
	@mkdir -p $(DESTDIR)/boot
ifneq ($(DESTDIR),)
	install -c -m 644 $(RTLINUX)/System.map $(DESTDIR)/boot/System.map-$(KERNELRELEASE)-$(RELVER)
	install -c -m 644 $(RTLINUX)/vmlinux $(DESTDIR)/boot/vmlinux-$(KERNELRELEASE)-$(RELVER)
ifeq ($(ARCH),i386)
	install -c -m 644 $(RTLINUX)/arch/i386/boot/bzImage $(DESTDIR)/boot/bzImage-$(KERNELRELEASE)-$(RELVER)
endif
endif
	@cp -f rtl.mk debugger
	@cp -f rtl.mk tracer
	@echo

clean:  dummy
	@rm -f rtl.mk rtl.config
	@echo  RTL_DIR = $(RTL) >rtl.mk
	@set -e; \
        for i in $(EXAMPLE_DIRS); do \
           $(MAKE) -C examples/$$i clean; \
        done
ifdef NETWORK_DIRS
	for i in $(NETWORK_DIRS); do \
           $(MAKE) -C network/$$i clean; \
        done
endif
ifdef DRIVER_DIRS
	for i in $(DRIVER_DIRS); do \
           $(MAKE) -C drivers/$$i clean; \
        done
endif
	for i in $(MODULE_DIRS); do \
	   $(MAKE) -C  $$i clean ; \
	done
	$(MAKE) -C measurement clean
	$(MAKE) -C regression clean
	$(MAKE) -C scripts clean
	rm -rf modules kconfig.tk .menuconfig.log scripts/rtlinux.spec regression.log
	find . -type f \( -name .depend -o -name '#*#' -o -name '*~' -o -name '*.o' -o \( -name core -a \! -type d \) -or -name rtl.mk -or -name '*.a' \) -exec /bin/rm -r '{}' \;
	rm -f scripts/mkdep

distclean: clean
	rm -f .config main/arch schedulers/arch include/rtl_conf.h tags include/arch \
		scheduler/arch debugger/arch include/rtl_version.h \
		${RTL}/.rtlinuxaccepted \
		${RTL}/.rtlinux_license_tmp ${RTL}/.hdepend

rpm:
	rm -rf /usr/rtlinux-${RELVER}
	bk export -w /usr/rtlinux-${RELVER}
	rm -rf /usr/rtlinux-${RELVER}/BitKeeper
	tar -C /usr -czf /usr/src/rpm/SOURCES/rtlinux-${RELVER}.tar.gz rtlinux-${RELVER}
	rm -rf /tmp/rtlinux-${VERSION}
	mkdir /tmp/rtlinux-${VERSION}
	tar -C /tmp -czf /usr/src/rpm/SOURCES/rtlinux.tar.gz rtlinux-${VERSION}
	rm -f scripts/rtlinux.spec
	echo '%define rtlversion' $${VERSION} > scripts/rtlinux.spec
	echo '%define rtlrelease' $${RELEASE} >> scripts/rtlinux.spec
	echo '%define kernelrelease' $${RPMKERNELRELEASE} >> scripts/rtlinux.spec
ifdef CONFIG_SMP
	cat scripts/rtlinux.spec.tmpl >> scripts/rtlinux.spec
else 
	sed 's/\.smp//g' scripts/rtlinux.spec.tmpl >> scripts/rtlinux.spec
endif
	$(MAKE) -C /usr/rtlinux-${RELVER} oldconfig
	$(MAKE) -C /usr/rtlinux-${RELVER}
	@rm -rf /tmp/rtlinux-root
	@mkdir -p /tmp/rtlinux-root/usr/rtlinux-${RELVER}
	bk export -w /tmp/rtlinux-root/usr/rtlinux-${RELVER}
	$(MAKE) -C /usr/rtlinux-${RELVER} install DESTDIR=/tmp/rtlinux-root
	rpm --short-circuit -bb scripts/rtlinux.spec
	rm -rf /usr/src/rpm/SOURCES/rtlinux.tar.gz /tmp/rtlinux-${VERSION}

scripts/mkdep: scripts/mkdep.c
	${HOSTCC} -Wall -O2 -o scripts/mkdep scripts/mkdep.c

TAGS:
	rm -f tags
	find . -name "*.[c,h]" -print | uniq | egrep \
		'SCCS+|i386+|ppc+|alpha+|mips+' -v | xargs etags -a 
	find main/${ARCH} include/${ARCH} schedulers/${ARCH} debugger/${ARCH} \
		-name "*.[c,h]" -print | uniq | grep -v SCCS | xargs etags -a

tags:
	rm -f tags
	find . -name "*.[c,h]" -print | uniq | egrep \
		'SCCS+|i386+|ppc+|alpha+|mips+' -v | xargs ctags -a 
	find main/${ARCH} include/${ARCH} schedulers/${ARCH} debugger/${ARCH} \
		-name "*.[c,h]" -print | uniq | grep -v SCCS | xargs ctags -a

snapshot:
	@if [ "" = "$(VER)" ]; then \
		echo usage: make snapshot VER=3.0-pre6e; \
		exit 1; \
	fi
	@echo making rtlinux-$(VER).tar.gz
	rm -rf /tmp/rtltmp-$(VER)
	mkdir -p /tmp/rtltmp-$(VER)
	bk export -w /tmp/rtltmp-$(VER)/rtlinux-$(VER)
	scripts/rtldoc /tmp/rtltmp-$(VER)/rtlinux-$(VER)
	(cd /tmp/rtltmp-$(VER); \
	rm -rf rtlinux-$(VER)/BitKeeper; \
	tar czf rtlinux-$(VER).tar.gz rtlinux-$(VER))
	mv /tmp/rtltmp-$(VER)/rtlinux-$(VER).tar.gz .
	rm -rf /tmp/rtltmp-$(VER)

.PHONY: dummy modules schedulers devices fifos pmqueue
