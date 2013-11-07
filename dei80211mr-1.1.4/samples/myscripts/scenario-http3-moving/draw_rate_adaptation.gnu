# gnuplot scripts
set terminal postscript eps color 
set xlabel "time (s)"
set ylabel "Physical Rate"
set output "Rate_Adaptation_History_AP_1.eps"
plot "RateChangeAP_1.log" u 1:4 w lp title "client"
