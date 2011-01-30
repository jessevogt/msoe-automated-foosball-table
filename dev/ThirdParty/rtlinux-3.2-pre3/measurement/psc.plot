#set terminal postscript portrait color solid 10

set title "RTLinux vs. Linux Periodic Timing"
set xlabel "Sample #"
set ylabel "Actual Period (us)"
#set output "PSC_vs_Linux.ps"
plot "gnuplot.out" using 3 title "RTLinux" with lines 1, "gnuplot.out" using 1:3:2:4 notitle with errorbars 1

# "gnuplot.out" using 6 title "Linux" with lines 2, "gnuplot.out" using 1:6:5:7 notitle with errorbars 2

pause 100
