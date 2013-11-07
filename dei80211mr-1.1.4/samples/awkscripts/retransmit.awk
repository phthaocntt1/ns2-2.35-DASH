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


send_seq_n1=0;
send_seq_n2=0;
send_seq_n3=0;
send_seq_n4=0;

high_seq_n1=0;
high_seq_n2=0;
high_seq_n3=0;
high_seq_n4=0;


#for calculation of retransmission times

seq_count_n1=0;
seq_count_n2=0;
seq_count_n3=0;
seq_count_n4=0;


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

if(action == "s" && layer == "MAC" && type == "cbr" && size >= 10) {



endtime=time;

# set the first packet sent time
if(initialized==0)
{
starttime=time;
prevtime=time;
initialized=1;
}






prev_seq_no=seqno;
prev_dest_node=dest_addr;
prev_source_node=node_id;




if(dest_addr==1) 
{

send_count_n1++;

if(session_seq>high_seq_n1)
{
high_seq_n1=session_seq;
seq_count_n1++;

}
}
else if(dest_addr==2) 
{

send_count_n2++;

if(session_seq>high_seq_n2)
{
high_seq_n2=session_seq;
seq_count_n2++;

}
}
else if(dest_addr==3) 
{

send_count_n3++;

if(session_seq>high_seq_n3)
{
high_seq_n3=session_seq;
seq_count_n3++;

}
}
else if(dest_addr==4) 
{

send_count_n4++;

if(session_seq>high_seq_n4)
{
high_seq_n4=session_seq;
seq_count_n4++;

}
}













}

}



END {
printf("MAC layer packet statistics:\n");
printf("For node 1: send %d packets with %d unique sequence no, retrans rate %f\n",send_count_n1,seq_count_n1,seq_count_n1>0?send_count_n1/seq_count_n1:0);

printf("For node 2: send %d packets with %d unique sequence no, retrans rate %f\n",send_count_n2,seq_count_n2,seq_count_n2>0?send_count_n2/seq_count_n2:0);

printf("For node 3: send %d packets with %d unique sequence no, retrans rate %f\n",send_count_n3,seq_count_n3,seq_count_n3>0?send_count_n3/seq_count_n3:0);

printf("For node 4: send %d packets with %d unique sequence no, retrans rate %f\n",send_count_n4,seq_count_n4,seq_count_n4>0?send_count_n4/seq_count_n4:0);







}
