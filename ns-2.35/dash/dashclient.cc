#include <stdarg.h>
#include <queue>
#include <list>
#include "dashclient.h"
#include "media.h"
using namespace std;
 
int client_number=0;

static class DashHttpClientAppClass : public TclClass {
 public:
	DashHttpClientAppClass() : TclClass("Application/HTTP/DashClient") {}
	TclObject* create(int, const char*const*) {
		return (new DashHttpClient);
	}
} class_app_dashhttpclient;

static class VLCHLSHttpClientAppClass : public TclClass {
 public:
	VLCHLSHttpClientAppClass() : TclClass("Application/HTTP/VLCHLSClient") {}
	TclObject* create(int, const char*const*) {
		return (new VLCHLSHttpClient);
	}
} class_app_vlchlshttpclient;


static class WEMAHttpClientAppClass : public TclClass {
 public:
	WEMAHttpClientAppClass() : TclClass("Application/HTTP/WEMAClient") {}
	TclObject* create(int, const char*const*) {
		return (new WEMAHttpClient);
	}
} class_app_wemahttpclient;

static class VLCDashHttpClientAppClass : public TclClass {
 public:
	VLCDashHttpClientAppClass() : TclClass("Application/HTTP/VLCDashClient") {}
	TclObject* create(int, const char*const*) {
		return (new VLCDashHttpClient);
	}
} class_app_vlcdashhttpclient;

static class AndroidDashHttpClientAppClass : public TclClass {
 public:
	AndroidDashHttpClientAppClass() : TclClass("Application/HTTP/AndroidDashClient") {}
	TclObject* create(int, const char*const*) {
		return (new AndroidDashHttpClient);
	}
} class_app_androiddashhttpclient;


static class VAndroidDashHttpClientAppClass : public TclClass {
 public:
	VAndroidDashHttpClientAppClass() : TclClass("Application/HTTP/VAndroidDashClient") {}
	TclObject* create(int, const char*const*) {
		return (new VAndroidDashHttpClient);
	}
} class_app_vandroiddashhttpclient;


static class SAndroidDashHttpClientAppClass : public TclClass {
 public:
	SAndroidDashHttpClientAppClass() : TclClass("Application/HTTP/SAndroidDashClient") {}
	TclObject* create(int, const char*const*) {
		return (new SAndroidDashHttpClient);
	}
} class_app_sandroiddashhttpclient;


static class AIMDDashHttpClientAppClass : public TclClass {
public:
	AIMDDashHttpClientAppClass() : TclClass("Application/HTTP/AIMDDashClient") {}
	TclObject* create(int, const char*const*) {
		return (new AIMDHttpClient);
	}
} class_app_aimddashhttpclient;


void DashClientDownloadTimer::expire(Event* e)
{
    t_->timeout_download();
}

void DashClientPlayTimer::expire(Event* e)
{
    t_->timeout_play();
}

DashHttpClient::DashHttpClient()
:Application(),_state(DASH_INITIAL),
_current_program_id(-1),
_segment_duration(0),
_total_segments(0),
_received_segment_id(-1),
_current_requested_bandwidth(0),
_current_requested_segment_id(0),
_expected_ts_id(-1),
_expected_play_segment(-1),
_received_segment_size(0),
_bitrate_lower_bound(-1),
_bitrate_upper_bound(-1),
_sent_data(NULL),
_received_data(NULL),
_default_log_level(DASH_LOG_NOTE),
_play_freeze(false),
_download_timer(this),
_play_timer(this),
_download_history_ptr(NULL),
_download_bitrate_ptr(NULL),
_play_history_ptr(NULL),
_play_summary_ptr(NULL),
_addr(-1),
_addr_got(false),
_play_after(1),
MAX_STACK_SIZE(100)
{
    
    memset(_media_url,0,sizeof(_media_url));
    memset(&_qosstat,0,sizeof(_qosstat));
    agent_=NULL;
    strcpy(_client_name,"DashClient");
    
    client_number++;
   
    //=fopen("download_history.txt","w");
    
    
    
}


DashHttpClient::~DashHttpClient()
{
    if(!_cache_queue.empty())
        _cache_queue.pop();
    
    if(!_bwqueue.empty())
        _bwqueue.pop_back();
    
    if(_download_history_ptr)
        fclose(_download_history_ptr);
    
    if(_download_bitrate_ptr)
        fclose(_download_bitrate_ptr);
    
    if(_play_history_ptr)
        fclose(_play_history_ptr);
    
    if(_play_summary_ptr)
        fclose(_play_summary_ptr);
}


void DashHttpClient::recv(int nbytes, AppData* data)
{
    
    /*sanity check*/
    if(nbytes==0 || data==NULL)
    {
        Log(DASH_LOG_FATAL,"received data has nothing inside!");
        return;
        
    }
    
    process_data(nbytes,data);
    
    
}



void DashHttpClient::start()
{
    
    /*sanity check*/
    
    
    if(agent_==NULL)
    {
        Log(DASH_LOG_FATAL,"transport agent is not set!");
        return;
    }
    
    if(strlen(_media_url)==0)
    {
        Log(DASH_LOG_FATAL,"requested media is not specified!");
        return;
    }
    
    if(_state!=DASH_INITIAL)
    {
        Log(DASH_LOG_FATAL,"dash client is not in initial state!");
        return;
    }
    
    if(!_addr_got)
    {
        _addr=agent_->addr();
       
        
        char name[500];
        
        sprintf(name,"download_history_%d.txt",_addr);
        
        _download_history_ptr=fopen(name,"w");
        
        if(_download_history_ptr)
            fprintf(_download_history_ptr,"# %s %s\n",_client_name,this->_media_url);
        
        
        
        sprintf(name,"bitrate_history_%d.txt",_addr);
        
        _download_bitrate_ptr=fopen(name,"w");
        
        if(_download_bitrate_ptr)
            fprintf(_download_bitrate_ptr,"# %s %s\n",_client_name,this->_media_url);
        
        
        sprintf(name,"play_history_%d.txt",_addr);
        _play_history_ptr=fopen(name,"w");
        
        if(_play_history_ptr)
            fprintf(_play_history_ptr,"# %s %s\n",_client_name,this->_media_url);
        
        
        sprintf(name,"play_summary_%d.txt",_addr);
        _play_summary_ptr=fopen(name,"w");
        
         _addr_got=true;
    }
    
    send_m3u8meta_request();
    
    _qosstat.request_start_time=now();
    
    incState();
    
}


void DashHttpClient::stop()
{
    
    
    printStats();
    Tcl &tcl = Tcl::instance();
    tcl.evalf("Simulator instance");
    char SimName[32];
    strncpy(SimName,tcl.result(),32);
    
    //Log(DASH_LOG_DEBUG,"simulator name is %s",SimName);
    
    client_number--;
    
    if(client_number==0)
    {
    sprintf(tcl.buffer(),"%s at %f \"finish\" ",SimName,now()+5.0);
    tcl.eval();
    }
   
}

void DashHttpClient::printStats()
{
    Log(DASH_LOG_NOTE,"start time %f, first seg received time %f, start play time %f, initial waiting time %f",
            _qosstat.request_start_time,_qosstat.first_segment_received_time,
            _qosstat.play_start_time,_qosstat.play_start_time-_qosstat.request_start_time);
    Log(DASH_LOG_NOTE,"play finish time %f, total play duration %f, while media duration %d",
            _qosstat.play_finish_time,_qosstat.play_finish_time-_qosstat.play_start_time,_segment_duration*_total_segments);
    
    /*write to file*/
    Log(_play_summary_ptr,DASH_LOG_NOTE,"start time %f, first seg received time %f, start play time %f, initial waiting time %f\n",
            _qosstat.request_start_time,_qosstat.first_segment_received_time,
            _qosstat.play_start_time,_qosstat.play_start_time-_qosstat.request_start_time);
    Log(_play_summary_ptr,DASH_LOG_NOTE,"play finish time %f, total play duration %f, while media duration %d\n",
            _qosstat.play_finish_time,_qosstat.play_finish_time-_qosstat.play_start_time,_segment_duration*_total_segments);
    
 //==========================================================================================================
    if(_qosstat.total_seg_played>0)
    {
    float avg_bandwidth=(float)(_qosstat.total_bandwidth)/(float)(_qosstat.total_seg_played);
    Log(DASH_LOG_NOTE,"average bitrate for played video: %f bs, or %f kbs",avg_bandwidth,avg_bandwidth/1000.0);
    
    Log(_play_summary_ptr,DASH_LOG_NOTE,"average bitrate for played video: %f bs, or %f kbs\n",avg_bandwidth,avg_bandwidth/1000.0);
    }
    
    double freeze_ratio=(double)_qosstat.total_freeze/(float)_total_segments;
    Log(DASH_LOG_NOTE,"freeze count %d, freeze ratio %lf, total freeze duration %f",
            _qosstat.total_freeze,freeze_ratio,_qosstat.avg_freeze_duration);
   
    
    
    Log(_play_summary_ptr,DASH_LOG_NOTE,"freeze count %d, freeze ratio %lf, total freeze duration %f\n",
            _qosstat.total_freeze,freeze_ratio,_qosstat.avg_freeze_duration);
   
    _qosstat.avg_freeze_duration=_qosstat.total_freeze==0?0.0:_qosstat.avg_freeze_duration/_qosstat.total_freeze;
     
     
    Log(DASH_LOG_NOTE,"average freeze time %f",_qosstat.avg_freeze_duration);
    
    Log(DASH_LOG_NOTE,"quality change time %d, change ratio %f",_qosstat.quality_change,(float)_qosstat.quality_change/(float)_total_segments);
    
    Log(_play_summary_ptr,DASH_LOG_NOTE,"average freeze time %f\n",_qosstat.avg_freeze_duration);
    
    Log(_play_summary_ptr,DASH_LOG_NOTE,"quality change time %d, change ratio %f\n",_qosstat.quality_change,(float)_qosstat.quality_change/(float)_total_segments);
    
    double mos=4.23-0.0672*(_qosstat.play_start_time-_qosstat.request_start_time)-
    0.742*freeze_ratio-0.106*_qosstat.avg_freeze_duration;
    
    Log(DASH_LOG_NOTE,"MOS: %lf",mos);
    
    Log(_play_summary_ptr,DASH_LOG_NOTE,"MOS: %lf\n",mos);
    
    int downloaded_kb=_total_download_size/125;
    fprintf(stderr,"total downloaded kb %d\n",downloaded_kb);
    Log(DASH_LOG_NOTE,"average download speed: %f kbs\n",downloaded_kb/(double)_total_download_time);
    Log(_play_summary_ptr,DASH_LOG_NOTE,"average download speed: %f kbs\n",downloaded_kb/(double)_total_download_time);
    
}




void DashHttpClient::process_data(int size, AppData* data)
{
    
    /*first check if it is a media data*/
    DashMediaChunk *dashmediachunk=dynamic_cast<DashMediaChunk*>(data);
    if(dashmediachunk)
    {
        process_media_data(dashmediachunk);
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
    if(appdata->http_header.type!=HTTP_RESPONSE)
    {
        /*client does not respond to http request*/
        Log(DASH_LOG_WARNING,"received http request and ignored!");
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
        
        process_m3u8meta_data(meta);
    }
    else if(type==PT_TM3U8_ITEM)
    {
        M3U8Item *item=dynamic_cast<M3U8Item*>(data);
        
        if(!item)
        {
            Log(DASH_LOG_ERROR,"cannot convert to M3U8Item data!");
            return;
        }
        
        process_m3u8item_data(item);
        
    }
    
    else
    
        Log(DASH_LOG_ERROR,"unknown Dash Data Type received!");
    
    
       
    
        
}

void DashHttpClient::incState()
{
    if(_state<DASH_FINISHED)
        _state=(enum client_state)(_state+1);
}


void DashHttpClient::process_media_data(DashMediaChunk* chunk)
{
           
             
           /*we ignore non-continuous media data, as this is very unlikely and should never happen in application layer*/
           /* If this happens, it indicates an implementation error in TCP layer*/
            if(chunk->_segment_id!=_current_requested_segment_id
                    ||chunk->_transport_stream_id!=_expected_ts_id)
                
            { 
             
                Log(DASH_LOG_ERROR, "requested segment %d:%d but received %d:%d",
                        _current_requested_segment_id,_expected_ts_id,
                         chunk->_segment_id,chunk->_transport_stream_id);
           
                return;
            }
            
            
            /*but varied bandwidth is allowed*/
            if(chunk->_bandwidth!=_current_requested_bandwidth)
            {
                Log(DASH_LOG_WARNING, "requested bandwidth %d but received %d",
                        _current_requested_bandwidth,chunk->_bandwidth);
                
                _current_requested_bandwidth=chunk->_bandwidth;
                
                
                
            }
            
           
            
    
            
             _received_segment_size+=chunk->_media_packet_size;
             _expected_ts_id++;
             
          
   
    
    /*check if we have received all the media data of this segment*/
    if(_received_segment_size>=(_current_requested_bandwidth*_segment_duration/8))
    {   
        
        
        
        Log(DASH_LOG_NOTE,"segment %d received",_current_requested_segment_id);
        
       if(_download_history_ptr)
       fprintf(_download_history_ptr," %f %lf \n",now(),((double)_current_requested_bandwidth/(double)1e6));
        
        
        if(_current_requested_segment_id==0)
            _qosstat.first_segment_received_time=now();
            
        _segment_download_end=now();
            
        PlaySegment ps;
        ps.bandwidth=_current_requested_bandwidth;
        ps.segment_id=_current_requested_segment_id;
        _cache_queue.push(ps);
        
        
        
        _current_requested_segment_id++;
        _current_request_sent=false;
        
        if(_current_requested_segment_id>=_total_segments)
        {  
            
            
            Log(DASH_LOG_NOTE,"received all %d segments. Download finishes!",_total_segments);
            
            _current_requested_segment_id=-1;
        }
        
        /*tell the app that one segment has been downloaded*/
        if(_play_timer.status()!=TIMER_PENDING)
        _play_timer.resched(0.05); // denote the decoding time for the first few frames before display
        else if(_current_requested_segment_id>=0 && _cache_queue.size()<3)
        {
            _download_timer.resched(0.0);
        }
        
        /*request the next segment, if any*/
        /*decide if continue to download*/
        
        
        
        
        
        _received_segment_size=0;
        _expected_ts_id=0;
        
        
        /*change bandwidth,if necessary*/
        calculate_bandwidth();
        change_requested_bandwidth();
        
        
    }
    
}

void DashHttpClient::process_m3u8item_data(M3U8Item* data)
{
    if(_state!=DASH_M3U8ITEM_REQUEST_SENT)
    {
        Log(DASH_LOG_FATAL,"should be in m3u8item request sent state!");
        return;
    }
    
    _segment_duration=data->_segment_duration;
    
    if(_segment_duration>0)
    _total_segments=data->_total_duration/data->_segment_duration;
    else
    {
        Log(DASH_LOG_FATAL,"received an invalid segment duration %d",_segment_duration);
        return;
    }
    
    if(_total_segments<=0)
    {
        Log(DASH_LOG_FATAL,"received a media file with %d segments",_total_segments);
        return;
    }
    
    
    incState();
    
    Log(DASH_LOG_NOTE,"m3u8 file reception completed");
    Log(DASH_LOG_NOTE,"media file %s has %d bandwidths version, seg duration %d, total segments no %d",_media_url,_bwqueue.size(),
            _segment_duration,_total_segments);
    unsigned int i;
    for(i=0;i<_bwqueue.size();i++)
    
        Log(DASH_LOG_NOTE,"bandwidth %d: %d",i,_bwqueue.at(i).bandwidth);
    
    
    /*now requesting the first segment*/
    /*if the lower bound is not set, starting from the lowest bandwidth*/
    if(_bitrate_lower_bound<=0)
    _current_requested_bandwidth=_bwqueue.at(0).bandwidth;
    else
    _current_requested_bandwidth=higher_bandwidth(_bitrate_lower_bound); 
        
        
        
        
    _current_requested_segment_id=0;
    _expected_ts_id=0;
    _expected_play_segment=0;
    _total_download_size=0;
    _total_download_time=0;
    
    
   // send_media_data_request();
    
    /*check the validity of the upper and lower bitrate requirement of the client*/
    if(_bitrate_lower_bound>0 && _bitrate_upper_bound>0)
        if(_bitrate_upper_bound-_bitrate_lower_bound<512000)
        {
            Log(DASH_LOG_ERROR,"bitrate lower bound set at %d, upper bound set at %d",_bitrate_lower_bound,_bitrate_upper_bound);
            Log(DASH_LOG_ERROR,"Higher bound should be at least 0.5M larger than lower bound to avoid oscillation!");
            exit(0);
        }
    
    
   // if(_bitrate_lower_bound>0)
     //   _current_requested_bandwidth=
                
                
                
    _current_request_sent=false;
    _download_timer.sched(0.0);
    
    
    
    
    
    
}

void DashHttpClient::process_m3u8meta_data(M3U8Meta* data)
{  
    Log(DASH_LOG_DEBUG,"received m3u8 meta response!");
    
    if(_state!=DASH_M3U8META_REQUEST_SENT)
    
    {
        Log(DASH_LOG_FATAL,"should be in m3u8meta request sent state!");
        return;
    }
    
    if(data->_program_id<0)
    {
        
        
        Log(DASH_LOG_WARNING,"requested media is not found on the server, client going to stop!");
        stop();
    }
    
    _current_program_id=data->_program_id;
    
    int i;
    for(i=0;i<50;i++)
    {  
        if(data->_bwqueue[i].bandwidth<=0)
            break;
       
    _bwqueue.push_back(data->_bwqueue[i]);
    }
    
    
    if(_bwqueue.size()==0)
    {
        Log(DASH_LOG_FATAL,"received no bandwidth version");
        return;
    }
    
    
    incState();
           
    send_m3u8item_request();
    
    incState();
}


void DashHttpClient::send_m3u8meta_request()
{
    _sent_data=new DashAppData(PT_TREQUEST);
    
    _sent_data->http_header.type=HTTP_REQUEST;
    sprintf(_sent_data->http_header.http_url,"%s %s %d",_media_url,"meta",0);
    
   //  Log(DASH_LOG_DEBUG,"send m3u8 meta request: %s!",_sent_data->http_header.http_url);
    
    send(_sent_data->size(),_sent_data);
    
   //  Log(DASH_LOG_DEBUG,"m3u8 request sent finished with size %d!",_sent_data->size());
    
    delete _sent_data;
    _sent_data=NULL;
}


/*in real applications client should send a request to retrieve m3u8 file for each bandwidth
 *so that the client knows the exact location of each media file on the server
 *as each bandwidth version has the same segment length and total media length
 * we send only one m3u8 request to get this segment length */
void DashHttpClient::send_m3u8item_request()
{
    
    _sent_data=new DashAppData(PT_TREQUEST);
    
    _sent_data->http_header.type=HTTP_REQUEST;
    sprintf(_sent_data->http_header.http_url,"%s %s %d",_media_url,"item",0);
    
    send(_sent_data->size(),_sent_data);
    
    delete _sent_data;
    _sent_data=NULL;
    
   
    
}

void DashHttpClient::send_media_data_request()
{
    _sent_data=new DashAppData(PT_TREQUEST);
    
    _sent_data->http_header.type=HTTP_REQUEST;
    sprintf(_sent_data->http_header.http_url,"%s %d %d",_media_url,_current_requested_bandwidth,_current_requested_segment_id);
    
    send(_sent_data->size(),_sent_data);
    
   
    
    delete _sent_data;
    _sent_data=NULL;
    
    _segment_download_start=now();
    
    _state=DASH_STREAM_REQUEST_SENT;
    
    if(_download_history_ptr)
        fprintf(_download_history_ptr,"%f %lf ",now(),((double)_current_requested_bandwidth/(double)1e6));
    
}

void DashHttpClient::send(int size, AppData* data)
{
    
    
    agent_->sendmsg(size,data,NULL);
}


void DashHttpClient::Log(int log_level, const char* fmt, ...)
{
    va_list arg_list;
    va_start(arg_list,fmt);
    
    char name[50];
    sprintf(name,"DASH-CLIENT %d",_addr);
    
    
    
    DashLog(stdout,log_level,_default_log_level,DASH_COLOR_GREEN,name,fmt,arg_list);
    va_end(arg_list);
    
    /*those fatal situations should not happen*/
    if(log_level==DASH_LOG_FATAL)
        exit(0);
    
}


void DashHttpClient::Log(FILE* stream, int log_level, const char* fmt, ...)
{
    if(stream==NULL)
        return;
    
    
    va_list arg_list;
    va_start(arg_list,fmt);
    
    
    
    vfprintf(stream, fmt,arg_list);
    va_end(arg_list);
    
    /*those fatal situations should not happen*/
    if(log_level==DASH_LOG_FATAL)
        exit(0);
}

int DashHttpClient::command(int argc, const char* const * argv)
{
    Tcl& tcl = Tcl::instance();

  if (argc == 3) {
    if (strcmp(argv[1], "attach-agent") == 0) {
      agent_ = (Agent*) TclObject::lookup(argv[2]);
      if (agent_ == 0) {
	tcl.resultf("no such agent %s", argv[2]);
	return(TCL_ERROR);
       
      }
      agent_->attachApp(this);
      
      return(TCL_OK);
    }
    else if(strcmp(argv[1],"request")==0){
        
        strcpy(_media_url,argv[2]);
        return(TCL_OK);
    }
    
    else if (strcmp(argv[1],"set-bitrate-lower")==0){
        
        
        _bitrate_lower_bound=atoi(argv[2]);
        return(TCL_OK);
        
    }
    
    else if(strcmp(argv[1],"play-after")==0){
        
        _play_after=atoi(argv[2]);
        
        return(TCL_OK);
        
    }
    else if (strcmp(argv[1],"set-bitrate-upper")==0){
        
        _bitrate_upper_bound=atoi(argv[2]);
        return(TCL_OK);
    }
        
        
  }

  return(Application::command(argc, argv));
}


void DashHttpClient::resume()
{
    
}

void DashHttpClient::recv(int nbytes)
{
    abort();
}

void DashHttpClient::calculate_bandwidth()
{
    int download_size=_current_requested_bandwidth*_segment_duration/8;
    double download_duration=_segment_download_end-_segment_download_start;
    if(download_duration<=0.0)
    {
        Log(DASH_LOG_WARNING,"calculated download duration is %lf",download_duration);
        return;
    }
    
    double bitrate=8*download_size/download_duration;
    
    DownloadSegment ds;
    ds.size=download_size;
    ds.time=download_duration;
    ds.bitrate=bitrate/1000.0;
    
    _bitrate_history.push_back(ds);
    
    
    
    _total_download_size+=download_size;
    _total_download_time+=download_duration;
    
    
    
    if(_bitrate_history.size()>=MAX_STACK_SIZE)
        _bitrate_history.pop_front();
    
    Log(DASH_LOG_WARNING,"calculated download bitrate is %lf kb/s",bitrate/1000);
    
   
}


/*do nothing, to be overridden by inherited classes*/
void DashHttpClient::change_requested_bandwidth()
{
    abort();
}

void DashHttpClient::timeout_download()
{
    
    /*download is not pipelined, a new download is started only when
     the previous one has finished*/
    if(_current_requested_segment_id<0 || _current_request_sent)
        return;
    
    
    send_media_data_request();
    
    _current_request_sent=true;
}

void DashHttpClient::timeout_play()
{  
    
    
    /*set download timer*/
        
  //  _download_timer.resched(0.0);
    
    if(_cache_queue.empty())
    {  
        if(_expected_play_segment>=_total_segments)
        {
           Log(DASH_LOG_NOTE,"played all segments!");
           stop();
           return;
        }
        else
        {
        Log(DASH_LOG_WARNING,"should play segment %d, but nothing to play from buffer!",_expected_play_segment);
        _qosstat.total_freeze++;
        _play_freeze=true;
        _start_freeze_time=now();
        
        
        
        return;
        }
    }
    
    
    
    /*wait until the previous segment has finished playing*/
    if(_play_timer.status()==TIMER_PENDING)
        return;
    
    PlaySegment ps=_cache_queue.front();
    _cache_queue.pop();
    
    if(ps.segment_id!=_expected_play_segment)
    
        Log(DASH_LOG_WARNING,"expect to play segment %d, but currently only segment %d available!",_expected_play_segment,ps.segment_id);
    
    
    
    
    
    play_media(ps);
    
        
    
}

void DashHttpClient::play_media(PlaySegment& ps)
{
    if(ps.segment_id==0)
        _qosstat.play_start_time=now();
    else if(ps.segment_id==_total_segments-1)
        _qosstat.play_finish_time=now()+_segment_duration;
    
    _qosstat.total_seg_played++;
    _qosstat.total_bandwidth+=ps.bandwidth;
    
    if(_play_freeze)
    {
        _qosstat.avg_freeze_duration+=now()-_start_freeze_time;
        _play_freeze=false;
    }
    
    if(_expected_play_segment!=0 && ps.bandwidth!=_prev_played_bandwidth)
    _qosstat.quality_change++;
    
    _expected_play_segment++;
    _prev_played_bandwidth=ps.bandwidth;
    
    
    Log(DASH_LOG_WARNING,"now playing segment %d from bandwidth %d",ps.segment_id,ps.bandwidth);
    
    
    if(_play_history_ptr)
        fprintf(_play_history_ptr,"%d %lf\n",ps.segment_id,now());
    
    
    _play_timer.resched(_segment_duration);
}




void VLCHLSHttpClient::change_requested_bandwidth()
{
    
    if(_bitrate_history.empty())
        return;
    
    
    DownloadSegment ds=_bitrate_history.back();
    
    int bitrate=(int)(8*ds.size/ds.time);
    
    _current_requested_bandwidth=best_matching_bandwidth(bitrate);
    
     if(_download_bitrate_ptr)
    {
        fprintf(_download_bitrate_ptr,"%f %f %f %d\n",NOW,ds.time,bitrate/1000.0,_current_requested_bandwidth/1000);
    }
    
}

void VLCHLSHttpClient::timeout_play()
{
    /*decide if continue to download*/
 /*   if(_current_requested_segment_id>=0 && _current_requested_segment_id-_expected_play_segment<3)
    {
        _download_timer.resched(0);
    }
 */
    
    if(_current_requested_segment_id>=0 && _cache_queue.size()<4)
    {
        _download_timer.resched(0.0);
    }
    
    
    
     if(_cache_queue.empty())
    {  
        if(_expected_play_segment>=_total_segments)
        {
           Log(DASH_LOG_NOTE,"played all segments!");
           stop();
           return;
        }
        else
        {
        Log(DASH_LOG_WARNING,"should play segment %d, but nothing to play from buffer!",_expected_play_segment);
        _qosstat.total_freeze++;
        _play_freeze=true;
        _start_freeze_time=now();
        return;
        }
    }
     
     
     /*only start playing if two segments have been downloaded*/
   //  if(_expected_play_segment==0 && _cache_queue.size()<2)
       //  return;
    
    /*only start playing if the downloaded segment number is no less than the specified value*/
     if(_expected_play_segment==0 && _cache_queue.size()<_play_after)
        return;
    
    
    /*only start playing if the previous segment has finished playing*/
    if(_play_timer.status()==TIMER_PENDING)
        return;
    
    
    PlaySegment ps=_cache_queue.front();
    _cache_queue.pop();
    
    if(ps.segment_id!=_expected_play_segment)
    
        Log(DASH_LOG_WARNING,"expect to play segment %d, but currently only segment %d available!",_expected_play_segment,ps.segment_id);
    
    
    
    
    play_media(ps);
}

void VLCDashHttpClient::change_requested_bandwidth()
{
    int bitrate=(int)(8.0*_total_download_size/_total_download_time);
    
    Log(DASH_LOG_DEBUG,"size %d, time %lf",_total_download_size,_total_download_time);
    
    _current_requested_bandwidth=best_matching_bandwidth(bitrate);
    
    
    DownloadSegment ds=_bitrate_history.back();
    if(_download_bitrate_ptr)
    {
        fprintf(_download_bitrate_ptr,"%f %f %f %d\n",NOW,ds.time,ds.bitrate,_current_requested_bandwidth/1000);
    }
}

void VLCDashHttpClient::timeout_play()
{
    /*decide if continue to download*/
  /* if(_current_requested_segment_id>=0 && _current_requested_segment_id-_expected_play_segment<3)
   {
        _download_timer.resched(0);
   }
  */
    
     if(_current_requested_segment_id>=0 && _cache_queue.size()<4)
    {
        _download_timer.resched(0.0);
    }
    
    
     if(_cache_queue.empty())
    {  
        if(_expected_play_segment>=_total_segments)
        {
           Log(DASH_LOG_NOTE,"played all segments!");
           stop();
           return;
        }
        else
        {
        Log(DASH_LOG_WARNING,"should play segment %d, but nothing to play from buffer!",_expected_play_segment);
        _qosstat.total_freeze++;
        _play_freeze=true;
        _start_freeze_time=now();
        return;
        }
    }
     
     
     /*only start playing if  media content of at least minbuffertime have been downloaded*/
  //  unsigned int min_seg=_minbuffertime/_segment_duration;
  //  if(_minbuffertime%_segment_duration)
   //     min_seg++;
   //  if(_expected_play_segment==0 && _cache_queue.size()<min_seg)
     //    return;
     /*only start playing if the downloaded segment number is no less than the specified value*/
     if(_expected_play_segment==0 && _cache_queue.size()<_play_after)
        return;
    
    
    /*only start playing if the previous segment has finished playing*/
    if(_play_timer.status()==TIMER_PENDING)
        return;
    
    
    PlaySegment ps=_cache_queue.front();
    _cache_queue.pop();
    
    if(ps.segment_id!=_expected_play_segment)
    
        Log(DASH_LOG_WARNING,"expect to play segment %d, but currently only segment %d available!",_expected_play_segment,ps.segment_id);
    
    
    
    
    play_media(ps);
}

void AndroidDashHttpClient::timeout_play()
{
    /*decide if continue to download*/
   /* if(_current_requested_segment_id>=0 && _current_requested_segment_id-_expected_play_segment<3)
    {
        _download_timer.resched(0.0);
    }
   */
    
     if(_current_requested_segment_id>=0 && _cache_queue.size()<4)
    {
        _download_timer.resched(0.0);
    }
    
    
     if(_cache_queue.empty())
    {  
        if(_expected_play_segment>=_total_segments)
        {
           Log(DASH_LOG_NOTE,"played all segments!");
           stop();
           return;
        }
        else
        {
        Log(DASH_LOG_WARNING,"should play segment %d, but nothing to play from buffer!",_expected_play_segment);
        _qosstat.total_freeze++;
        _play_freeze=true;
        _start_freeze_time=now();
        return;
        }
    }
     
     
    /* the 15040 Bytes requirements for initial buffer are always satisfied*/
    
    /*only start playing if the previous segment has finished playing*/
    if(_play_timer.status()==TIMER_PENDING)
        return;
     
     /*only start playing if the downloaded segment number is no less than the specified value*/
     if(_expected_play_segment==0 && _cache_queue.size()<_play_after)
        return;
    
    
    PlaySegment ps=_cache_queue.front();
    _cache_queue.pop();
    
    if(ps.segment_id!=_expected_play_segment)
    
        Log(DASH_LOG_WARNING,"expect to play segment %d, but currently only segment %d available!",_expected_play_segment,ps.segment_id);
    
    
    
    
    play_media(ps);
}




void AndroidDashHttpClient::change_requested_bandwidth()
{   
    
    if(_bitrate_history.empty())
    {
        return;
    }
    
    list<DownloadSegment>::iterator it;
    long long total_size=0;
    double total_time=0.0;
    for(it=_bitrate_history.begin();it!=_bitrate_history.end();it++)
    {
        total_size+=(*it).size;
        total_time+=(*it).time;
    }
    
    //int bitrate=(int)(0.8*8*total_size/total_time);
    
    double bitrate=8*total_size/total_time;
    
    //fprintf(stderr,"in android client, total size %lld, time %f,bitrate %f\n",8*total_size,total_time,bitrate);
    
    _current_requested_bandwidth=best_matching_bandwidth((int)bitrate);
    
    
    DownloadSegment ds=_bitrate_history.back();
    if(_download_bitrate_ptr)
    {
        fprintf(_download_bitrate_ptr,"%f %f %f %d\n",NOW,ds.time,ds.bitrate,_current_requested_bandwidth/1000);
    }
    
       
}


void SAndroidDashHttpClient::change_requested_bandwidth()
{   
    
    if(_bitrate_history.empty())
    {
        return;
    }
    
    list<DownloadSegment>::iterator it;
    long long total_size=0;
    double total_time=0.0;
    for(it=_bitrate_history.begin();it!=_bitrate_history.end();it++)
    {
        total_size+=(*it).size;
        total_time+=(*it).time;
    }
    
    //int bitrate=(int)(0.8*8*total_size/total_time);
    
    double bitrate=0.8*8*total_size/total_time;
    
    //fprintf(stderr,"in android client, total size %lld, time %f,bitrate %f\n",8*total_size,total_time,bitrate);
    
    _current_requested_bandwidth=best_matching_bandwidth((int)bitrate);
    
    
    DownloadSegment ds=_bitrate_history.back();
    if(_download_bitrate_ptr)
    {
        fprintf(_download_bitrate_ptr,"%f %f %f %d\n",NOW,ds.time,ds.bitrate,_current_requested_bandwidth/1000);
    }
    
       
}

void AIMDHttpClient::change_requested_bandwidth()
{
    /*calculate sigma only once*/
    if(!_sigma_calculated)
    {
        
        if(_bwqueue.size()<2)
        {
            Log(DASH_LOG_WARNING,"less than two bandwidth level in this streaming session, no bitrate switch needed");
            _sigma_calculated=false;
            _sigma=-1.0;
            return;
        }
        
        unsigned int i;
        
        float prev_bandwidth=(float)_bwqueue.at(0).bandwidth;
        
        for(i=1;i<_bwqueue.size();i++)
        {
            if(_bwqueue.at(i).bandwidth/prev_bandwidth>_sigma)
                _sigma=_bwqueue.at(i).bandwidth/prev_bandwidth;
            
            prev_bandwidth=(float)_bwqueue.at(i).bandwidth;
        }
        
        
        _sigma_calculated=true;
        
        Log(DASH_LOG_DEBUG,"calculated sigma is %f",_sigma);
    }
    
    /*no history records or only one bandwidth version available*/
     if(_bitrate_history.empty() || _sigma<0.0)
        return;
    
    
    DownloadSegment ds=_bitrate_history.back();
    int bitrate;
    float miu=_segment_duration/ds.time;
    
    if(miu>=_sigma)
        bitrate=next_higher_bandwidth(_current_requested_bandwidth);
    else if(miu<=_lambda)
        bitrate=_current_requested_bandwidth*miu;
    else
        bitrate=_current_requested_bandwidth;
    
    _current_requested_bandwidth=best_matching_bandwidth(bitrate);
    
    
    
    if(_download_bitrate_ptr)
    {
        fprintf(_download_bitrate_ptr,"%f %f %f %d\n",NOW,ds.time,ds.bitrate,_current_requested_bandwidth/1000);
    }
    
}



void AIMDHttpClient::timeout_play()
{
    /*decide if continue to download*/
    
    float idle_time=_cache_queue.size()*_segment_duration-(float)_min_buffer-(float)(_segment_duration*_current_requested_bandwidth)/(float)(_bwqueue.at(0).bandwidth);
    
    Log(DASH_LOG_DEBUG,"idle time is %f",idle_time);
    
    if(_current_requested_segment_id>=0 && idle_time<=0.0)
    {
        _download_timer.resched(0);
    }
    
    
     if(_cache_queue.empty())
    {  
        if(_expected_play_segment>=_total_segments)
        {
           Log(DASH_LOG_NOTE,"played all segments!");
           stop();
           return;
        }
        else
        {
        Log(DASH_LOG_WARNING,"should play segment %d, but nothing to play from buffer!",_expected_play_segment);
        _qosstat.total_freeze++;
        _play_freeze=true;
        _start_freeze_time=now();
        return;
        }
    }
     
     
     /*only start playing if cache buffer is larger than min buffer*/
     /*start playing as soon as possible*/
    // if(_expected_play_segment==0 && _cache_queue.size()*_segment_duration<_min_buffer)
     //    return;
    /*only start playing if the downloaded segment number is no less than the specified value*/
     if(_expected_play_segment==0 && _cache_queue.size()<_play_after)
        return;
    
    
    
    /*only start playing if the previous segment has finished playing*/
    if(_play_timer.status()==TIMER_PENDING)
        return;
    
    
    PlaySegment ps=_cache_queue.front();
    _cache_queue.pop();
    
    if(ps.segment_id!=_expected_play_segment)
    
        Log(DASH_LOG_WARNING,"expect to play segment %d, but currently only segment %d available!",_expected_play_segment,ps.segment_id);
    
    
    
    
    play_media(ps);
}




int DashHttpClient::best_matching_bandwidth(int bandwidth)
{
    int size=_bwqueue.size();
    int candidate_bandwidth=0;
    
    int i=0;
    
    while(i<size)
    {
    if(_bwqueue.at(i).bandwidth<=bandwidth && _bwqueue.at(i).bandwidth>candidate_bandwidth)
    {
        candidate_bandwidth=_bwqueue.at(i).bandwidth;
        i++;
    }
    else
        break;
    }
    
    
    /*if no suitable bandwidth is found, select the first one*/
    if(candidate_bandwidth==0)
        candidate_bandwidth=_bwqueue.at(0).bandwidth;
    
    
    if(_bitrate_lower_bound>0 && candidate_bandwidth<_bitrate_lower_bound)
        candidate_bandwidth=higher_bandwidth(_bitrate_lower_bound);
    if(_bitrate_upper_bound>0 && candidate_bandwidth>_bitrate_upper_bound)
        candidate_bandwidth=lower_bandwidth(_bitrate_upper_bound);
        
        
    
    
    Log(DASH_LOG_DEBUG,"base bw: %d, requested bandwidth is %d", bandwidth,candidate_bandwidth);
    
    
    if(bandwidth<=0)
    {
        fprintf(stderr,"Error! Calculated bandwidth is negative!\n");
        exit(-1);
    }
    
    return candidate_bandwidth;
    
}


/*input bandwidth is one element of the bandwidth queue*/
int DashHttpClient::next_higher_bandwidth(int bandwidth)
{
    
    
    int size=_bwqueue.size();
   
    
    
    
    int i;
    for(i=0;i<size;i++)
    { 
        if(_bwqueue.at(i).bandwidth==bandwidth)
            break;
    }
   
    if(i==size)
        return -1;
    else if(i==size-1)
        return bandwidth;
    else
        return _bwqueue.at(i+1).bandwidth;
}

int DashHttpClient::higher_bandwidth(int lower_bound)
{
    int size=_bwqueue.size();
    int i;
    for(i=0;i<size;i++)
    {
        if(_bwqueue.at(i).bandwidth>=lower_bound)
            return _bwqueue.at(i).bandwidth;
    }
    
    /*no bandwidth reaches this limit, so send the largest one*/
    return _bwqueue.at(size-1).bandwidth;
}


int DashHttpClient::lower_bandwidth(int higher_bound)
{
    int size=_bwqueue.size();
    int i;
    for(i=size-1;i>=0;i--)
    {
        if(_bwqueue.at(i).bandwidth<=higher_bound)
            return _bwqueue.at(i).bandwidth;
    }
    
    /*send the smallest one*/
    return _bwqueue.at(0).bandwidth;
}




void WEMAHttpClient::change_requested_bandwidth()
{ 
    
    if(_bitrate_history.empty())
        return;
    
    
    DownloadSegment ds=_bitrate_history.back();
    
    int bitrate=(int)(8*ds.size/ds.time);
    
    if(_smoothed_rate==0)
        _smoothed_rate=bitrate;
    else
        _smoothed_rate=(int)(_alpha*_smoothed_rate+(1-_alpha)*bitrate);
    
    _current_requested_bandwidth=best_matching_bandwidth(bitrate);
    
     if(_download_bitrate_ptr)
    {
        fprintf(_download_bitrate_ptr,"%f %f %f %d\n",NOW,ds.time,bitrate/1000.0,_current_requested_bandwidth/1000);
    }
    
}

void WEMAHttpClient::timeout_play()
{
    /*decide if continue to download*/
 /*   if(_current_requested_segment_id>=0 && _current_requested_segment_id-_expected_play_segment<3)
    {
        _download_timer.resched(0);
    }
 */
    
    if(_current_requested_segment_id>=0 && _cache_queue.size()<4)
    {
        _download_timer.resched(0.0);
    }
    
    
    
     if(_cache_queue.empty())
    {  
        if(_expected_play_segment>=_total_segments)
        {
           Log(DASH_LOG_NOTE,"played all segments!");
           stop();
           return;
        }
        else
        {
        Log(DASH_LOG_WARNING,"should play segment %d, but nothing to play from buffer!",_expected_play_segment);
        _qosstat.total_freeze++;
        _play_freeze=true;
        _start_freeze_time=now();
        return;
        }
    }
     
     
     /*only start playing if two segments have been downloaded*/
   //  if(_expected_play_segment==0 && _cache_queue.size()<2)
       //  return;
    
    /*only start playing if the downloaded segment number is no less than the specified value*/
     if(_expected_play_segment==0 && _cache_queue.size()<_play_after)
        return;
    
    
    /*only start playing if the previous segment has finished playing*/
    if(_play_timer.status()==TIMER_PENDING)
        return;
    
    
    PlaySegment ps=_cache_queue.front();
    _cache_queue.pop();
    
    if(ps.segment_id!=_expected_play_segment)
    
        Log(DASH_LOG_WARNING,"expect to play segment %d, but currently only segment %d available!",_expected_play_segment,ps.segment_id);
    
    
    
    
    play_media(ps);
}





void VAndroidDashHttpClient::timeout_play()
{
    /*decide if continue to download*/
   /* if(_current_requested_segment_id>=0 && _current_requested_segment_id-_expected_play_segment<3)
    {
        _download_timer.resched(0.0);
    }
   */
    
     if(_current_requested_segment_id>=0 && _cache_queue.size()<4)
    {
        _download_timer.resched(0.0);
    }
    
    
     if(_cache_queue.empty())
    {  
        if(_expected_play_segment>=_total_segments)
        {
           Log(DASH_LOG_NOTE,"played all segments!");
           stop();
           return;
        }
        else
        {
        Log(DASH_LOG_WARNING,"should play segment %d, but nothing to play from buffer!",_expected_play_segment);
        _qosstat.total_freeze++;
        _play_freeze=true;
        _start_freeze_time=now();
        return;
        }
    }
     
    
    /* the 15040 Bytes requirements for initial buffer are always satisfied*/
    
    /*only start playing if the previous segment has finished playing*/
    if(_play_timer.status()==TIMER_PENDING)
        return;
     
     /*only start playing if the downloaded segment number is no less than the specified value*/
     if(_expected_play_segment==0 && _cache_queue.size()<_play_after)
        return;
    
    
    PlaySegment ps=_cache_queue.front();
    _cache_queue.pop();
    
    if(ps.segment_id!=_expected_play_segment)
    
        Log(DASH_LOG_WARNING,"expect to play segment %d, but currently only segment %d available!",_expected_play_segment,ps.segment_id);
    
    
    
    
    play_media(ps);
}




void VAndroidDashHttpClient::change_requested_bandwidth()
{   
    
    if(_bitrate_history.empty())
    {
        return;
    }
    
    list<DownloadSegment>::iterator it;
    long long total_size=0;
    double total_time=0.0;
    for(it=_bitrate_history.begin();it!=_bitrate_history.end();it++)
    {
        total_size+=(*it).size;
        total_time+=(*it).time;
    }
    
    //int bitrate=(int)(0.8*8*total_size/total_time);
    
    double bitrate=8*total_size/total_time;
    
    //fprintf(stderr,"in android client, total size %lld, time %f,bitrate %f\n",8*total_size,total_time,bitrate);
    
    _current_requested_bandwidth=best_matching_bandwidth((int)bitrate);
    
    
    DownloadSegment ds=_bitrate_history.back();
    if(_download_bitrate_ptr)
    {
        fprintf(_download_bitrate_ptr,"%f %f %f %d\n",NOW,ds.time,ds.bitrate,_current_requested_bandwidth/1000);
    }
    
       
}
