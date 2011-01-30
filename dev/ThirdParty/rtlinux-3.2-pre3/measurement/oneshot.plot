#set terminal postscript portrait color solid 10
#set size 1.2,1.2

set title "One-Shot Timing Measurement"
set xlabel "Sample #"
set ylabel "Actual Period (us)"
#set output "oneshot.ps"
plot "gnuplot.out" using 3 title "foo" with lines 1, "gnuplot.out" using 1:3:2:4 notitle with errorbars 1
pause 100
