# gnuplot scripts
set terminal postscript eps color 
set xlabel "time (s)"
set ylabel "Packet Number (pkts)"
set output "Dropped_and_Retrans_at_MAC.eps"
plot "dropped.stat" u 1:2 w lp title "dropped",\
 "retrans.stat" u 1:2 w lp title "retransmitted"
