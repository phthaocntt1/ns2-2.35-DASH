#ifndef ns_dashll_h
#define ns_dashll_h

#include <delay.h>
#include <queue.h>
#include <arp.h>
#include <classifier.h>
#include <lanRouter.h>
#include <varp.h>
#include <list>
#include <set>

#include "node.h"
#include "channel.h"
#include "phy.h"
#include "queue.h"
#include "net-interface.h"
#include "timer-handler.h"
#include "mac-802_11mr.h"
#include "dash/media.h"
#include "dash/utils.h"

typedef enum{
    /*simplest*/
    SP_FIFO,
            
    /*packet fairness*/
    SP_ROUND_ROBIN,
            
    /*approach bit fairness*/        
    SP_DEFICIT_ROUND_ROBIN,
            
    /*approach airtime fairness*/
    SP_AIRTIME_ROUND_ROBIN,
            
    /*worst case weighted fair queuing*/
    SP_WORST_CASE_WEIGHTED_FAIR,
            
            
            
    /*Last one*/
    SP_UNKNOWN_POLICY
    
}SCHEDULING_POLICY;

struct _ClientBW{
    double _current_bitrate;
    double _smoothed_bitrate;
    double _trend;
};

typedef struct _ClientBW ClientBW;

class DashLL : public LL {
    public:
        DashLL();
        ~DashLL();
        
        virtual void recv(Packet* p, Handler* h);
        void Log(int log_level,const char *fmt,...);
        virtual void process_data(AppData *data,int src_addr,int dst_addr);
        void process_media_data(DashMediaChunk *chunk,int src_addr, int dst_addr);
        void process_transmitted_media_data(DashMediaChunk *chunk,int dst_addr);
        void process_m3u8meta_data(M3U8Meta *data,int src_addr,int dst_addr);
        void process_m3u8item_data(M3U8Item *data,int src_addr,int dst_addr);
        void process_request(DashAppData *data, int src_addr,int dst_addr);
        
        /*change requested bandwidth, if necessary*/
        int change_requested_bandwidth(DashClientPlay* client_db,int bandwidth);
        int get_underlying_bandwidth(DashClientPlay *client_db);
        void update_underlying_bandwidth(int client_addr);
        void update_underlying_queue_weight(int client_addr);
        void update_client_number(int client_addr, bool add);
        
        void update_recv(Packet *p,int dst);
       
        
        
        virtual Packet *select_packet();
        PeerStatsDB *getPeerStatsDB(){return _peerstatsdb;}
        
        
    protected:
        list<DashClientPlay*> _dash_clients;
        set<int> _client_set;
        int _default_log_level;
        Mac802_11mr *_mac80211mr;
        bool _mac80211mr_initialized;
        PeerStatsDB*         _peerstatsdb; 
        SCHEDULING_POLICY  _spolicy;
        
        
       
        ClientBW *_client_bw_table;
        int _max_client_number;
        
        
       
        
       
};










#endif