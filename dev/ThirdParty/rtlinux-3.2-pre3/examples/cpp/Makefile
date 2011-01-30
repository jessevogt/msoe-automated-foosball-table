all: app.o rtl_cpp.o

include ../../rtl.mk

CRTBEGIN=`g++ -print-file-name=crtbegin.o` 
CRTEND=`g++ -print-file-name=crtend.o`

app.o: hello.o
	$(LD) -r -o app.o $(CRTBEGIN) hello.o $(CRTEND)

clean:
	rm -f *.o

test: all
	@echo "This is a simple RTLinux program"
	@echo "First we remove any existing rtl-modules"
	@echo "You may see error warnings from \"make\" - ignore them"
	@echo "Type <return> to continue"
	@read junk
	-rmmod sound
	-rmmod rt_process
	-rmmod frank_module
	-rmmod rtl_cpp
	(cd $(RTL_DIR); scripts/rmrtl)
	@echo "Now insert the fifo and scheduler"
	@echo "Type <return> to continue"
	@read junk
	(cd $(RTL_DIR); scripts/insrtl)
	@echo "Now insert the C++ support module"
	insmod rtl_cpp.o
	@echo "Now start the real-time tasks  module"
	@echo "Type <return> to continue"
	@read junk
	@insmod app.o
	@sleep 3
	@echo "Now let's stop the application"
	@echo "Type <return> to finish"
	@read junk
	@rmmod app
	@rmmod rtlcpp

include $(RTL_DIR)/Rules.make
