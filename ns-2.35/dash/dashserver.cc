#include <list>
#include <stdarg.h>
#include <fstream>
#include <iostream>
#include "dashserver.h"
#include "dashclient.h"

using namespace std;


static class DashHttpServerAppClass : public TclClass {
 public:
	DashHttpServerAppClass() : TclClass("Application/HTTP/DashServer") {}
	TclObject* create(int, const char*const*) {
		return (new DashHttpServer);
	}
} class_app_dashhttpserver;





DashHttpServer::DashHttpServer()
:_total_program(0),_default_log_level(DASH_LOG_NOTE),
        _sent_data(NULL),_mss(0),_started(false)
{
    
}
DashHttpServer::~DashHttpServer()
{
    while(!_video_list.empty())
        _video_list.pop_back();
}

/*The description file has the following format:
     name:stream-name-1
     total_duration:
     segment_duration:
     bandwidth-1
     bandwidth-2
     ...
     bandwidth-n
     
     name:stream-name-2
     total_duration:
     segment_duration:
     bandwidth-1
     bandwidth-2
     ...
     bandwidth-n
     
     ...
     */
void DashHttpServer::ReadVideoListFile(const char* file)
{
    
    assert(file!=NULL);

    ifstream ifs(file,ifstream::in);
    if(ifs.fail())
    {
        Log(DASH_LOG_FATAL,"Cannot open file %s",file);
        exit(0);
        
    }
    
   
    //at most 50 bandwidth levels are assumed
    int bandwidth_level[50];
    int bandwidth_index=0;
    char video_name[200];
    int total_duration;
    int segment_duration;
    char read_buffer[500];
    
    
    memset(bandwidth_level,0,sizeof(int)*50);
    memset(read_buffer,0,500);
    
    
    const char *name_needle="name:";
    const char *total_duration_needle="total_duration:";
    const char *segment_duration_needle="segment_duration:";
    const char *bandwidth_needle="bandwidth:";
    bool new_item_start=false;
    
    
    
    while(ifs.good())
    {
        
        
       
        
        ifs.getline(read_buffer,500);
        
        char *p;
       
        
       if((p=strstr(read_buffer,name_needle))!=NULL)
       {
           p=p+strlen(name_needle);
           strcpy(video_name,p);
           
           new_item_start=true;
           
       }
       else if((p=strstr(read_buffer,total_duration_needle))!=NULL)
       {
           p=p+strlen(total_duration_needle);
           total_duration=atoi(p);
       }
       else if((p=strstr(read_buffer,segment_duration_needle))!=NULL)
       {
           p=p+strlen(segment_duration_needle);
           segment_duration=atoi(p);
       }
       else if((p=strstr(read_buffer,bandwidth_needle))!=NULL)
       {
           p=p+strlen(bandwidth_needle); 
           bandwidth_level[bandwidth_index++]=atoi(p);
       }
       else
       {
           /*this means the description for one stream has been finished, and
            we can add its parameters to the system*/
           
          if(new_item_start)
          {
           VideoDescription vd(_total_program++,video_name,total_duration,segment_duration,bandwidth_level,bandwidth_index);
           _video_list.push_back(vd);
           bandwidth_index=0;
           
          new_item_start=false;
          }
       }
    }
    
    
    ifs.close();
}

void DashHttpServer::process_data(int size, AppData* data)
{
    
    
     /*it must be a DashAppData*/
    
    DashAppData *appdata=dynamic_cast<DashAppData*>(data);
    if(!appdata)
    {
        
        Log(DASH_LOG_FATAL,"cannot convert data to dash app data type!");
        return;
    }
    
    /*check http header first*/
    if(appdata->http_header.type!=HTTP_REQUEST || appdata->_dtype!=PT_TREQUEST)
    {
        /*server does not respond to http response*/
        Log(DASH_LOG_WARNING,"received http response and ignored!");
        return;
    }
    
    /*parse the requested url*/
    char resource_name[50];
    char resource_item[10];
    int resource_id=-1;
    memset(resource_name,0,50);
    memset(resource_item,0,10);
    
    sscanf(appdata->http_header.http_url,"%s %s %d",resource_name,resource_item,&resource_id);
    
    /*first check if the requested resource is on the server*/
    
    VideoDescription vd;
    list<VideoDescription>::iterator it;
    for(it=_video_list.begin();it!=_video_list.end();it++)
    {
        if(strcmp((*it)._name,resource_name)==0)
        {
            vd=*it;
            break;
        }
        
    }
    
    if(vd._program_id<0)
        Log(DASH_LOG_WARNING,"requested resource: %s is not found!",resource_name);
    
    Log(DASH_LOG_NOTE,"received request for resource: %s",appdata->http_header.http_url);
    
    if(!strcmp(resource_item,"meta"))
    {
        send_m3u8meta_response(vd);
        return;
    }
    else if(!strcmp(resource_item,"item"))
    {
        send_m3u8item_response(vd);
        return;
        
    }
    else if(atoi(resource_item)>0 && resource_id>=0)
    {
        send_media_data_response(vd,atoi(resource_item),resource_id);
    }
    else
    
        Log(DASH_LOG_ERROR,"received invalid request: %s",appdata->http_header.http_url);
        
    
    
    
    
}

void DashHttpServer::send_m3u8item_response(VideoDescription &vd)
{
    
    assert(_sent_data==NULL);
    _sent_data=new M3U8Item(vd);
    M3U8Item *item=(M3U8Item*)(_sent_data);
    item->http_header.type=HTTP_RESPONSE;
    
    send(item->size(),_sent_data);
    
    delete _sent_data;
    _sent_data=NULL;
}

void DashHttpServer::send_m3u8meta_response(VideoDescription &vd)
{   
    assert(_sent_data==NULL);
    _sent_data=new M3U8Meta(vd);
    M3U8Meta *meta=(M3U8Meta*)(_sent_data);
    meta->http_header.type=HTTP_RESPONSE;
    
    send(meta->size(),_sent_data);
    
    delete _sent_data;
    _sent_data=NULL;
    
    
}

void DashHttpServer::send_media_data_response(VideoDescription& vd, int bandwidth, int segment_id)
{  
    
    assert(_sent_data=NULL);
    _sent_data=new DashMediaChunk(vd,bandwidth,segment_id);
    
    DashMediaChunk *chunk=(DashMediaChunk*)(_sent_data);
    chunk->_sent_time=now();
    chunk->_tcp_segment_id=0; /*set at TCP layer*/
    chunk->_transport_stream_id=0;/*set transport stream id*/
    
    //Log(DASH_LOG_DEBUG,"sending bandwidth %d, segment %d, total media size %d",bandwidth,segment_id,chunk->_media_packet_size);
    send(chunk->size(),_sent_data);
    
    delete _sent_data;
    _sent_data=NULL;
    
}

 /*we assume a stateless server will not exit on fatal error or warning*/
void DashHttpServer::Log(int log_level, const char* fmt, ...)
{
    
    va_list arg_list;
    va_start(arg_list,fmt); 
    
    DashLog(stdout,log_level,_default_log_level,DASH_COLOR_YELLOW,"DASH-SERVER", fmt,arg_list);
    va_end(arg_list);
}


void DashHttpServer::send(int nbytes, AppData* data)
{
    
    DashMediaChunk *chunk=dynamic_cast<DashMediaChunk*>(data);
    if(chunk==NULL)
    {
    agent_->sendmsg(nbytes,data,NULL);
    return;
    }
    
    AppData *new_data;
    
    assert(_mss>sizeof(DashMediaChunk));
    
    int packet_number=chunk->_media_packet_size/(_mss-sizeof(DashMediaChunk));
    int start_ts_seq=0;
    
   // Log(DASH_LOG_DEBUG,"media size %d,  segmented number: %d %d %d",chunk->_media_packet_size,packet_number,_mss,sizeof(DashMediaChunk));
    while(packet_number)
    {
        new_data=new DashMediaChunk(*chunk,_mss,start_ts_seq);
        
        start_ts_seq++;      
        agent_->sendmsg(new_data->size(),new_data,NULL);
        delete new_data;
        new_data=NULL;
        packet_number--;
    }
    int remain_bytes=chunk->_media_packet_size%(_mss-sizeof(DashMediaChunk));
    if(remain_bytes>0)
    {
        new_data=new DashMediaChunk(*chunk,remain_bytes+sizeof(DashMediaChunk),start_ts_seq);
        agent_->sendmsg(new_data->size(),new_data,NULL);
        delete new_data;
        new_data=NULL;
    }
    
    return;
       
}


void DashHttpServer::recv(int nbytes, AppData* data)
{
    
    if(!_started)
        start();
    
    /*sanity check*/
    if(nbytes==0 || data==NULL)
    {
        Log(DASH_LOG_FATAL,"received data has nothing inside!");
        return;
        
    }
    
    process_data(nbytes,data);
}


int DashHttpServer::command(int argc, const char* const *argv)
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
    else if(strcmp(argv[1],"read-video-list")==0){
        
        ReadVideoListFile(argv[2]);
        return(TCL_OK);
    }
  }

  return(Application::command(argc, argv));
}

void DashHttpServer::start()
{
    /*sanity check*/
    if(agent_==NULL)
    {
        Log(DASH_LOG_FATAL,"transport agent is not attached!");
        exit(0);
    }
    
    _mss=agent_->size();
    
    if(_mss<=0)
    {
        Log(DASH_LOG_FATAL,"lower layer has non-positive mss sizse!");
        exit(0);
    }
    
    Log(DASH_LOG_DEBUG,"_mss is %d",_mss);
    
    if(_video_list.empty() || _total_program==0)
    {
        Log(DASH_LOG_FATAL,"no media stream found on server!");
        exit(0);
    }
    
    
    
    Log(DASH_LOG_NOTE,"waiting for requests from clients!");
    
    _started=true;
    
}

void DashHttpServer::stop()
{
    
}