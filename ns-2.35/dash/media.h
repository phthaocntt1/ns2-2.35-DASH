#ifndef ns_media_h
#define ns_media_h



#include "ns-process.h"
#include "app.h"
#include <queue>
#include <vector>
#include <stdio.h>
using namespace std;

const int TS_PACKET_SIZE=188;
const int MAX_TS_NUMBER=7;
const int MAX_TS_PAYLOAD=TS_PACKET_SIZE*MAX_TS_NUMBER;

enum HttpHeaderType
{
    HTTP_REQUEST=1,
    HTTP_RESPONSE
};


typedef struct hdr_http
{
    enum HttpHeaderType type;
    char http_url[50];
    
}hdr_http;



enum DashDataType{
    PT_TM3U8_META=100,
    PT_TM3U8_ITEM=101,
    PT_TMEDIA_DATA=102,
    PT_TREQUEST=103
};


enum RequestTrend{
    RTREND_UP,
    RTREND_DOWN,
    RTREND_STABLE
};



struct _BWItem
{
    int quality_id;
    int bandwidth;
};

struct _BWReqHistory{
    
    int segment_id;
    int bandwidth;
    double time;
};

struct _SegTransHistory{
    double start;
    double end;
    double duration;
    double bitrate;
};

typedef struct _BWReqHistory BWReqHistory;
typedef struct _SegTransHistory SegTransHistory;

class VideoDescription
{
public:
    
    
    char _name[100];
    int _program_id;
    int _total_duration;
    int _segment_duration;
    std::vector<_BWItem> _bwqueue;
    
    
public:
    VideoDescription();
    VideoDescription(int program_id,char* name,int total_duration,int segment_duration,int *bw_queue,int total_bw);
   ~VideoDescription();
    
    
};

class DashClientPlay
{
public:
    int _client_addr;
    
    
    char _media_name[100];
    int _media_id;
    int _total_duration;
    int _segment_duration;
    int _total_segments;
    std::vector<_BWItem> _bwqueue;
    bool _streaming_started;
    
    int _max_bw;
    
    
    int _max_bandwidth_version;
    int _current_requested_segment_id;
    int _current_received_segment_id;
    int _current_received_ts_id;
    std::vector<BWReqHistory> _request_bandwidth_history;
    std::vector<BWReqHistory> _original_request_bandwidth_history;
    std::vector<SegTransHistory> _seg_trans_history;
    std::vector<int> _estimated_max_bw;
    
    int _current_transmitted_segment_id;
    int _current_transmitted_ts_id;
    int _transmitted_segment_size;
    int _current_segment_total_size;
    double _seg_trans_start_time;
    double _seg_received_ratio;
    
    enum RequestTrend _request_trend;
    
    FILE *_trend_file;
    
    
    
public:
    
    DashClientPlay(int addr,char *media);
    ~DashClientPlay();
    int find_next_bandwidth(int bandwidth, int direction);
    
    int find_lower_bandwidth(int bandwidth);
    
    int find_closest_bandwidth(int bandwidth);
    
    void update_request_trend();
    
    double check_request_stability();
   
    double check_original_request_stability();
    
    double check_util(int intended_bw);
    
    int check_util_range(int lower_bw, int higher_bw);
    
    int get_estimated_bw_dev();
    
    
    int find_bw_index(int bw);
    
    int min3(int a, int b, int c);
    int max3(int a, int b, int c);
    
};







/*If this base class has instance, it is for dash request*/
class DashAppData: public AppData
{
    
    
public:
    enum DashDataType _dtype;
    hdr_http http_header;
    
public:
    DashAppData(enum DashDataType dtype):AppData(MEDIA_DATA),_dtype(dtype){}
    DashAppData(DashAppData &data):AppData(MEDIA_DATA){ *this=data;}
    ~DashAppData(){}
    enum DashDataType getDashType(){return _dtype;}
    virtual int size() const {return sizeof(DashAppData);}
    virtual AppData* copy()
    {
        DashAppData *data=new DashAppData(*this);
        assert(data);
        return (AppData*)data;
    }
};


/*As a response from the server to the client,
 a negative _program_id indicates that the
 requested resource is not present on the server,
 and _bwqueue is empty thereafter*/
class M3U8Meta: public DashAppData
{
public:
    
    int _program_id;
    _BWItem _bwqueue[50];
    
public:
    M3U8Meta(VideoDescription &vd):DashAppData(PT_TM3U8_META)
    {
        _program_id=vd._program_id;
        
        memset(_bwqueue,0,50*sizeof(_BWItem));
        
        unsigned int i;
        for(i=0;i<vd._bwqueue.size();i++)
        {
            
            if(i>=50)
            break;
          _bwqueue[i]=vd._bwqueue.at(i);
        }
    }  
    
    M3U8Meta(M3U8Meta &data):DashAppData(PT_TM3U8_META){*this=data;}
    ~M3U8Meta()
    {
       
    }
    virtual int size() const {return sizeof(M3U8Meta);}
    virtual AppData* copy()
    {
        M3U8Meta *data=new M3U8Meta(*this);
        assert(data);
        return (AppData*)data;
    }
    
    
};

class M3U8Item: public DashAppData
{
public:
    
    
    int _segment_duration;
    int _total_duration;
    
    /*only _bandwidth should vary for different m3u8 items*/
    int _bandwidth; 
    
    
public:
    M3U8Item(VideoDescription &vd):DashAppData(PT_TM3U8_ITEM)
    {
        _segment_duration=vd._segment_duration;
        _total_duration=vd._total_duration;
        
        /*bandwidth is not important here*/
        _bandwidth=0;
    }
    M3U8Item(M3U8Item &data):DashAppData(PT_TM3U8_ITEM){*this=data;}
    ~M3U8Item(){};
    virtual int size() const {return sizeof(M3U8Item);}
    virtual AppData* copy()
    {
        M3U8Item *data=new M3U8Item(*this);
        assert(data);
        return (AppData*)data;
    }
    
};


/*dash media data should be delivered via tcp directly, so
 it doesn't have a http header.
 It should not inherit from DashAppData, but AppData directly.
 */
class DashMediaChunk: public AppData
{
public:
    enum DashDataType _dtype;
    int _media_packet_size;
    int _bandwidth;
    int _segment_id;
    int _tcp_segment_id;
    int _transport_stream_id;
    double _sent_time;
    
public:
    DashMediaChunk(VideoDescription &vd,int bandwidth,int segment_id);
    
    DashMediaChunk(DashMediaChunk &chunk):AppData(MEDIA_DATA){*this=chunk;}
    DashMediaChunk(DashMediaChunk &chunk,int updated_size):AppData(MEDIA_DATA)
    { 
        *this=chunk;
        
       
        _media_packet_size=updated_size-sizeof(DashMediaChunk);
        
        if(_media_packet_size<0)
            _media_packet_size=0;
    }
    DashMediaChunk(DashMediaChunk &chunk,int updated_size,int ts_id):AppData(MEDIA_DATA)
    { 
        *this=chunk;
        
       
        _media_packet_size=updated_size-sizeof(DashMediaChunk);
        _transport_stream_id=ts_id;
        
        if(_media_packet_size<0)
            _media_packet_size=0;
    }
    ~DashMediaChunk(){};
    
    virtual int size() const 
    {
        
        return sizeof(DashMediaChunk)+_media_packet_size;
    }
    virtual AppData* copy()
    {
        DashMediaChunk *chunk=new DashMediaChunk(*this);
        assert(chunk);
        return (AppData*) chunk;
    }
    
};


#endif