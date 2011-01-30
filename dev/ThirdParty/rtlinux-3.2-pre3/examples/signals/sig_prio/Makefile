all: sig_prio.o

include ../../../rtl.mk


help:
	@echo "/******************************************************************/"	
	@echo "/******************************************************************/"	
	@echo "/*  In this program a thread sends a signal to a higher priority  */"
	@echo "/* thread. At this moment the higher priority thread should take  */"
	@echo "/* over the CPU. Then it suspends the signal generation to prove  */"
	@echo "/* that it has taken over the CPU.                                */"
	@echo "/******************************************************************/"	
	@echo "/******************************************************************/"	

clean:
	rm -f *.o

test: all help
	@echo "First we remove any existing rtl-modules"
	@echo "You may see error warnings from \"make\" - ignore them"
	@echo "Type <return> to continue"
	@read junk
	(cd ../../../; scripts/rmrtl)
	@echo "Now insert the fifo and scheduler"
	@echo "Type <return> to continue"
	@read junk
	(cd ../../../; scripts/insrtl)
	@echo "Now start the real-time tasks  module"
	@echo "Type <return> to continue"
	@read junk
	@tail -f /var/log/messages -n 0 &
	@insmod sig_prio.o
	@sleep 3
	@echo "Now let's stop the application"
	@echo "Type <return> to finish"
	@read junk
	@rmmod sig_prio
	@kill `pidof -s tail`

include ../../../Rules.make
