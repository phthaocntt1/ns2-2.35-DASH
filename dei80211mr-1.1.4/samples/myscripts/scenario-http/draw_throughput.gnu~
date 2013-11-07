# gnuplot scripts
set terminal postscript eps color 
set xlabel "time (s)"
set ylabel "throughput (pkts)"
set output "throughput_at_mac.eps"
plot "receive.stat" u 1:3 w lp title "node 1",\
 "receive.stat" u 1:4 w lp title "node 2" , \
"receive.stat" u 1:5 w lp title "node 3", \
"receive.stat" u 1:6 w lp title "node 4"
