# gnuplot scripts
set terminal postscript eps color 
set xlabel "time (s)"
set ylabel "Bitrate(Mb/s)"
set title "Bitrate History 6 clients WFQ equal weight static"
set output "bitrate_history_all.eps"
plot "bitrate_history_1.txt" u 1:3 w lp title "android client 1",\
 "bitrate_history_2.txt" u 1:3 w lp title "android client 2",\
"bitrate_history_3.txt" u 1:3 w lp title "AIMD client 3", \
"bitrate_history_4.txt" u 1:3 w lp title "AIMD client 4", \
"bitrate_history_5.txt" u 1:3 w lp title "VLC client 5", \
"bitrate_history_6.txt" u 1:3 w lp title "VLC client 6"
