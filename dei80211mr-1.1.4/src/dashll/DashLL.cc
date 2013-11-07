#include <typeinfo>


#include "DashLL.h"
#include "ll.h"
#include "packet.h"
#include "mac-802_11mr.h"
#include "dash/utils.h"
#include "dash/media.h"
#include "dash/dashpriqueue.h"

using namespace std;

static class DashLLClass : public TclClass {
public:
	DashLLClass() : TclClass("DashLL") {}
	TclObject* create(int, const char*const*) {
		return (new DashLL);
	}
} class_ll;



DashLL::DashLL():LL(),
        _default_log_level(DASH_LOG_NOTE),
        _mac80211mr(NULL),
        _mac80211mr_initialized(false),
        _peerstatsdb(NULL),
        //_spolicy(SP_WORST_CASE_WEIGHTED_FAIR)
        _spolicy(SP_FIFO)
{
    _client_bw_table=new ClientBW[10];
    assert(_client_bw_table);
    
    memset(_client_bw_table,0,10*sizeof(ClientBW));
    
    
    
    _max_client_number=0;
    
    
}

DashLL::~DashLL()
{
    while(!_dash_clients.empty())
    {
        delete _dash_clients.front();
        _dash_clients.pop_front();
    }
    
    if(_client_bw_table)
        delete _client_bw_table;
    
   
}



void
DashLL::recv(Packet* p, Handler* h)
{
    /*analyze packet header (and content)*/
    
   // fprintf(stderr,"in link layer with mac %d received a packet\n",mac_->addr());
    /*first check mac layer type*/
    Mac802_11mr *mac80211=dynamic_cast<Mac802_11mr*>(mac_);
    
    if(mac80211==NULL)
    {
        LL::recv(p,h);
        return;
    }
    
    if(!_mac80211mr_initialized)
    {
        _mac80211mr=mac80211;
        _mac80211mr_initialized=true;
        _peerstatsdb=_mac80211mr->getPeerStatsDB();
        
    }
    
  //  fprintf(stderr,"in LL with mac addr %d\n",mac_->addr());
    
    /*decide if this is the AP*/
    if(mac80211->addr()!=mac80211->bss_id())
    {
        LL::recv(p,h);
        return;
    }
    
    
    /*if upstream data, must have mac header
      if downstream data, must have ip header*/
    
    hdr_ip *ch = HDR_IP(p);
    
    hdr_cmn *header=HDR_CMN(p);
    
    if(header->direction()!=hdr_cmn::UP)
    { 
        
        update_client_number(ch->dst().addr_,true);
        update_underlying_bandwidth(ch->dst().addr_);
        update_underlying_queue_weight(ch->dst().addr_);
    }
    
    
    if(p->userdata()==NULL)
    {
        LL::recv(p,h);
        return;
    }
    
    
    

      /*process data*/
      process_data(p->userdata(),ch->src().addr_,ch->dst().addr_);
    
    
    
    
    LL::recv(p,h);
}


void
DashLL::Log(int log_level, const char* fmt, ...)
{
    va_list arg_list;
    va_start(arg_list,fmt);
    
    
    
    DashLog(stdout,log_level,_default_log_level,DASH_COLOR_CYAN,"DASH-LINK  ",fmt,arg_list);
    va_end(arg_list);
    
    /*those fatal situations should not happen*/
    if(log_level==DASH_LOG_FATAL)
        exit(0);
}

void 
DashLL::process_data(AppData* data,int src_addr,int dst_addr)
{
    /*first check if it is a media data*/
    DashMediaChunk *dashmediachunk=dynamic_cast<DashMediaChunk*>(data);
    if(dashmediachunk)
    {
        process_media_data(dashmediachunk,src_addr,dst_addr);
        return;
    }
    
    /*then it must be a DashAppData*/
    
    DashAppData *appdata=dynamic_cast<DashAppData*>(data);
    if(!appdata)
    {
        
        Log(DASH_LOG_FATAL,"Cannot convert data to dash app data type!");
        return;
    }
    
    /*check http header first*/
    if(appdata->http_header.type==HTTP_REQUEST)
    {
        
        process_request(appdata,src_addr,dst_addr);
        return;
    }
    
    /*check dash data type*/
    DashDataType type=appdata->_dtype;
    
    if(type==PT_TM3U8_META)
    {
        M3U8Meta *meta=dynamic_cast<M3U8Meta*>(data);
        
        if(!meta)
        {
            Log(DASH_LOG_ERROR,"cannot convert to M3U8Meta data!");
            return;
        }
        
        process_m3u8meta_data(meta,src_addr,dst_addr);
    }
    else if(type==PT_TM3U8_ITEM)
    {
        M3U8Item *item=dynamic_cast<M3U8Item*>(data);
        
        if(!item)
        {
            Log(DASH_LOG_ERROR,"cannot convert to M3U8Item data!");
            return;
        }
        
        process_m3u8item_data(item,src_addr,dst_addr);
        
    }
    
    else
    
        Log(DASH_LOG_WARNING,"unknown Dash Data Type received!");
}

void 
DashLL::process_m3u8item_data(M3U8Item* data,int src_addr,int dst_addr)
{
    //Log(DASH_LOG_DEBUG,"received m3u8 item from server %d",addr);
        list<DashClientPlay*>::iterator it;
        for(it=_dash_clients.begin();it!=_dash_clients.end();it++)
        {
            if((*it)->_client_addr==dst_addr)
                break;
        }
        
        if(it==_dash_clients.end())
        {
            Log(DASH_LOG_WARNING,"received m3u8item from server %d to client %d,but record has not been established",src_addr,dst_addr);
            return;
        }
        
        (*it)->_segment_duration=data->_segment_duration;
        (*it)->_total_duration=data->_total_duration;
        (*it)->_total_segments=data->_total_duration/data->_segment_duration;
        
        (*it)->_streaming_started=true;
        
        
}

void 
DashLL::process_m3u8meta_data(M3U8Meta* data, int src_addr,int dst_addr)
{
     
     /*first check if there is a record for dst_addr*/
        list<DashClientPlay*>::iterator it;
        for(it=_dash_clients.begin();it!=_dash_clients.end();it++)
        {
            if((*it)->_client_addr==dst_addr)
                break;
        }
        
        if(it==_dash_clients.end())
        {
            Log(DASH_LOG_WARNING,"received m3u8meta from server %d to client %d,but record has not been established",src_addr,dst_addr);
            return;
        }
        
        if(data->_program_id<0)
            return;
        
        
        (*it)->_media_id=data->_program_id;
         int i;
         for(i=0;i<50;i++)
        {  
        if(data->_bwqueue[i].bandwidth<=0)
            break;
       
        (*it)->_bwqueue.push_back(data->_bwqueue[i]);
        
        (*it)->_max_bandwidth_version=data->_bwqueue[i].bandwidth;
            
        }
        
    
}

void
DashLL::process_media_data(DashMediaChunk* chunk,int src_addr,int dst_addr)
{
    // Log(DASH_LOG_DEBUG,"received media data from server %d",addr);
        list<DashClientPlay*>::iterator it;
        for(it=_dash_clients.begin();it!=_dash_clients.end();it++)
        {
            if((*it)->_client_addr==dst_addr)
                break;
        }
        
        if(it==_dash_clients.end())
        {
            Log(DASH_LOG_WARNING,"received media data from server %d to client %d,but record has not been established",src_addr,dst_addr);
            return;
        }
        
        
        if(chunk->_segment_id>(*it)->_current_received_segment_id)
        {
            
        (*it)->_current_received_segment_id=chunk->_segment_id;
        (*it)->_current_received_ts_id=chunk->_transport_stream_id;
        
        }
        
        if(chunk->_transport_stream_id>(*it)->_current_received_ts_id)
        (*it)->_current_received_ts_id=chunk->_transport_stream_id;
        
}

void
DashLL::process_transmitted_media_data(DashMediaChunk* chunk, int dst_addr)
{
        list<DashClientPlay*>::iterator it;
        for(it=_dash_clients.begin();it!=_dash_clients.end();it++)
        {
            if((*it)->_client_addr==dst_addr)
                break;
        }
        
        if(it==_dash_clients.end())
        {
            Log(DASH_LOG_WARNING,"transmitted media data to client %d,but record has not been established",dst_addr);
            return;
        }
        
         if(chunk->_segment_id>(*it)->_current_transmitted_segment_id)
        {
             
            
        (*it)->_current_transmitted_segment_id=chunk->_segment_id;
        
        (*it)->_seg_trans_start_time=NOW;
        
        
        }
        
        if(chunk->_transport_stream_id>(*it)->_current_transmitted_ts_id )
        {
        (*it)->_current_transmitted_ts_id=chunk->_transport_stream_id;
        
        (*it)->_transmitted_segment_size+=chunk->_media_packet_size;
        
       
        
        
        //check if all packets in the current segment has been transmitted
        if((*it)->_transmitted_segment_size>=(*it)->_current_segment_total_size)
        {
            SegTransHistory segtrans;
            segtrans.start=(*it)->_seg_trans_start_time;
            segtrans.end=NOW;
            segtrans.duration=segtrans.end-segtrans.start;
            segtrans.bitrate=((*it)->_current_segment_total_size)/segtrans.duration;
            
            (*it)->_seg_trans_history.push_back(segtrans);
            
            //clear history data for next segment
          //  (*it)->_current_segment_total_size=0;
            (*it)->_current_transmitted_ts_id=-1;
            (*it)->_transmitted_segment_size=0;
            
            
            if(chunk->_segment_id>=(*it)->_total_segments-1)
            {
                fprintf(stderr,"all segments have been transmitted to client %d\n",dst_addr);
                update_client_number(dst_addr,false);
            }
           
            
        }
        
        
        }
        
        
}

void
DashLL::process_request(DashAppData *appdata, int src_addr,int dst_addr)
{
    /*parse the requested url*/
    char resource_name[50];
    char resource_item[10];
    int resource_id=-1;
    memset(resource_name,0,50);
    memset(resource_item,0,10);
    
    sscanf(appdata->http_header.http_url,"%s %s %d",resource_name,resource_item,&resource_id);
    
    
    /*if received the meta request, establish a record for this client*/
    if(!strcmp(resource_item,"meta"))
    {   
        
        
        list<DashClientPlay*>::iterator it;
        for(it=_dash_clients.begin();it!=_dash_clients.end();it++)
        {
            if((*it)->_client_addr==src_addr)
                break;
        }
        
        if(it!=_dash_clients.end())
        {
            Log(DASH_LOG_WARNING,"client %d requested m3u8meta ,but record has been established",src_addr);
            return;
        }
        
        
        DashClientPlay *client=new DashClientPlay(src_addr,resource_name);
        assert(client);
        _dash_clients.push_back(client);
        
        Log(DASH_LOG_DEBUG,"client %d requested m3u8 meta for resource %s",src_addr,resource_name);
        
        
        if(src_addr>_max_client_number)
        {
            _max_client_number=src_addr;
            if(_max_client_number>=10)
            {
                fprintf(stderr,"too many clients in the network!\n");
                exit(0);
            }
        }
       
        return;
    }
    
    /*sanity check, to make sure there is a record for this client*/
    else if(!strcmp(resource_item,"item"))
    {
        
        list<DashClientPlay*>::iterator it;
        for(it=_dash_clients.begin();it!=_dash_clients.end();it++)
        {
            if((*it)->_client_addr==src_addr)
                break;
        }
        
        if(it==_dash_clients.end())
        {
            Log(DASH_LOG_WARNING,"client %d requested m3u8item, but record is not found",src_addr);
            return;
        }
        
        if(strcmp((*it)->_media_name,resource_name))
        {
           Log(DASH_LOG_WARNING,"client %d requested resource %s, but previously it requested %s",src_addr,resource_name,(*it)->_media_name);
           
           return;
        }
        return;
        
    }
    
    /*media data request, record current requested id and bandwidth*/
    else if(atoi(resource_item)>0 && resource_id>=0)
    {
        list<DashClientPlay*>::iterator it;
        for(it=_dash_clients.begin();it!=_dash_clients.end();it++)
        {
            if((*it)->_client_addr==src_addr)
                break;
        }
        
        if(it==_dash_clients.end())
        {
            Log(DASH_LOG_WARNING,"client %d requested %s data , but record is not found",src_addr,resource_name);
            return;
        }
        
        int record_number=(*it)->_request_bandwidth_history.size();
        if(record_number!=resource_id)
        {
            Log(DASH_LOG_WARNING,"client %d should request segment %d, but requests %d",src_addr,record_number,resource_id);
            return;
        }
        else
        {
            (*it)->_current_requested_segment_id=resource_id;
            
            Log(DASH_LOG_WARNING,"client %d requested segment %d", src_addr, resource_id);
            int bandwidth=atoi(resource_item);
            
            /*check the validity of the requested bandwidth*/
             std::vector<_BWItem>::iterator itt;
             for(itt=(*it)->_bwqueue.begin();itt!=(*it)->_bwqueue.end();itt++)
             {
                 if((*itt).bandwidth==bandwidth)
                     break;
             }
             
             if(itt==(*it)->_bwqueue.end())
             {
                 Log(DASH_LOG_WARNING,"client %d requested bandwidth %d, but this bandwidth version is not recorded",src_addr,bandwidth);
             }
             
             //record the original bandwidth version the client requests
             BWReqHistory orig_bwreqhis;
             orig_bwreqhis.segment_id=record_number;
             orig_bwreqhis.bandwidth=bandwidth;
             orig_bwreqhis.time=NOW;
             (*it)->_original_request_bandwidth_history.push_back(orig_bwreqhis);
             //update_request_trend;
             (*it)->update_request_trend();
             
             
             
             int new_bandwidth=bandwidth;
             
             //new_bandwidth=change_requested_bandwidth(*it,bandwidth);
             
             if(new_bandwidth!=bandwidth)
             {
                sprintf(appdata->http_header.http_url,"%s %d %d",resource_name,new_bandwidth,resource_id); 
                fprintf(stderr,"client %d requested bandwidth %d, but it has been changed to %d\n",src_addr,bandwidth/1000,new_bandwidth/1000);
             }
                 
             
             BWReqHistory bwreqhis;
             bwreqhis.segment_id=record_number;
             bwreqhis.bandwidth=new_bandwidth;
             bwreqhis.time=NOW;
             
        
            (*it)->_request_bandwidth_history.push_back(bwreqhis);
            
            (*it)->_current_segment_total_size=((*it)->_segment_duration)*new_bandwidth/8;
            
           
           
        }
        
    }
    
    else
    
        Log(DASH_LOG_WARNING,"received invalid request: %s\n",appdata->http_header.http_url);
}

Packet* DashLL::select_packet()
{  
    
    Packet *p=NULL;
    DashPriQueue *dashq=dynamic_cast<DashPriQueue*>(downtarget_);
    if(dashq)
    {
        switch(_spolicy)
        {
        case SP_ROUND_ROBIN:
                
           p=dashq->select_packet_rr();
           break;
           
        case SP_AIRTIME_ROUND_ROBIN:
        
            
            //p=dashq->select_packet_atrr();
            //break;
        
        case SP_FIFO:
            p=dashq->select_packet_fifo();
            break;
        
        case SP_DEFICIT_ROUND_ROBIN:
                
            p=dashq->select_packet_drr();
            break;
            
        case SP_WORST_CASE_WEIGHTED_FAIR:
            
            
            p=dashq->select_packet_wf2q();
            
            
            break;
                
        default:
            
            p=dashq->select_packet_fifo();
                
        }
       
    }
    else
    {
        fprintf(stderr,"underlying queue is not DashPriQueue Type!\n");
        fprintf(stderr,"%s %s\n",typeid(ifq_).name(),typeid(*this).name());
    }
    
   
    
    return p;
    
}

void DashLL::update_underlying_bandwidth(int client_addr)
{
     /*get statistics data*/
    PeerStats *stat=_peerstatsdb->getPeerStats(_mac80211mr->addr(),client_addr);
    assert(stat);
     
    /*save its current bit-rate to table*/
    _client_bw_table[client_addr]._current_bitrate=stat->arf->_current_bitrate;
    _client_bw_table[client_addr]._smoothed_bitrate=stat->arf->_bitrate_des.smoothed_value;
    _client_bw_table[client_addr]._trend=stat->arf->_bitrate_des.trend;
    
   
    
}

void DashLL::update_client_number(int client_addr, bool add)
{
    std::set<int>::iterator it=_client_set.find(client_addr);
    
    if(add)
    {
    if(it==_client_set.end())
    {
       std::pair<std::set<int>::iterator,bool> ret=_client_set.insert(client_addr);
       
       if(ret.second==false)
       {
           fprintf(stderr,"Error! client %d already exists the set!\n",client_addr);
           exit(-1);
       
       }
       else
           fprintf(stderr,"Client %d has been inserted into the AP set!\n",client_addr);
     
    }
    }
    else
    {
        if(it==_client_set.end())
        {
            fprintf(stderr,"Error! Going to remove Client %d, which is not in the set!\n",client_addr);
        }
        
        _client_set.erase(it);
    }
    
}

void DashLL::update_underlying_queue_weight(int client_addr)
{
     //dynamically adapt weight of underlying queue
      DashPriQueue *dashq=dynamic_cast<DashPriQueue*>(downtarget_);
            
      if(dashq==NULL)
      {
       fprintf(stderr,"the underlying queue is not DashPriQueue type!\n");
       exit(-1);
      }
            
      int client_bw=(int)(_client_bw_table[client_addr]._current_bitrate);
      double weight;
      double avg_bandwidth=20000.0;
      
      if(client_bw<=0)
          weight=1.0;
      else
          weight=client_bw/avg_bandwidth;
          
                
           // double avg_band=14000.0/6.0;
            // weight=avg_band/(bandwidth/1000);
           
           // dashq->set_weight(src_addr,weight);
          dashq->set_weight_direct(client_addr,weight);
            
       //  fprintf(stderr,"at time %f, weight %f is set for dst %d with total %d clients\n",NOW,weight,client_addr,_client_set.size());
}

int DashLL::change_requested_bandwidth(DashClientPlay* client_db, int bandwidth)
{
   
    /*if this is the first requested segment, return*/
    if(client_db->_current_requested_segment_id==0)
        return bandwidth;
    
    int original_bw=bandwidth/1000;
    int new_bw=bandwidth;
    
    if(client_db->_seg_trans_history.size()==0)
    {
        fprintf(stderr,"Error when requesting segment %d, should have at least one transmitted history!\n",client_db->_current_requested_segment_id);
        exit(-1);
    }
    
    SegTransHistory sth=client_db->_seg_trans_history.front();
    SegTransHistory sth_last=client_db->_seg_trans_history.back();
    
    double first_seg_start_play_time=sth.end;
    double last_seg_start_play_time=sth_last.end;
    
    fprintf(stderr,"for client %d, first seg start play time %f, duration %f\n",client_db->_client_addr,first_seg_start_play_time,sth.duration);
    fprintf(stderr,"for client %d, last  seg start play time %f, duration %f\n",client_db->_client_addr,last_seg_start_play_time,sth_last.duration);
    
    double transmit_deadline_1=first_seg_start_play_time+client_db->_segment_duration*client_db->_current_requested_segment_id;
    double transmit_deadline_2=last_seg_start_play_time+client_db->_segment_duration;
    
    double transmit_deadline=max(transmit_deadline_1,transmit_deadline_2);
    
    double quota_time=transmit_deadline-NOW;
    
    fprintf(stderr,"for client %d, quota time is %f\n",client_db->_client_addr,quota_time);
    
   
    if(quota_time>0.0)
    {
        double smoothed_bitrate=_client_bw_table[client_db->_client_addr]._smoothed_bitrate;
        double trend=_client_bw_table[client_db->_client_addr]._trend;
        int available_bw=(smoothed_bitrate+trend*client_db->_segment_duration*0.5)/_client_set.size();
        int max_bw=quota_time*available_bw/client_db->_segment_duration;
        
        //max_bw=max_bw>available_bw?available_bw:max_bw;
        
        fprintf(stderr,"for client %d, underlying bw %f, total client no %d, avail bw %d\n",
                client_db->_client_addr,_client_bw_table[client_db->_client_addr]._current_bitrate,
                _client_set.size(),available_bw);
        fprintf(stderr,"for client %d, max bw version %d, previously requested bandwidth %d\n",client_db->_client_addr,max_bw,original_bw);
        
        client_db->_estimated_max_bw.push_back(available_bw*1000);
        
        double coeff_variation=client_db->check_request_stability();
        
        
        double req_variation=client_db->check_original_request_stability();
        
        
        int previous_bandwidth;
        //change bandwidth, if necessary
        if(original_bw>max_bw*0.01 && _client_bw_table[client_db->_client_addr]._current_bitrate>0)
        {
            int bw_temp=max_bw*0.6*1000;
            new_bw=client_db->find_lower_bandwidth(bw_temp);
            
           
            
           int bw_dev=client_db->get_estimated_bw_dev();
            
           //fprintf(stderr,"bw deviation is %d while 0.1max_bw is %d\n",bw_dev,max_bw*100);
            
           if(bw_dev>max_bw*0.2*1000)
              bw_dev=max_bw*0.2*1000;
           // fprintf(stderr,"requested is %d while max is %d\n",bandwidth,max_bw*1000);
            if(new_bw<client_db->_max_bw)
            {
                int temp_bw=client_db->find_lower_bandwidth(max_bw*0.7*1000);
                
                if(temp_bw>=client_db->_max_bw)
                    new_bw=client_db->_max_bw;
            }
          //  else if(new_bw>client_db->_max_bw)
          //  {
             //   int temp_bw=client_db->find_lower_bandwidth(max_bw*0.6*1000);
             //   if(temp_bw<client_db->_max_bw)
             //       new_bw=client_db->_max_bw;
           // }
            
           
         
          
         // new_bw=(new_bw_high-bw_temp)>(bw_temp-new_bw_low)?new_bw_low:new_bw_high;
          
          client_db->_max_bw=new_bw;
          
          /*check previous requested bandwidth*/
          
          fprintf(stderr,"have sent %d segments to client\n",client_db->_request_bandwidth_history.size());
          previous_bandwidth= client_db->_request_bandwidth_history.back().bandwidth;
          if(new_bw>previous_bandwidth)
          {
              //fprintf(stderr,"for client %d, change from %d to %d\n",client_db->_client_addr,previous_bandwidth,new_bw);
              fprintf(stderr,"the bitrate trend is %f\n",trend);
              fprintf(stderr,"the request trend is %d\n",client_db->_request_trend);
             
              
             if(trend<0.0 || client_db->_request_trend!=RTREND_UP)
                new_bw=previous_bandwidth;
             // if(client_db->_request_trend!=RTREND_DOWN && coeff_variation==0.0)
                // new_bw=client_db->find_next_bandwidth(previous_bandwidth,1);
             
                  
          }
        }
        
          if(new_bw>bandwidth )
          {
          
              
              
            new_bw=bandwidth;
              
          }
        
        //check stability of transmitted version
        
        int max_available=client_db->_max_bw;
        int previous_available=previous_bandwidth>max_available?max_available:previous_bandwidth;
        int requested_available=bandwidth>max_available?max_available:bandwidth;
        
        int lower_thresh=client_db->min3(max_available,previous_available,requested_available);
        int higher_thresh=client_db->max3(max_available,previous_available,requested_available);
        
        if(max_bw>0)
        {
        double util=new_bw/(double)(1000*max_bw);
        double satis=new_bw/(double)bandwidth;
        fprintf(stderr,"util: %f satis %f var %f requested bandwidth %d\n",util,satis,coeff_variation,bandwidth/1000);
        
        int original_new_bw=new_bw;
        
        
        double util_new_bw=client_db->check_util(new_bw);
        double util_previous=client_db->check_util(previous_available);
        double util_requested=client_db->check_util(requested_available);
        
        if(client_db->_trend_file)
        {
            
            fprintf(client_db->_trend_file,"%d %d %f %f %f %f %f %f %f\n",client_db->_current_requested_segment_id,bandwidth/1000,NOW,util,satis,coeff_variation,
                    util_new_bw,util_previous,util_requested);
            fprintf(client_db->_trend_file,"# threash %d %d %d %d %d\n",lower_thresh,higher_thresh,new_bw,previous_available,requested_available);
            fflush(client_db->_trend_file);
        }
        
        
        double max_util=util_new_bw;
        //change bw according to utility function value
        if(util_previous>max_util  || (util_previous==max_util && previous_available<new_bw))
        {
            max_util=util_previous;
            new_bw=previous_available;
        }
           
        if(util_requested>max_util || (util_requested==max_util && requested_available<new_bw))
        {
            max_util=util_requested;
            new_bw=requested_available;
        }
         
        
     // if(new_bw>previous_available)
          // new_bw=client_db->find_next_bandwidth(previous_available,1);
       
         
        // new_bw=client_db->check_util_range(lower_thresh,new_bw);
        }
        
       
        
    }
    else
    {
        
        if(_client_bw_table[client_db->_client_addr]._current_bitrate>0)
         new_bw=client_db->_bwqueue.at(0).bandwidth;
    }
    
    
    
   
    
   
    
  /* 
    if(bandwidth/1000>max_bandwidth*1.2)
    {
       
        bandwidth=client_db->find_lower_bandwidth(max_bandwidth*1000);
        fprintf(stderr,"client %d changed to %d\n",client_db->_client_addr,bandwidth/1000);
    }
    else if(bandwidth/1000<max_bandwidth*0.8)
    {
        bandwidth=client_db->find_next_bandwidth(bandwidth,1);
        fprintf(stderr,"client %d changed to %d\n",client_db->_client_addr,bandwidth/1000);
    }
   * 
   */
    
    return new_bw;
}


int DashLL::get_underlying_bandwidth(DashClientPlay* client_db)
{
    
    int bandwidth=-1;
    /*if this is the first requested segment, return*/
    if(client_db->_current_requested_segment_id==0)
        return bandwidth;
    
     /*get statistics data*/
    PeerStats *stat=_peerstatsdb->getPeerStats(_mac80211mr->addr(),client_db->_client_addr);
    assert(stat);
    double current_bitrate=stat->arf->_current_bitrate; 
    /*save its current bit-rate to table*/
    _client_bw_table[client_db->_client_addr]._current_bitrate=(int)current_bitrate;
    
    
    bandwidth=(int)current_bitrate;
    
    /*if current measured MAC bit-rate has not been sampled yet,return*/
    if(bandwidth<=0)
        return -1;
    
    return bandwidth;
    
   
}

void DashLL::update_recv(Packet* p, int dst)
{
    /*If this is not the AP, return */
    if(_mac80211mr->addr()!=_mac80211mr->bss_id())
    
        return;
  
    /*if this is not a media packet, return*/
    if(p->userdata()==NULL)
        return;
    
    
    /*first check if it is a media data*/
    DashMediaChunk *dashmediachunk=dynamic_cast<DashMediaChunk*>(p->userdata());
    if(dashmediachunk)
    {
        process_transmitted_media_data(dashmediachunk,dst);
        return;
    }
    
    
    
   
}