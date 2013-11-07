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

send_seq_n1=0;
send_seq_n2=0;
send_seq_n3=0;
send_seq_n4=0;

high_seq_n1=0;
high_seq_n2=0;
high_seq_n3=0;
high_seq_n4=0;

recv_seq_n1=0;
recv_seq_n2=0;
recv_seq_n3=0;
recv_seq_n4=0;

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

source_str = $14;

destination_str= $15;

ttl= $16;

next_hop= $17;

session_seq_str= $18;


if(NF>=18)
{

#get session packet seqence no
strlength=length(session_seq_str);
session_seq=1*substr(session_seq_str,2,strlength-2);

#get destination address and port no 
split(destination_str, strarray,":");
dest_addr=1*strarray[1];
dest_port=1*strarray[2];


}

if(node_id =="_0_" && action == "s" && layer == "AGT" && type == "cbr" && size >= 10) {


count++;
count_temp++;
totalsize+=size;
endtime=time;

# set the first packet sent time
if(initialized==0)
{
starttime=time;
prevtime=time;
initialized=1;
}

if(dest_addr==1 && session_seq> high_seq_n1) 
{

stime_packet_n1[session_seq]=time;
high_seq_n1=session_seq;


}

else if(dest_addr==2 && session_seq > high_seq_n2) 
{
stime_packet_n2[session_seq]=time;
high_seq_n2=session_seq;

}
else if(dest_addr==3 && session_seq > high_seq_n3) 
{
stime_packet_n3[session_seq]=time;
high_seq_n3=session_seq;

}
else if(dest_addr==4 && session_seq > high_seq_n4)  
{
stime_packet_n4[session_seq]=time;
high_seq_n4=session_seq;

}

}

else if(action=="r" && type=="cbr" && size>=10 && layer=="AGT")

{
if(node_id=="_1_")
{
received_n1++;
received_n1_temp++;
rtime_packet_n1[session_seq]=time;
}
else if(node_id=="_2_")
{
received_n2++;
received_n2_temp++;
rtime_packet_n2[session_seq]=time;
}
else if(node_id=="_3_")
{
received_n3++;
received_n3_temp++;
rtime_packet_n3[session_seq]=time;
}
else if(node_id=="_4_")
{
received_n4++;
received_n4_temp++;
rtime_packet_n4[session_seq]=time;
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

}



END {
printf("total packets sent %d, total size %d, start time %f, end time %f, duration %f, bitrate  %f Kbs\n",count,totalsize, starttime, endtime, endtime-starttime, 8*totalsize/(endtime-starttime)/1000.0) ;

printf("packets delivered to the mac layer: %d, size %d, bitrate %d Kbs\n",count_mac,totalsize_mac, 8*totalsize_mac/(endtime-starttime)/1000.0) ;
printf("packets received by the 4 nodes: %d %d %d %d\n", received_n1, received_n2, received_n3, received_n4) ;

#get the transmission delay for each packet
delay_n1=0;




for( mindex=1; mindex <= 1*high_seq_n1; mindex++ )
{
  if(stime_packet_n1[mindex]!=0 && rtime_packet_n1[mindex]!=0)

  delay_n1+=rtime_packet_n1[mindex]-stime_packet_n1[mindex];

  print mindex " " stime_packet_n1[mindex] " " rtime_packet_n1[mindex] " " rtime_packet_n1[mindex]-stime_packet_n1[mindex]> "n1.out";
  
}


if(received_n1!=0)
printf("N1 average delay %f ms\n", delay_n1/received_n1*10^3);
else
printf("N1 received no packets!\n");

delay_n2=0;
for( mindex=1; mindex<=1*high_seq_n2; mindex++ )
{
  if(stime_packet_n2[mindex]!=0 && rtime_packet_n2[mindex]!=0)
{
  delay_n2+=rtime_packet_n2[mindex]-stime_packet_n2[mindex];
  print mindex " " stime_packet_n2[mindex] " " rtime_packet_n2[mindex] " "  rtime_packet_n2[mindex]-stime_packet_n2[mindex] > "n2.out";
}
}

if(received_n2!=0)
printf("N2 average delay %f ms\n", delay_n2/received_n2*10^3);
else
printf("N2 received no packets!\n");


delay_n3=0;
for( mindex=1; mindex<=1*high_seq_n3; mindex++ )
{
  if(stime_packet_n3[mindex]!=0 && rtime_packet_n3[mindex]!=0)
{
  delay_n3+=rtime_packet_n3[mindex]-stime_packet_n3[mindex];
  print mindex " " stime_packet_n3[mindex] " " rtime_packet_n3[mindex] " " rtime_packet_n3[mindex]-stime_packet_n3[mindex] > "n3.out";
}
}

if(received_n3!=0)
printf("N3 average delay %f ms\n", delay_n3/received_n3*10^3);
else
printf("N3 received no packets!\n");



delay_n4=0;
for( mindex=1; mindex<=1*high_seq_n4; mindex++ )
{
  if(stime_packet_n4[mindex]!=0 && rtime_packet_n4[mindex]!=0)
{
  delay_n4+=rtime_packet_n4[mindex]-stime_packet_n4[mindex];
  print mindex " " stime_packet_n4[mindex] " "  rtime_packet_n4[mindex] " " rtime_packet_n4[mindex]-stime_packet_n4[mindex] > "n4.out" ;
}
}

if(received_n4!=0)
printf("N4 average delay %f ms\n", delay_n4/received_n4*10^3);
else
printf("N4 received no packets!\n");

}
