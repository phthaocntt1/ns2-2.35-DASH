BEGIN {

starttime=0;
endtime=0;

initialized=0;

prevtime=0;

#dropped total packets
count = 0;
totalsize=0;

#dropped packets by nodes
dropped_n1=0;
dropped_n2=0;
dropped_n3=0;
dropped_n4=0;
dropped_others=0;

#dropped packets by reasons and nodes
dropped_n1_arp=0;
dropped_n1_ifq=0;
dropped_n1_macret=0;
dropped_n1_maccol=0;
dropped_n1_macbsy=0;
dropped_n1_interf=0;
dropped_n1_noise=0;
dropped_n1_other=0;

dropped_n2_arp=0;
dropped_n2_ifq=0;
dropped_n2_macret=0;
dropped_n2_maccol=0;
dropped_n2_macbsy=0;
dropped_n2_interf=0;
dropped_n2_noise=0;
dropped_n2_other=0;


dropped_n3_arp=0;
dropped_n3_ifq=0;
dropped_n3_macret=0;
dropped_n3_maccol=0;
dropped_n3_macbsy=0;
dropped_n3_interf=0;
dropped_n3_noise=0;
dropped_n3_other=0;


dropped_n4_arp=0;
dropped_n4_ifq=0;
dropped_n4_macret=0;
dropped_n4_maccol=0;
dropped_n4_macbsy=0;
dropped_n4_interf=0;
dropped_n4_noise=0;
dropped_n4_other=0;

macret_n1=0;
maccol_n1=0;
macbsy_n1=0;
macint_n1=0;
macnos_n1=0;

macret_n2=0;
maccol_n2=0;
macbsy_n2=0;
macint_n2=0;
macnos_n2=0;

macret_n3=0;
maccol_n3=0;
macbsy_n3=0;
macint_n3=0;
macnos_n3=0;

macret_n4=0;
maccol_n4=0;
macbsy_n4=0;
macint_n4=0;
macnos_n4=0;


phycst_n1=0;
phycst_n2=0;
phycst_n3=0;
phycst_n4=0;

dropped_n1_ifq_temp=0;
dropped_n2_ifq_temp=0;
dropped_n3_ifq_temp=0;
dropped_n4_ifq_temp=0;

dropped_queue_file="dropped_queue.stat"
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

if(node_id=="_0_" && action == "D") {


count++;
totalsize+=size;
if(initialized==0)
{
starttime=time;
initialized=1;
}
endtime=time;


#if destination is node 1
if(dest_addr==1)
{
dropped_n1++;
if(layer=="IFQ" && flags=="ARP")
dropped_n1_arp++;
else if(layer=="IFQ")
{
dropped_n1_ifq++;
dropped_n1_ifq_temp++;
}
else if(layer=="MAC" && flags=="RET")
dropped_n1_macret++;
else if(layer=="MAC" && flags=="BSY")
dropped_n1_macbsy++;
else if(layer=="MAC" && flags=="COL")
dropped_n1_maccol++;
else if(layer=="MAC" && flags=="INT")
dropped_n1_interf++;
else if(layer=="MAC" && flags=="NOS")
dropped_n1_noise++;
else
dropped_n1_other++;

}

#if destination is node 2
else if(dest_addr==2)
{
dropped_n2++;
if(layer=="IFQ" && flags=="ARP")
dropped_n2_arp++;
else if(layer=="IFQ")
{
dropped_n2_ifq++;
dropped_n2_ifq_temp++;
}
else if(layer=="MAC" && flags=="RET")
dropped_n2_macret++;
else if(layer=="MAC" && flags=="BSY")
dropped_n2_macbsy++;
else if(layer=="MAC" && flags=="COL")
dropped_n2_maccol++;
else if(layer=="MAC" && flags=="INT")
dropped_n2_interf++;
else if(layer=="MAC" && flags=="NOS")
dropped_n2_noise++;
else
dropped_n2_other++;

}

#if destination is node 3
else if(dest_addr==3)
{
dropped_n3++;
if(layer=="IFQ" && flags=="ARP")
dropped_n3_arp++;
else if(layer=="IFQ")
{
dropped_n3_ifq++;
dropped_n3_ifq_temp++;
}
else if(layer=="MAC" && flags=="RET")
dropped_n3_macret++;
else if(layer=="MAC" && flags=="BSY")
dropped_n3_macbsy++;
else if(layer=="MAC" && flags=="COL")
dropped_n3_maccol++;
else if(layer=="MAC" && flags=="INT")
dropped_n3_interf++;
else if(layer=="MAC" && flags=="NOS")
dropped_n3_noise++;
else
dropped_n3_other++;

}

#if destination is node 4
else if(dest_addr==4)
{
dropped_n4++;
if(layer=="IFQ" && flags=="ARP")
dropped_n4_arp++;
else if(layer=="IFQ")
{
dropped_n4_ifq++;
dropped_n4_ifq_temp++;
}
else if(layer=="MAC" && flags=="RET")
dropped_n4_macret++;
else if(layer=="MAC" && flags=="BSY")
dropped_n4_macbsy++;
else if(layer=="MAC" && flags=="COL")
dropped_n4_maccol++;
else if(layer=="MAC" && flags=="INT")
dropped_n4_interf++;
else if(layer=="MAC" && flags=="NOS")
dropped_n4_noise++;
else
dropped_n4_other++;

}

else
dropped_others++;




#summarize every 1 s
if(time-prevtime>=1.0)
{
printf("%f %d %d %d %d %d\n", time, dropped_n1_ifq_temp, dropped_n2_ifq_temp,dropped_n3_ifq_temp,dropped_n4_ifq_temp,
dropped_n1_ifq_temp+dropped_n2_ifq_temp+dropped_n3_ifq_temp+dropped_n4_ifq_temp) > dropped_queue_file;

prevtime=time;
dropped_n1_ifq_temp=0;
dropped_n2_ifq_temp=0;
dropped_n3_ifq_temp=0;
dropped_n4_ifq_temp=0;
}
}
else if(action == "D")
{

if(node_id=="_1_")
{
if(layer=="PHY" && dest_addr==1)
phycst_n1++;
else if(layer=="MAC")
{
if(flags=="RET") macret_n1++;
else if(flags=="BSY") macbsy_n1++;
else if(flags=="COL") maccol_n1++;
else if(flags=="INT") macint_n1++;
else if(flags=="NOS") macnos_n1++;
}
}
else if (node_id=="_2_")
{
if(layer=="PHY" && dest_addr==2)
phycst_n2++;
else if(layer=="MAC")
{
if(flags=="RET") macret_n2++;
else if(flags=="BSY") macbsy_n2++;
else if(flags=="COL") maccol_n2++;
else if(flags=="INT") macint_n2++;
else if(flags=="NOS") macnos_n2++;
}
}

else if (node_id=="_3_")
{
if(layer=="PHY" && dest_addr==3)
phycst_n3++;
else if(layer=="MAC")
{
if(flags=="RET") macret_n3++;
else if(flags=="BSY") macbsy_n3++;
else if(flags=="COL") maccol_n3++;
else if(flags=="INT") macint_n3++;
else if(flags=="NOS") macnos_n3++;
}
}
else if (node_id=="_4_")
{
if(layer=="PHY" && dest_addr==4)
phycst_n4++;
else if(layer=="MAC")
{
if(flags=="RET") macret_n4++;
else if(flags=="BSY") macbsy_n4++;
else if(flags=="COL") maccol_n4++;
else if(flags=="INT") macint_n4++;
else if(flags=="NOS") macnos_n4++;
}
}
 
}

}





END {
if(count==0)
printf("No packets have been dropped!\n");
else
printf("total packets dropped %d, total size %d, start time %f, end time %f, duration %f, bitrate  %f Kbs\n",count,totalsize, starttime, endtime, endtime-starttime, 8*totalsize/(endtime-starttime)/1000.0);

printf("For node 1, AP dropped packets %d, arp %d, ifq %d, mac ret %d, mac col %d, mac busy %d, mac interf %d, mac noise %d, others %d\n", dropped_n1, dropped_n1_arp,
dropped_n1_ifq,dropped_n1_macret,dropped_n1_macbsy,dropped_n1_maccol,dropped_n1_interf, dropped_n1_noise,dropped_n1_other);

printf("For node 2, AP dropped packets %d, arp %d, ifq %d, mac ret %d, mac col %d, mac busy %d, mac interf %d, mac noise %d, others %d\n", dropped_n2, dropped_n2_arp,
dropped_n2_ifq,dropped_n2_macret,dropped_n2_macbsy,dropped_n2_maccol,dropped_n2_interf, dropped_n2_noise,dropped_n2_other);

printf("For node 3, AP dropped packets %d, arp %d, ifq %d, mac ret %d, mac col %d, mac busy %d, mac interf %d, mac noise %d, others %d\n", dropped_n3, dropped_n3_arp,
dropped_n3_ifq,dropped_n3_macret,dropped_n3_macbsy,dropped_n3_maccol,dropped_n3_interf, dropped_n3_noise, dropped_n3_other);

printf("For node 4, AP dropped packets %d, arp %d, ifq %d, mac ret %d, mac col %d, mac busy %d, mac interf %d, mac noise %d, others %d\n", dropped_n4, dropped_n4_arp,
dropped_n4_ifq,dropped_n4_macret,dropped_n4_macbsy,dropped_n4_maccol,dropped_n4_interf, dropped_n4_noise, dropped_n4_other);

if(dropped_others!=0)
printf("Dropped packets for other nodes %d\n",dropped_others);

printf("===========================================================\n");
printf("From node 1, mac ret %d, mac col %d, mac busy %d, mac interf %d, mac noise %d, phy cst %d\n",macret_n1,macbsy_n1,maccol_n1,macint_n1,macnos_n1,phycst_n1);
printf("From node 2, mac ret %d, mac col %d, mac busy %d, mac interf %d, mac noise %d, phy cst %d\n",macret_n2,macbsy_n2,maccol_n2,macint_n2,macnos_n2,phycst_n2);
printf("From node 3, mac ret %d, mac col %d, mac busy %d, mac interf %d, mac noise %d, phy cst %d\n",macret_n3,macbsy_n3,maccol_n3,macint_n3,macnos_n3,phycst_n3);
printf("From node 4, mac ret %d, mac col %d, mac busy %d, mac interf %d, mac noise %d, phy cst %d\n",macret_n4,macbsy_n4,maccol_n4,macint_n4,macnos_n4,phycst_n4);

}
