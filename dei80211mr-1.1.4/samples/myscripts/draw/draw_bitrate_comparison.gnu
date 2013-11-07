# gnuplot scripts
set terminal postscript eps color 
set xlabel "time (s)"
set ylabel "Bitrate(Kb/s)"
set title "Comparison of Bitrate History--1st client moving"
set output "bitrate_comparison.eps"
plot "total_bitrate_original.txt" u 1:2 w lp title "original",\
 "total_bitrate_0.25.txt" u 1:2 w lp title "adaptive"
