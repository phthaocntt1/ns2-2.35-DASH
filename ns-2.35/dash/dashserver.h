#ifndef ns_dashserver_h
#define ns_dashserver_h



#include "ns-process.h"
#include "app.h"
#include <list>
#include "media.h"

using namespace std;

/*The dash server will not record the status of the client,
 so it will reply to the client's request whenever possible
 *without considering logic correctness */

class DashHttpServer:public Application
{
public:
    DashHttpServer();
    ~DashHttpServer();
    void ReadVideoListFile(const char *file);
    void Log(int log_level,const char *fmt,...);
    
    void process_data(int size, AppData* data);
    virtual void recv(int nbytes,AppData *data);
    virtual void send(int nbytes, AppData *data);
    inline double now() { return Scheduler::instance().clock(); }
    
    void send_m3u8meta_response(VideoDescription &vd);
    void send_m3u8item_response(VideoDescription &vd);
    void send_media_data_response(VideoDescription &vd, int bandwidth, int segment_id);
    
    virtual int command(int argc, const char*const* argv);
	
    virtual void start(); 
    virtual void stop(); 
protected:
    list<VideoDescription> _video_list;
    int _total_program;
    
    int _default_log_level;
    
    
    
    AppData *_sent_data;
    int _mss;
    bool _started;
    
    
};

#endif