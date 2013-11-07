#ifndef ns_dashclient_h
#define ns_dashclient_h



#include <queue>
#include <vector>
#include <stack>
#include "ns-process.h"
#include "app.h"
#include "media.h"
#include "utils.h"

using namespace std;


enum client_state
{
    DASH_INITIAL=0,
    DASH_M3U8META_REQUEST_SENT,
    DASH_M3U8META_RESPONSE_RECEIVED,
    DASH_M3U8ITEM_REQUEST_SENT,
    DASH_M3U8ITEM_REQUEST_RECEIVED,
    DASH_STREAM_REQUEST_SENT,
    DASH_STREAM_REQUEST_RECEIVED,
    DASH_FINISHED
};

typedef struct PlaySegment{
    int segment_id;
    int bandwidth;
}PlaySegment;

typedef struct DownloadSegment{
    int size;
    double time;
    double bitrate;
}DownloadSegment;


typedef struct QosStat{
    double request_start_time;
    double first_segment_received_time;
    double play_start_time;
    double play_finish_time;
    int total_bandwidth;
    int total_seg_played;
    int total_freeze;
    int quality_change;
    float avg_freeze_duration;
}QosStat;

class DashHttpClient;

class DashClientDownloadTimer : public TimerHandler {
 public:
	DashClientDownloadTimer(DashHttpClient* t) : TimerHandler(), t_(t) {}
	virtual void expire(Event* e);
 protected:
	DashHttpClient* t_;
};

class DashClientPlayTimer : public TimerHandler {
 public:
	DashClientPlayTimer(DashHttpClient* t) : TimerHandler(), t_(t) {}
	virtual void expire(Event* e);
 protected:
	DashHttpClient* t_;
};


/*basic http client
 always retrieves the lowest quality
 start playing once first segment received*/
class DashHttpClient: public Application
{
    public:
        DashHttpClient();
	~DashHttpClient();

	virtual void recv(int nbytes);
        virtual void recv(int nbytes,AppData *data);
	virtual void send(int nbytes, AppData *data);
        virtual void change_requested_bandwidth();
        void calculate_bandwidth();
        void send_media_data_request();
        void send_m3u8meta_request();
        void send_m3u8item_request();
        inline double now() { return Scheduler::instance().clock(); }
        virtual void timeout_download();
        virtual void timeout_play();
        virtual void play_media(PlaySegment &ps);
        virtual void printStats();
	
        int  best_matching_bandwidth(int bandwidth);
        int  next_higher_bandwidth(int bandwidth);
        int  higher_bandwidth(int lower_bound);
        int  lower_bandwidth(int higher_bound);
        void Log(int log_level,const char *fmt,...);
        void Log(FILE *stream,int log_level,const char *fmt,...);

	virtual void process_data(int size, AppData* data);
        void process_media_data(DashMediaChunk *chunk);
        void process_m3u8meta_data(M3U8Meta *data);
        void process_m3u8item_data(M3U8Item *data);
	virtual AppData* get_data(int&, AppData*) {
		// Not supported
		abort();
		return NULL;
	}

	// Do nothing when a connection is terminated
	virtual void resume();
        void incState();

protected:
	virtual int command(int argc, const char*const* argv);
	

	
	virtual void start(); 
	virtual void stop(); 

        char _client_name[50];
	int curbytes_;
        enum client_state _state;
        int _current_program_id;
        char _media_url[50];
        vector<_BWItem> _bwqueue;
        int _segment_duration;    /* duration of each segment*/
        int _total_segments;
        int _received_segment_id;
        
        int _current_requested_bandwidth;
        int _current_requested_segment_id;
        bool _current_request_sent;
        int _expected_ts_id;
        int _expected_play_segment;
        int _received_segment_size;
        
        
        /*set an upper and/or lower bound for the bitrate a client can receive*/
        int _bitrate_lower_bound;
        int _bitrate_upper_bound;
        
        DashAppData *_sent_data; /*client send requests only*/
        AppData *_received_data;
        
        queue<PlaySegment> _cache_queue;
        
        
        
        int _default_log_level;
        QosStat _qosstat;
        bool _play_freeze;
        double _start_freeze_time;
        int _prev_played_bandwidth;
        
        
        DashClientDownloadTimer _download_timer;
        DashClientPlayTimer _play_timer;
        
        
        /*download bitrate estimation*/
        double _segment_download_start;
        double _segment_download_end;
        int _total_download_size;
        double _total_download_time;
        
        list<DownloadSegment> _bitrate_history;
        
        FILE *_download_history_ptr;
        FILE *_download_bitrate_ptr;
        FILE *_play_history_ptr;
        FILE *_play_summary_ptr;
        int _addr;
        bool _addr_got;
        
        int _play_after;
        
        int MAX_STACK_SIZE;
        
        
};

class VLCHLSHttpClient :public DashHttpClient
{
public:
    VLCHLSHttpClient():DashHttpClient()
    {
        strcpy(_client_name,"VLCHLSClient");
    }
protected:
    void change_requested_bandwidth();
    void timeout_play();
};

/*weighted exponential moving average*/
class WEMAHttpClient :public DashHttpClient
{
public:
    WEMAHttpClient():DashHttpClient()
    {
        strcpy(_client_name,"WEMAClient");
        _alpha=0.8;
        _smoothed_rate=0;
    }
protected:
    void change_requested_bandwidth();
    void timeout_play();
    
    
protected:
    double _alpha;
    int _smoothed_rate;
};


class VLCDashHttpClient: public DashHttpClient
{
public:
    VLCDashHttpClient():DashHttpClient(),
    _minbuffertime(15){strcpy(_client_name,"VLDDashClient");}



protected:
    
    /*specified by DASH standard, a minbuffertime variable should be included*/
    int _minbuffertime;
    void change_requested_bandwidth();
    void timeout_play();
    
};

class AndroidDashHttpClient: public DashHttpClient
{
    
public:
    AndroidDashHttpClient():DashHttpClient(),
            _buffersize(15040){strcpy(_client_name,"AndroidClient");}
protected:
    int  _buffersize;
    void change_requested_bandwidth();
    void timeout_play();
    
};


class SAndroidDashHttpClient: public AndroidDashHttpClient
{
    
public:
    SAndroidDashHttpClient():AndroidDashHttpClient()
            {strcpy(_client_name,"SAndroidClient");}
protected:
    
    void change_requested_bandwidth();
    
    
};





class VAndroidDashHttpClient: public DashHttpClient
{
    
public:
    VAndroidDashHttpClient():DashHttpClient(),
            _buffersize(15040)
     {
        strcpy(_client_name,"VAndroidClient");
        MAX_STACK_SIZE=20;
     }
    
protected:
    int  _buffersize;
    void change_requested_bandwidth();
    void timeout_play();
    
};

/* an implementation of the Chenghao Liu's method,
 * described in his paper:
 Rate adaptation for adaptive HTTP streaming
 mmsys 11*/
class AIMDHttpClient: public DashHttpClient
{
public:
    AIMDHttpClient():DashHttpClient(),
    _lambda(0.67),_sigma(0.0),
    _sigma_calculated(false),_min_buffer(9){strcpy(_client_name,"AIMDClient");}
protected:
    float _lambda;
    float _sigma;
    bool _sigma_calculated;
    int _min_buffer;
    void change_requested_bandwidth();
    void timeout_play();
};

#endif

