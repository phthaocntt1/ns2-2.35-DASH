#include <assert.h>
#include <stdio.h>
#include <iostream>
#include <list>
#include <math.h>
#include <queue>
#include "media.h"

using namespace std;

VideoDescription::VideoDescription()
{
    VideoDescription(-1,NULL,-1,-1,NULL,0);
}

VideoDescription::VideoDescription(int program_id, char* name, int total_duration, int segment_duration, int *bw_queue,int total_bw)
{  
    _program_id=program_id;
    
    if(name)
    strcpy(_name,name);
    else
    memset(_name,0,sizeof(_name));
    _total_duration=total_duration;
    _segment_duration=segment_duration;
    
    if(!bw_queue)
        return; 
    
    
    int i=0,quality_id=0;
    struct _BWItem bw_item;
    for(i=0;i<total_bw;i++)
    {
         bw_item.bandwidth=bw_queue[i];
         bw_item.quality_id=quality_id++;
         _bwqueue.push_back(bw_item);
    }
    
    
}

VideoDescription::~VideoDescription()
{
    while(!_bwqueue.empty())
        _bwqueue.pop_back();
}

DashMediaChunk::DashMediaChunk(VideoDescription& vd, int bandwidth, int segment_id)
:AppData(MEDIA_DATA)
{
    _dtype=PT_TMEDIA_DATA;
    if(vd._program_id<0) /*resource non-existent*/
    {
    _bandwidth=0;
    _segment_id=-1;
    
    }
    else
    {  
        /*check if the requested bandwidth version exists*/
        int initbandwidth=0;
        vector<_BWItem>::iterator it;
        for(it=vd._bwqueue.begin();it!=vd._bwqueue.end();it++)
        {
            _BWItem bwitem=(_BWItem)(*it);
            if(bwitem.bandwidth==bandwidth)
            {
                initbandwidth=bandwidth;
                break;
            }
        }
        
        _bandwidth=initbandwidth;
        
        /*check if the requested segment chunk exists*/
        int total_chunk_number=vd._segment_duration<=0?-1:vd._total_duration/vd._segment_duration;
        
        if(segment_id>=0 && segment_id<total_chunk_number)
            _segment_id=segment_id;
        else
            _segment_id=-1;
    }
    
    _media_packet_size=_bandwidth*vd._segment_duration/8;
    
}


DashClientPlay::DashClientPlay(int addr,char *media):
 _client_addr(addr),
_streaming_started(false),
 _max_bandwidth_version(0),
_current_requested_segment_id(-1),
_current_received_segment_id(-1),
_current_received_ts_id(-1)
{
    strcpy(_media_name,media);
    
   _current_transmitted_segment_id=-1;
   _current_transmitted_ts_id=-1;
   _transmitted_segment_size=0;
   _current_segment_total_size=0;
   _seg_trans_start_time=0.0;
   _seg_received_ratio=0.0;
   
   _request_trend=RTREND_STABLE;
   
   char filename[200];
   sprintf(filename,"trend_file_%d.txt",_client_addr);
   
   _trend_file=fopen(filename,"w");
   
   if(_trend_file==NULL)
   {
       fprintf(stderr,"Error, cannot create file %s\n",filename);
       exit(0);
   }
}

DashClientPlay::~DashClientPlay()
{
    while(!_request_bandwidth_history.empty())
        _request_bandwidth_history.pop_back();
    
    while(!_bwqueue.empty())
        _bwqueue.pop_back();
    
    while(!_original_request_bandwidth_history.empty())
        _original_request_bandwidth_history.pop_back();
    
    while(!_estimated_max_bw.empty())
        _estimated_max_bw.pop_back();
    
    if(_trend_file)
    {
        fclose(_trend_file);
        _trend_file=NULL;
    }
        
}

int DashClientPlay::find_next_bandwidth(int bandwidth, int direction)
{
    assert(bandwidth>0);
    assert(direction==1 && direction==0);
    
    std::vector<_BWItem>::iterator it;
    int i;
    for(i=0;i<_bwqueue.size();i++)
    {
        if(_bwqueue.at(i).bandwidth==bandwidth)
            break;
            
    }
    
    if(i==_bwqueue.size())
    {
        fprintf(stderr,"bandwidth %d is not in the bandwidth queue!\n",bandwidth);
        exit(0);
    }
    
    if(direction)
    {
        i++;
        if(i>=_bwqueue.size())
            i=_bwqueue.size()-1;
    }
    else
    {
        i--;
        if(i<0)
            i=0;
    }
    
    return _bwqueue.at(i).bandwidth;
    
    
}

int DashClientPlay::find_lower_bandwidth(int bandwidth)
{
    assert(bandwidth>0);
    
    
    
    int i;
    for(i=_bwqueue.size()-1;i>=0;i--)
    {
        if(_bwqueue.at(i).bandwidth<=bandwidth)
            
            return _bwqueue.at(i).bandwidth;
            
    }
    
    
    
    
    
    return _bwqueue.at(0).bandwidth;
}

void DashClientPlay::update_request_trend()
{
    
    int record_size=_original_request_bandwidth_history.size();
    if(record_size<=1)
        return;
    
    int most_recent=_original_request_bandwidth_history.at(record_size-1).bandwidth;
    int second_most_recent=_original_request_bandwidth_history.at(record_size-2).bandwidth;
    if(most_recent==second_most_recent)
        _request_trend=RTREND_STABLE;
    else if(most_recent>second_most_recent)
        _request_trend=RTREND_UP;
    else
        _request_trend=RTREND_DOWN;
}


int DashClientPlay::find_closest_bandwidth(int bandwidth)
{
    assert(bandwidth>0);
    
    int candidate_bandwidth=-1;
    
    int i;
    for(i=_bwqueue.size()-1;i>=0;i--)
    {
        if(_bwqueue.at(i).bandwidth<=bandwidth)
        {
            candidate_bandwidth=_bwqueue.at(i).bandwidth;
            break;
        }
    }
    
    //no bandwidth is smaller,return the smallest available
    if(candidate_bandwidth==-1)
    {
        return _bwqueue.at(0).bandwidth;
    }
    
    int higher_bandwidth=_bwqueue.at(i+1).bandwidth;
    
    if(higher_bandwidth-bandwidth>bandwidth-candidate_bandwidth)
        return candidate_bandwidth;
    else
        return higher_bandwidth;
    
    
    
    
}

double DashClientPlay::check_request_stability()
{
    
    if(_request_bandwidth_history.size()<=2)
        return 0.0;
    
    int check_record_number=60/_segment_duration;
    
    if(60%_segment_duration)
        check_record_number++;
    
    std::vector<BWReqHistory>::reverse_iterator rit;
    
    
   
    
    int count=0;
    
    int bandwidth_sum=0;
    int bandwidth_ssum=0;
    
    int vari_count=0;
    
    int prev_bandwidth=-1;
    
   
    for(rit=_request_bandwidth_history.rbegin();rit!=_request_bandwidth_history.rend()-1;rit++)
    {
        
        
        
        int local_bw=(*rit).bandwidth/1000;
        bandwidth_sum+=local_bw;
        bandwidth_ssum+=local_bw*local_bw;
        count++;
        
        if(prev_bandwidth>0)
        {
            if(local_bw!=prev_bandwidth)
                vari_count++;
        }
        
      
       // fprintf(_trend_file,"%d ",local_bw);
        
        
        prev_bandwidth=local_bw;
        if(count>=check_record_number)
            break;
    }
    
   
    //fprintf(_trend_file,"\n");
    
    
    int diff=bandwidth_ssum-bandwidth_sum*bandwidth_sum/count;
  //  fprintf(stderr,"ssum %d  bandwidth sum %d diff is %d\n",bandwidth_ssum,bandwidth_sum,diff);
    double std=sqrt(diff/count);
    
    int mean=bandwidth_sum/count;
    
    double coeff=std/mean;
    
   // fprintf(stderr,"std is %f, mean is %d, coeff is %f\n",std,mean,coeff);
    
    return vari_count*coeff;
}



double DashClientPlay::check_original_request_stability()
{
    
    if(_original_request_bandwidth_history.size()<=2)
        return 0.0;
    
    int check_record_number=60/_segment_duration;
    
    if(60%_segment_duration)
        check_record_number++;
    
    std::vector<BWReqHistory>::reverse_iterator rit;
    
    
   
    
    int count=0;
    
    int bandwidth_sum=0;
    int bandwidth_ssum=0;
    
    int vari_count=0;
    
    int prev_bandwidth=-1;
    
   
    for(rit=_original_request_bandwidth_history.rbegin();rit!=_original_request_bandwidth_history.rend()-1;rit++)
    {
        
        
        
        int local_bw=(*rit).bandwidth/1000;
        bandwidth_sum+=local_bw;
        bandwidth_ssum+=local_bw*local_bw;
        count++;
        
        if(prev_bandwidth>0)
        {
            if(local_bw!=prev_bandwidth)
                vari_count++;
        }
        
        
       
        
        
        prev_bandwidth=local_bw;
        if(count>=check_record_number)
            break;
    }
    
   
    
    
    int diff=bandwidth_ssum-bandwidth_sum*bandwidth_sum/count;
  //  fprintf(stderr,"ssum %d  bandwidth sum %d diff is %d\n",bandwidth_ssum,bandwidth_sum,diff);
    double std=sqrt(diff/count);
    
    int mean=bandwidth_sum/count;
    
    double coeff=std/mean;
    
   // fprintf(stderr,"std is %f, mean is %d, coeff is %f\n",std,mean,coeff);
    
    return vari_count*coeff;
}


double DashClientPlay::check_util(int intended_bw)
{
    
    if(_max_bw<=0)
    {
        fprintf(stderr,"Error, max bw is not positive!\n");
        exit(-1);
    }
    double util_index=(double)intended_bw/(double)_max_bw;
    int requested_bw=_original_request_bandwidth_history.back().bandwidth;
    double satis_index=(double)intended_bw/(double)requested_bw;
    
    //check request stability
    BWReqHistory bw_temp;
    bw_temp.bandwidth=intended_bw;
    bw_temp.time=NOW;
    bw_temp.segment_id=-1;
    
    _request_bandwidth_history.push_back(bw_temp);
    
    double stability_index=check_request_stability();
    
    //remove the record
    _request_bandwidth_history.pop_back();
    
    double alpha=requested_bw/_max_bw;
    
   // return max(util_index,satis_index)/exp(stability_index);
    return exp(util_index)*satis_index/exp(alpha*stability_index);
}


int DashClientPlay::min3(int a, int b, int c)
{
    int min=a<b?a:b;
    return min<c?min:c;
}

int DashClientPlay::max3(int a, int b, int c)
{
    int max=a>b?a:b;
    return max>c?max:c;
}

int DashClientPlay::get_estimated_bw_dev()
{
     if(_estimated_max_bw.size()<=2)
        return 0.0;
    
    int check_record_number=60/_segment_duration;
    
    if(60%_segment_duration)
        check_record_number++;
    
    std::vector<int>::reverse_iterator rit;
    
    
     int count=0;
    
    long long bandwidth_sum=0;
    long long bandwidth_ssum=0;
    
   
    
    
   // fprintf(stderr,"bandwidth: ");
   
    for(rit=_estimated_max_bw.rbegin();rit!=_estimated_max_bw.rend()-1;rit++)
    {
        
        
      
        int local_bw=(*rit)/1000;
        bandwidth_sum+=local_bw;
        bandwidth_ssum+=local_bw*local_bw;
        count++;
       // fprintf(stderr," %d ",local_bw);
        
        if(count>=check_record_number)
            break;
    }
    
   
    
    
    int diff=bandwidth_ssum-bandwidth_sum*bandwidth_sum/count;
  //  fprintf(stderr,"ssum %d  bandwidth sum %d diff is %d\n",bandwidth_ssum,bandwidth_sum,diff);
    int std=sqrt(diff/count);
    
    return std*1000;
    
}

int DashClientPlay::check_util_range(int lower_bw, int higher_bw)
{
    
    
    
    if(lower_bw==higher_bw)
        return lower_bw;
    
    
    int candidate_bw=lower_bw;
    double max_util=0.0;
    
    int current_bw;
    
    int low_index=find_bw_index(lower_bw);
    int high_index=find_bw_index(higher_bw);
    
    assert(low_index>0 && high_index>0 && low_index<high_index);
    
    int i;
    for(i=low_index;i<=high_index;i++)
    {
        
        current_bw=_bwqueue.at(i).bandwidth;
        double current_util=check_util(current_bw);
    
    if(current_util>max_util)
    {
        
        max_util=current_util;
        candidate_bw=current_bw;
    }
       
       
    }
    
    
    
    
    return candidate_bw;
    
    
}


int DashClientPlay::find_bw_index(int bw)
{
    assert(bw>0);
    
    int i;
    for(i=0;i<_bwqueue.size();i++)
    {
        if(_bwqueue.at(i).bandwidth==bw)
       
            return i;
            
        
    }
    
    return -1;
    
}