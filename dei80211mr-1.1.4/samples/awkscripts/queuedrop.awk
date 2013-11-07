BEGIN {
count = 0;

dropped_node1=0;
dropped_node2=0;
dropped_node3=0;
dropped_node4=0;
dropped_other=0;


starttime=0;
endtime=0;

initialized=0;


}

{

left_bracket = $1;

queue_no = $2;

queue_len = $3;

right_bracket = $4;

queue_act= $5;

time = $6;

pkttype = $7;

a = $8;

source= $9;

destination= $10;





if(queue_act == "d") {


count++;

#count for each client
if(destination=="1")
dropped_node1++;
else if(destination=="2")
dropped_node2++;
else if(destination=="3")
dropped_node3++;
else if(destination=="4")
dropped_node4++;
else
dropped_other++;

if(initialized==0)
{
starttime=time;
initialized=1;
}
endtime=time;
}

}





END {
if(count==0)
printf("No packets have been dropped in the queuing process!\n");
else
{
printf("In the queuing process:\n Total packets dropped %d, start time %f, end time %f, duration %f\n",count, starttime, endtime, endtime-starttime);
printf("Client 1: %d, Client 2: %d, Client 3: %d, Client 4: %d, Others %d\n",dropped_node1,dropped_node2, dropped_node3, dropped_node4,dropped_other);
}
}
