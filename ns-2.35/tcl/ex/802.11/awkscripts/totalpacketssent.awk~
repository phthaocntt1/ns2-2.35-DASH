BEGIN {
count = 0;
count_temp=0;
count_mac=0;
totalsize=0;
totalsize_mac=0;

starttime=0;
endtime=0;
prevtime=0;

initialized=0;
received_n1=0;
received_n2=0;
received_n3=0;
received_n4=0;
received_n1_temp=0;
received_n2_temp=0;
received_n3_temp=0;
received_n4_temp=0;
output_file="output.file";

printf("# Sent packets  Received 1   Received 2  Received 3  Received 4\n") > output_file; 
}

{

action = $1;

time = $2;

node_id = $3;

layer = $4;

flags = $5;

seqno = $6;

type = $7;

size = $8;

a = $9;

b = $10;

c = $11;

d = $12;

energy = $14;



if(node_id =="_0_" && action == "s" && layer == "AGT" && type == "cbr" && size >= 10) {


count++;
count_temp++;
totalsize+=size;
if(initialized==0)
{
starttime=time;
prevtime=time;
initialized=1;
}
endtime=time;
}


else if(node_id =="_0_" && action == "s" && layer == "MAC" && type == "cbr" && size >= 10)
{

count_mac++;
totalsize_mac+=size;

}
else if(action=="r" && type=="cbr" && size>=10 && layer=="AGT")

{
if(node_id=="_1_")
{
received_n1++;
received_n1_temp++;
}
else if(node_id=="_2_")
{
received_n2++;
received_n2_temp++;
}
else if(node_id=="_3_")
{
received_n3++;
received_n3_temp++;
}
else if(node_id=="_4_")
{
received_n4++;
received_n4_temp++;
}


}


#summarize every 1 s
if(time-prevtime>=1.0)
{
printf("%f %d %d %d %d %d\n", time, count_temp, received_n1_temp,received_n2_temp, received_n3_temp, received_n4_temp) > output_file;

prevtime=time;
count_temp=0;
received_n1_temp=0;
received_n2_temp=0;
received_n3_temp=0;
received_n4_temp=0;
}




}





END {
printf("total packets sent %d, total size %d, start time %f, end time %f, duration %f, bitrate  %f Kbs\n",count,totalsize, starttime, endtime, endtime-starttime, 8*totalsize/(endtime-starttime)/1000.0) ;

printf("packets delivered to the mac layer: %d, size %d, bitrate %d Kbs\n",count_mac,totalsize_mac, 8*totalsize_mac/(endtime-starttime)/1000.0) ;
printf("packets received by the 4 nodes: %d %d %d %d\n", received_n1, received_n2, received_n3, received_n4) ;
}
