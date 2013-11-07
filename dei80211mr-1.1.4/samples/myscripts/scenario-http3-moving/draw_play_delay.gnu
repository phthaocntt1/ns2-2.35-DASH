# gnuplot scripts
set terminal postscript eps color 
set xlabel "time (s)"
set ylabel "Delay (s)"
set output "Play_delay_6.eps"
plot "play_delay_6.log" u 1:2 w lp title "client"
