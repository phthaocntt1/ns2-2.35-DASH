BEGIN {





prev_time=0;


output_file="play_delay_6.log"

printf("#play delay\n") > output_file;

}

{

seg_number = $1;

time= $2;

play_delay=time-prev_time-5.0;

if(play_delay >0.0 )
printf("%f %f\n",time,play_delay) > output_file ;

prev_time=time;

}
END {



}



