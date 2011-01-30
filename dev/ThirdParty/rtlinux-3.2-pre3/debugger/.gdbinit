# RTLinux Debugger macros
# Michael Barabanov (baraban@fsmlabs.com)
# Copyright (C) 2000, Finite State Machine Labs Inc.
# Released under the terms of the GPL


define di
disassemble $pc $pc+40
end

define dbg
target remote /dev/rtf10
end

define rdbg
target remote myhost.mycompany.com:5000
end

# define modaddsym
# shell if grep __insmod_`basename $arg0 .o`_S /proc/ksyms |grep -v rodata >.gdbtmp; then \
#  	cat .gdbtmp|sed 's/_S/ /;s/_L.*$//' |sort| awk 'BEGIN {printf "add-symbol-file $arg0 "} {printf "-T%s 0x%s ", $3, $1}' >.gdbtmp2; \
#  	else echo could not find address of `basename $arg0 .o`; exit 1; \
#  	fi
# so .gdbtmp2
# shell rm -f .gdbtmp .gdbtmp2
# end

define modaddsym
shell if grep __insmod_`basename $arg0 .o`_S.text /proc/ksyms >.gdbtmp; then \
	awk '{printf "add-symbol-file $arg0 0x%s\n", $1}' <.gdbtmp >.gdbtmp2; \
	else echo could not find address of `basename $arg0 .o`; exit 1; \
	fi
so .gdbtmp2
shell rm -f .gdbtmp .gdbtmp2
end

define modaddsched
shell awk '/RTL_DIR =/ { printf "modaddsym %s/modules/rtl_sched.o\n", $3 }' rtl.mk > .gdbtmp3
so .gdbtmp3
shell rm -f .gdbtmp3
# modaddsym ../modules/rtl_sched.o
end
