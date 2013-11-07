#some errors in the final output trace file
#as all agent packets are marked as ack
#so we will depend on packet size to decide if it is a data packet
BEGIN {
count = 0;
count_temp=0;
count_mac_temp=0;
count_mac=0;
totalsize=0;
totalsize_mac=0;

starttime=0;
endtime=0;
prevtime=0;

initialized=0;

sent_app_n1=0;
sent_app_n2=0;
sent_app_n3=0;
sent_app_n4=0;
sent_app_n5=0;
sent_app_n6=0;

sent_mac_n1=0;
sent_mac_n2=0;
sent_mac_n3=0;
sent_mac_n4=0;
sent_mac_n5=0;
sent_mac_n6=0;

sent_mac_n1_temp=0;
sent_mac_n2_temp=0;
sent_mac_n3_temp=0;
sent_mac_n4_temp=0;
sent_mac_n5_temp=0;
sent_mac_n6_temp=0;

retran_mac_n1=0;
retran_mac_n2=0;
retran_mac_n3=0;
retran_mac_n4=0;
retran_mac_n5=0;
retran_mac_n6=0;


retran_mac_n1_temp=0;
retran_mac_n2_temp=0;
retran_mac_n3_temp=0;
retran_mac_n4_temp=0;
retran_mac_n5_temp=0;
retran_mac_n6_temp=0;


dropped_macret_n1=0;
dropped_macret_n2=0;
dropped_macret_n3=0;
dropped_macret_n4=0;
dropped_macret_n5=0;
dropped_macret_n6=0;

dropped_macret_n1_temp=0;
dropped_macret_n2_temp=0;
dropped_macret_n3_temp=0;
dropped_macret_n4_temp=0;
dropped_macret_n5_temp=0;
dropped_macret_n6_temp=0;

received_n1=0;
received_n2=0;
received_n3=0;
received_n4=0;
received_n5=0;
received_n6=0;
received_other=0;
received_n1_temp=0;
received_n2_temp=0;
received_n3_temp=0;
received_n4_temp=0;
received_n5_temp=0;
received_n6_temp=0;

received_n1_size=0;
received_n2_size=0;
received_n3_size=0;
received_n4_size=0;
received_n5_size=0;
received_n6_size=0;
received_other_size=0;





current_high_seq=0;
high_seq_n1=-1;
high_seq_n2=-1;
high_seq_n3=-1;
high_seq_n4=-1;
high_seq_n5=-1;
high_seq_n6=-1;


receive_file="receive.stat";
retrans_file="retrans.stat";
dropped_file="dropped.stat";

printf("# Sent packets Send_MAC Received_1   Received_2  Received_3  Received_4 Received_5 Received_6\n") > receive_file; 
}

{

action = $1;

time = $2;

node_id = $3;

left_parenth=$4;

xcord=$5;

ycord=$6;

right_parenth=$7;

layer = $8;

flags = $9;

seqno = $10;

type = $11;

size = $12;

a = $13;

b = $14;

c = $15;

d = $16;

source_str = $17;

destination_str= $18;

ttl= $19;

next_hop= $20;

tcp_seq_str= $21;

tcp_ack_str=$22;


if(NF>=20)
{

#get tcp session packet seqence no
strlength=length(tcp_seq_str);
session_seq=1*substr(tcp_seq_str,2,strlength-1);

#get destination address and port no 
split(destination_str, strarray,":");
dest_addr=1*strarray[1];
dest_port=1*strarray[2];


}



if(node_id ==0 && action == "s" && layer == "AGT"  && size >= 10) {


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

if(dest_addr==1)
sent_app_n1++;
else if(dest_addr==2)
sent_app_n2++;
else if(dest_addr==3)
sent_app_n3++;
else if(dest_addr==4)
sent_app_n4++;
else if(dest_addr==5)
sent_app_n5++;
else if(dest_addr==6)
sent_app_n6++;
}


else if(node_id ==0 && action == "s" && layer == "MAC"  && size >= 10)
{



count_mac++;
count_mac_temp++;
totalsize_mac+=size;




if(dest_addr==1)
{

if(session_seq>high_seq_n1)
{
sent_mac_n1++;
sent_mac_n1_temp++;
}
else
{
retrans_mac_n1++;
retrans_mac_n1_temp++;
}

high_seq_n1=session_seq;
}
else if(dest_addr==2)
{

if(session_seq>high_seq_n2)
{
sent_mac_n2++;
sent_mac_n2_temp++;
}
else
{
retrans_mac_n2++;
retrans_mac_n2_temp++;
}

high_seq_n2=session_seq;
}
else if(dest_addr==3)
{
if(session_seq>high_seq_n3)
{
sent_mac_n3++;
sent_mac_n3_temp++;
}
else
{
retrans_mac_n3++;
retrans_mac_n3_temp++;
}

high_seq_n3=session_seq;
}
else if(dest_addr==4)
{

if(session_seq>high_seq_n4)
{
sent_mac_n4++;
sent_mac_n4_temp++;
}
else
{
retrans_mac_n4++;
retrans_mac_n4_temp++;
}

high_seq_n4=session_seq;
}

else if(dest_addr==5)
{

if(session_seq>high_seq_n5)
{
sent_mac_n5++;
sent_mac_n5_temp++;
}
else
{
retrans_mac_n5++;
retrans_mac_n5_temp++;
}

high_seq_n5=session_seq;
}

else if(dest_addr==6)
{

if(session_seq>high_seq_n6)
{
sent_mac_n6++;
sent_mac_n6_temp++;
}
else
{
retrans_mac_n6++;
retrans_mac_n6_temp++;
}

high_seq_n6=session_seq;
}

}
else if(action=="r" && size>=10 && layer=="AGT")

{
if(node_id==1)
{
received_n1++;
received_n1_size+=size;
received_n1_temp++;
}
else if(node_id==2)
{
received_n2++;
received_n2_size+=size;
received_n2_temp++;
}
else if(node_id==3)
{
received_n3++;
received_n3_size+=size;
received_n3_temp++;
}
else if(node_id==4)
{
received_n4++;
received_n4_size+=size;
received_n4_temp++;
}
else if(node_id==5)
{
received_n5++;
received_n5_size+=size;
received_n5_temp++;
}
else if(node_id==6)
{
received_n6++;
received_n6_size+=size;
received_n6_temp++;
}
else
{
received_other++;
received_other_size+=size;
}



}

else if(node_id==0 && action == "D" && layer=="MAC" && flags=="RET")
{

if(dest_addr==1)
{
dropped_macret_n1++;
dropped_macret_n1_temp++;
}
else if(dest_addr==2)
{
dropped_macret_n2++;
dropped_macret_n2_temp++;
}
else if(dest_addr==3)
{
dropped_macret_n3++;
dropped_macret_n3_temp++;
}
else if(dest_addr==4)
{
dropped_macret_n4++;
dropped_macret_n4_temp++;
}
else if(dest_addr==5)
{
dropped_macret_n5++;
dropped_macret_n5_temp++;
}
else if(dest_addr==5)
{
dropped_macret_n5++;
dropped_macret_n5_temp++;
}
}


#summarize every 1 s
if(time-prevtime>=1.0)
{
printf("%f %d %d %d %d %d %d %d %d\n", time, count_mac_temp, received_n1_temp,received_n2_temp, received_n3_temp, received_n4_temp,received_n5_temp,received_n6_temp,
received_n1_temp+received_n2_temp+received_n3_temp+received_n4_temp+received_n5_temp+received_n6_temp) > receive_file;
printf("%f %d %d %d %d %d %d %d\n", time, retrans_mac_n1_temp+retrans_mac_n2_temp+retrans_mac_n3_temp+retrans_mac_n4_temp+retrans_mac_n5_temp+retrans_mac_n6_temp,
retrans_mac_n1_temp, retrans_mac_n2_temp,retrans_mac_n3_temp,retrans_mac_n4_temp,retrans_mac_n5_temp,retrans_mac_n6_temp) > retrans_file;
printf("%f %d %d %d %d %d %d %d\n", time, dropped_macret_n1_temp+dropped_macret_n2_temp+dropped_macret_n3_temp+dropped_macret_n4_temp+dropped_macret_n5_temp+dropped_macret_n6_temp,
dropped_macret_n1_temp,dropped_macret_n2_temp,dropped_macret_n3_temp,dropped_macret_n4_temp,dropped_macret_n5_temp,dropped_macret_n6_temp) > dropped_file;


prevtime=time;
count_temp=0;
count_mac_temp=0;
received_n1_temp=0;
received_n2_temp=0;
received_n3_temp=0;
received_n4_temp=0;
received_n5_temp=0;
received_n6_temp=0;

dropped_macret_n1_temp=0;
dropped_macret_n2_temp=0;
dropped_macret_n3_temp=0;
dropped_macret_n4_temp=0;
dropped_macret_n5_temp=0;
dropped_macret_n6_temp=0;

retrans_mac_n1_temp=0;
retrans_mac_n2_temp=0;
retrans_mac_n3_temp=0;
retrans_mac_n4_temp=0;
retrans_mac_n5_temp=0;
retrans_mac_n6_temp=0;

sent_mac_n1_temp=0;
sent_mac_n2_temp=0;
sent_mac_n3_temp=0;
sent_mac_n4_temp=0;
sent_mac_n5_temp=0;
sent_mac_n6_temp=0;

}




}





END {
printf("start time %f, end time %f\n",startime,endtime);
printf("total packets sent from app level %d, total size %d, start time %f, end time %f, duration %f, bitrate  %f Kbs\n",count,totalsize, starttime, endtime, endtime-starttime, 8*totalsize/(endtime-starttime)/1000.0) ;
printf("packets sent from application level to nodes: %d %d %d %d %d %d\n",sent_app_n1, sent_app_n2, sent_app_n3, sent_app_n4,sent_app_n5,sent_app_n6);
printf("packets sent from mac level to nodes:         %d %d %d %d %d %d\n",sent_mac_n1, sent_mac_n2, sent_mac_n3, sent_mac_n4,sent_mac_n5,sent_mac_n6);
printf("packets retransmit in mac level to nodes:     %d %d %d %d %d %d\n",retrans_mac_n1,retrans_mac_n2,retrans_mac_n3,retrans_mac_n4,retrans_mac_n5,retrans_mac_n6);
printf("packets dropped in mac due to retrans limits: %d %d %d %d %d %d\n",dropped_macret_n1, dropped_macret_n2,dropped_macret_n3,dropped_macret_n4,dropped_macret_n5,dropped_macret_n6);
printf("packets drop ratio in LL queue :              %f, %f, %f, %f, %f, %f\n",
sent_app_n1>0? 1-sent_mac_n1/sent_app_n1: 0,sent_app_n2>0?1-sent_mac_n2/sent_app_n2:0,
sent_app_n3>0? 1-sent_mac_n3/sent_app_n3: 0,sent_app_n4>0? 1-sent_mac_n4/sent_app_n4:0,
sent_app_n5>0? 1-sent_mac_n5/sent_app_n5: 0,sent_app_n6>0? 1-sent_mac_n6/sent_app_n6:0);

printf("packets delivered to the mac layer: %d, size %d, bitrate %f Kbs\n",count_mac,totalsize_mac, 8*totalsize_mac/(endtime-starttime)/1000.0) ;
printf("packets received by the 6 nodes: %d %d %d %d %d %d\n", received_n1, received_n2, received_n3, received_n4, received_n5,received_n6) ;
printf("application bitrates received by the 6 nodes: %f Kbs, %f Kbs, %f Kbs, %f Kbs, %f Kbs, %f Kbs\n",
8*received_n1_size/(endtime-starttime)/1000.0,
8*received_n2_size/(endtime-starttime)/1000.0,
8*received_n3_size/(endtime-starttime)/1000.0,
8*received_n4_size/(endtime-starttime)/1000.0,
8*received_n5_size/(endtime-starttime)/1000.0,
8*received_n6_size/(endtime-starttime)/1000.0);
printf("packet loss ratio experienced by nodes: %f, %f, %f, %f, %f, %f\n",
sent_app_n1>0? 1-received_n1/sent_app_n1:0,
sent_app_n2>0? 1-received_n2/sent_app_n2:0,
sent_app_n3>0? 1-received_n3/sent_app_n3:0,
sent_app_n4>0? 1-received_n4/sent_app_n4:0,
sent_app_n5>0? 1-received_n5/sent_app_n5:0,
sent_app_n6>0? 1-received_n6/sent_app_n6:0);
printf("total application bitrates from receivers' side: %f Kbs\n", 8*(received_n1_size+received_n2_size+received_n3_size+received_n4_size+received_n5_size+received_n6_size+received_other_size)/(endtime-starttime)/1000.0);
}
