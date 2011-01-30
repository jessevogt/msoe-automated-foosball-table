all: hello.o posix_hello.o sem_hello.o

include ../../rtl.mk
clean:
	rm -f *.o

test: all
	@echo "This is the simplest RTLinux program"
	@echo "First we remove any existing rtl-modules"
	@echo "You may see error warnings from \"make\" - ignore them"
	@echo "Type <return> to continue"
	@read junk
	-rmmod sound
	-rmmod rt_process
	-rmmod frank_module
	(cd ../../; scripts/rmrtl)
	@echo "Now insert the fifo and scheduler"
	@echo "Type <return> to continue"
	@read junk
	(cd ../../; scripts/insrtl)
	@echo "Now start the real-time tasks  module"
	@echo "Type <return> to continue"
	@read junk
	@insmod hello.o
	@sleep 3
	@echo "Now let's stop the application"
	@echo "Type <return> to finish"
	@read junk
	@rmmod hello

include $(RTL_DIR)/Rules.make
