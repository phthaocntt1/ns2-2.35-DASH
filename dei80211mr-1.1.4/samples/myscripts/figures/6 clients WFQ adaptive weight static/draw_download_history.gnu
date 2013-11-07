# gnuplot scripts
set terminal postscript eps color 
set xlabel "time (s)"
set ylabel "Bitrate(Mb/s)"
set title "Download History 6 clients WFQ equal weight dynamic"
set output "download_history_all.eps"
plot "download_history_1.txt" u 1:2 w lp title "android client 1",\
 "download_history_2.txt" u 1:2 w lp title "android client 2",\
"download_history_3.txt" u 1:2 w lp title "AIMD client 3", \
"download_history_4.txt" u 1:2 w lp title "AIMD client 4", \
"download_history_5.txt" u 1:2 w lp title "VLC client 5", \
"download_history_6.txt" u 1:2 w lp title "VLC client 6"
