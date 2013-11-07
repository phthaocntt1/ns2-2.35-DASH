# gnuplot scripts
set terminal postscript eps color 
set xlabel "time (s)"
set ylabel "# UDP clients"
set output "udp_nodes.eps"
plot "nodes.stat" u 1:2 w lp title "Number of UDP clients"
