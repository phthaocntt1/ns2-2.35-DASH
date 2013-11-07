BEGIN {
count = 0;
totalsize=0;

starttime=0;
endtime=0;

initialized=0;


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



if(action == "D" && layer == "MAC" && type=="cbr") {


count++;
totalsize+=size;
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
printf("No packets have been dropped!\n");
else
printf("total packets dropped %d, total size %d, start time %f, end time %f, duration %f, bitrate  %f Kbs\n",count,totalsize, starttime, endtime, endtime-starttime, 8*totalsize/(endtime-starttime)/1000.0);
}
