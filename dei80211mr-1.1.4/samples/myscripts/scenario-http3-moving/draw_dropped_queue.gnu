# gnuplot scripts
set terminal postscript eps color 
set xlabel "time (s)"
set ylabel "Packet Number (pkts)"
set output "dropped_at_queue.eps"
plot "dropped_queue.stat" u 1:2 w lp title "n1 ifq",\
 "dropped_queue.stat" u 1:3 w lp title "n2 ifq",\
"dropped_queue.stat" u 1:4 w lp title "n3 ifq", \
"dropped_queue.stat" u 1:5 w lp title "n4 ifq", \
"dropped_queue.stat" u 1:6 w lp title "total ifq"
