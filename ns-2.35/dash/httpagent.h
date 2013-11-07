#ifndef ns_httpagent_h
#define ns_httpagent_h

using namespace std;

#include "ns-process.h"
#include "app.h"
#include "tcp.h"
#include "tcp-full.h"
#include "rq.h"
#include "utils.h"
#include <queue>
#include <list>


typedef struct TCPBufferedData
{  

    
    int length;
    int start_seq;
    AppData *data;

    
    
}TCPBufferedData;

class DHttpAgent:public FullTcpAgent
{
    public:
        DHttpAgent():FullTcpAgent(),_is_retransmit(false){};
        ~DHttpAgent(){};
        
        
        virtual void recv(Packet *pkt, Handler*);
        virtual void recvBytes(int datalen,AppData *data);
        virtual void sendmsg(int sz, AppData* data, const char*flags);
        virtual void sendpacket(int seq, int ack, int flags, int dlen, int why, Packet *p=0);
        virtual int foutput(int seqno, int reason = 0); // output 1 packet
        void Log(int log_level,const char *fmt,...);
        virtual int command(int argc, const char*const* argv);
        virtual void newack(Packet *pkt);
        virtual void finish();
        
        virtual int getfid(){return fid_;}
        
        void removeAckedPacket(int old_ack_no, int new_ack_no);
        void addtoReceiveQueue(int datalen,int seq_no,AppData *data);
        
        void findReceivedData(int start_seq,int end_seq);
       
        AppData *getRetransmitData(int seq_no, int datalen); 
        /*get a packet to send from the retransmit queue, 
         * which holds packets sent at least once,
         but not acknowledged yet*/
        
        
    protected:
        list<TCPBufferedData> _recvq;
        list<AppData*> _send_applicationq; /*buffer for data from application*/
        list<TCPBufferedData> _send_waitingq; /*sent at least once, waiting for ack*/
        bool _is_retransmit;
        int _default_log_level;
        
        
            
};




#endif