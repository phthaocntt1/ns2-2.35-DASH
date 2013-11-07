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

e= $13;

source_str = $14;

destination_str= $15;

ttl= $16;

next_hop= $17;

session_seq_str= $18;

printf("NF is %d\n",NF);

printf("%s\n %s\n %s\n %s\n %s\n",a, b,c,d,e);

printf("%s\n %s\n %s\n %s\n %s\n", source_str, destination_str,ttl,next_hop,session_seq_str);

#get session packet seqence no
strlength=length(session_seq_str);
session_seq=substr(session_seq_str,2,strlength-2);

#get destination address and port no 
split(destination_str, strarray,":");
dest_addr=strarray[1];
dest_port=strarray[2];
printf("destination address and port are %d:%d\n",dest_addr,dest_port);

printf("session seq no is %d\n",session_seq);
array[3]=5;
printf("we have a multiarray %d %d %d %d %d\n",array[0],array[1], array[2],array[3],array[4]);
printf("Value of an undeclared array %d\n",arrayxxxx[8]);
}
END {

}
