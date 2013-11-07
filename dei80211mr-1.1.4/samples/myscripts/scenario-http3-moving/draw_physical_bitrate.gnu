# gnuplot scripts
set terminal postscript eps color 
set xlabel "time (s)"
set ylabel "Physical Bitrate (Kb/s)"
set output "Physical_bitrate_client_6.eps"
plot "physical_bitrate_client_6" u 1:2 w lp title "measured bitrate",\
"physical_bitrate_client_6" u 1:3 w lp title "predicted bitrate"
