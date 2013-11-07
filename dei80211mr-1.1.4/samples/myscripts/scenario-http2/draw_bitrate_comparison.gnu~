# gnuplot scripts
set terminal postscript eps color 
set xlabel "time (s)"
set ylabel "Bitrate(Kb/s)"
set title "Comparison of Bitrate History"
set output "bitrate_comparison.eps"
plot "total_bitrate.txt" u 1:2 w lp title "original",\
 "total_bitrate_2.txt" u 1:2 w lp title "adaptive"
