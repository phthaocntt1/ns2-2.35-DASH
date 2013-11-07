BEGIN {
count4 = 0;
totalsize4=0;
count5 = 0;
totalsize5=0;
starttime4=0;
endtime4=0;
starttime5=0;
endtime5=0;
initialized4=0;
initialized5=0;

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



if(action == "s" && layer == "AGT" && type == "cbr" && size >= 10) {

if(node_id == "_4_")
{
count4++;
totalsize4+=size;
if(initialized4==0)
{
starttime4=time;
initialized4=1;
}
endtime4=time;
}
else if(node_id=="_5_")
{
count5++;
totalsize5+=size;
if(initialized5==0)
{
starttime5=time;
initialized5=1;
}
endtime5=time;
}
}

}



END {
printf("total packets sent %d, total size %d, start time %f, end time %f, duration %f\n",count4,totalsize4, starttime4, endtime4, endtime4-starttime4);
printf("total packets sent %d, total size %d, start time %f, end time %f, duration %f\n",count5,totalsize5, starttime5, endtime5, endtime5-starttime5);
}
