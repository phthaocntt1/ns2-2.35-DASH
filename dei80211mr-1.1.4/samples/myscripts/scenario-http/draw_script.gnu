# gnuplot scripts
set terminal postscript eps color 
set xlabel "time (s)"
set ylabel "Requested Bitrate (Mb/s)"
set output "client_request.eps"
plot "download_history_1.txt" u 1:2 w lp title "bandwidth"
set xlabel "time (s)"
set ylabel "segment id"
set output "client_play.eps"
plot "play_history_1.txt" u 2:1 w lp title "segment"
 
