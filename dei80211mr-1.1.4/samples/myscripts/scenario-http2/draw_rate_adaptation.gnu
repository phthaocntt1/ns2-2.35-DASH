# gnuplot scripts
set terminal postscript eps color 
set xlabel "time (s)"
set ylabel "Physical Rate"
set output "Rate_Adaptation_History_AP_5.eps"
plot "AP_rate_5.file" u 1:4 w p title "client"
