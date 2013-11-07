BEGIN {

output_file_1="RateChangeAP_1.log";
output_file_2="RateChangeAP_2.log";
output_file_3="RateChangeAP_3.log";
output_file_4="RateChangeAP_4.log";
output_file_5="RateChangeAP_5.log";

client_number=5;

for(i=1;i<=client_number;i++)
{
rate_change_count[i]=0;
time_initialized[i]=0;
finish_time[i]=0;
start_time[i]=0;
prev_rate_index[i]=7;
prev_time[i]=0;
}

RateNumber=8;

for(i=1;i<=client_number;i++)
for(j=0;j<RateNumber;j++)
{
rate_table[i,j]=0;
rate_time_table[i,j]=0;
}

# index 0 6Mb
# index 1 9Mb
# index 2 12Mb
# index 3 18Mb
# index 4 24Mb
# index 5 36Mb
# index 6 48Mb
# index 7 54Mb




}

{

time = $1;

src= $2;

dst = $3;

rate_index = $4;


#increase rate change count
rate_change_count[dst]++;

#increase rate count
rate_table[dst,rate_index]++;

#initialize start time
if(time_initialized[dst]==0)
{
time_initialized[dst]=1;
start_time[dst]=time;
}
#calculate time
rate_time_table[dst,prev_rate_index[dst] ]+=time-prev_time[dst];
prev_time[dst]=time;
prev_rate_index[dst]=rate_index;
finish_time[dst]=time;

output_file=sprintf("RateChangeAP_%d.log",dst);

print $0 > output_file;

}



END {

for(i=1;i<=client_number;i++)
{

total_time=finish_time[i]-start_time[i];
printf("for client %d, rate change count %d, total time %f\n",i,rate_change_count[i],total_time);

output_file=sprintf("RateChangeAP_%d.log",i);
printf("# change count: %d,total time %f\n",rate_change_count[i],total_time) > output_file;

#calculate the ratio for each rate
printf(" 6Mb: %f time %f\n",rate_table[i,0]/rate_change_count[i],rate_time_table[i,0]/total_time);
printf(" 9Mb: %f time %f\n",rate_table[i,1]/rate_change_count[i],rate_time_table[i,1]/total_time);
printf("12Mb: %f time %f\n",rate_table[i,2]/rate_change_count[i],rate_time_table[i,2]/total_time);
printf("18Mb: %f time %f\n",rate_table[i,3]/rate_change_count[i],rate_time_table[i,3]/total_time);
printf("24Mb: %f time %f\n",rate_table[i,4]/rate_change_count[i],rate_time_table[i,4]/total_time);
printf("36Mb: %f time %f\n",rate_table[i,5]/rate_change_count[i],rate_time_table[i,5]/total_time);
printf("48Mb: %f time %f\n",rate_table[i,6]/rate_change_count[i],rate_time_table[i,6]/total_time);
printf("54Mb: %f time %f\n\n",rate_table[i,7]/rate_change_count[i],rate_time_table[i,7]/total_time);

#put the result to file 
printf("#  6Mb: %f time %f\n",rate_table[i,0]/rate_change_count[i],rate_time_table[i,0]/total_time) > output_file;
printf("#  9Mb: %f time %f\n",rate_table[i,1]/rate_change_count[i],rate_time_table[i,1]/total_time) > output_file;
printf("# 12Mb: %f time %f\n",rate_table[i,2]/rate_change_count[i],rate_time_table[i,2]/total_time) > output_file;
printf("# 18Mb: %f time %f\n",rate_table[i,3]/rate_change_count[i],rate_time_table[i,3]/total_time) > output_file;
printf("# 24Mb: %f time %f\n",rate_table[i,4]/rate_change_count[i],rate_time_table[i,4]/total_time) > output_file;
printf("# 36Mb: %f time %f\n",rate_table[i,5]/rate_change_count[i],rate_time_table[i,5]/total_time) > output_file;
printf("# 48Mb: %f time %f\n",rate_table[i,6]/rate_change_count[i],rate_time_table[i,6]/total_time) > output_file;
printf("# 54Mb: %f time %f\n",rate_table[i,7]/rate_change_count[i],rate_time_table[i,7]/total_time)> output_file;


}


}
