# gnuplot scripts
set terminal postscript eps color 
set xlabel "time (s)"
set ylabel "Requested Bitrate (Mb/s)"
set output "Download_History_6.eps"
plot "download_history_6.txt" u 1:4 w lp title "actually downloaded",\
 "download_history_6.txt" u 1:2 w lp title "client requested" 
