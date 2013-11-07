BEGIN {


for(j=0;j<RateNumber;j++)
{
rate_table[j]=0;
rate_time_table[j]=0.0;
}

# index 0 6Mb
# index 1 9Mb
# index 2 12Mb
# index 3 18Mb
# index 4 24Mb
# index 5 36Mb
# index 6 48Mb
# index 7 54Mb

rate_change_count=0;
prev_time=0.0;
prev_rate_index=7;
initialized=0;
finish_time=0.0;


}

{

time = $1;

src= $2;

dst = $3;

rate_index = $4;

if(initialized==0)
{
start_time=time;
initialized=1;
}

delta_time=time-prev_time;
rate_time_table[prev_rate_index]+=delta_time;
prev_time=time;
prev_rate_index=rate_index;
finish_time=time;

#increase rate change count
rate_change_count++;

#increase rate count
rate_table[rate_index]++;




}



END {

total_time=finish_time-start_time;

printf("total time %f\n",total_time);

#put the result to file
printf("# change count: %d\n",rate_change_count) >> FILENAME; 
printf("#  6Mb: %f time %f\n",rate_table[0]/rate_change_count,rate_time_table[0]/total_time) >> FILENAME;
printf("#  9Mb: %f time %f\n",rate_table[1]/rate_change_count,rate_time_table[1]/total_time) >> FILENAME;
printf("# 12Mb: %f time %f\n",rate_table[2]/rate_change_count,rate_time_table[2]/total_time) >> FILENAME;
printf("# 18Mb: %f time %f\n",rate_table[3]/rate_change_count,rate_time_table[3]/total_time) >> FILENAME;
printf("# 24Mb: %f time %f\n",rate_table[4]/rate_change_count,rate_time_table[4]/total_time) >> FILENAME;
printf("# 36Mb: %f time %f\n",rate_table[5]/rate_change_count,rate_time_table[5]/total_time) >> FILENAME;
printf("# 48Mb: %f time %f\n",rate_table[6]/rate_change_count,rate_time_table[6]/total_time) >> FILENAME;
printf("# 54Mb: %f time %f\n",rate_table[7]/rate_change_count,rate_time_table[7]/total_time) >> FILENAME;


}
