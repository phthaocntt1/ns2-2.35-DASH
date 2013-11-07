# gnuplot scripts
set terminal postscript eps color 
set xlabel "time (s)"
set ylabel "Index"
set output "Trend_Index_6.eps"
plot "trend_file_6.txt" u 3:7 w lp title "total index" ,\
  "trend_file_6.txt" u 3:8 w lp title "previous" ,\
  "trend_file_6.txt" u 3:9 w lp title "requested" 