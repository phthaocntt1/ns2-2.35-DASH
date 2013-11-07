#ifndef _dashpriqueue_h
#define _dashpriqueue_h


#include <list>
#include <iostream>
#include "object.h"
#include "queue.h"
#include "priqueue.h"
#include "drop-tail.h"
#include "packet.h"
#include "lib/bsd-list.h"




using namespace std;

typedef enum{
    QUEUE_DROP,
    QUEUE_ENQUE,
    QUEUE_DEQUE,
    
}QUEUE_EVENT;

typedef enum{
    CLIENT_LEAVING=0x0001,
    CLIENT_APPROACHING=0x0010,
    CLIENT_STATIC=0x0100,
    CLIENT_INTERFERED=0x1000
}CLIENT_STATE;

typedef struct ClientInfo{
    CLIENT_STATE physical_state;
    double avg_snr;
    double avg_snir;
    double avg_retry_count;
    
    
}ClientInfo;


class RRQueue {
public:
    
    int dst;  
    list<Packet*> *q;
    ClientInfo info;
    
    RRQueue *prev;
    RRQueue *next;
    
    /*statistics*/
    
    //current state
    int pktcount;
    int bytecount;
    
    //dropped count
    int pktcount_dropped;
    int bytecount_dropped;
    
    //served count
    
    int pktcount_served;
    int bytecount_served;
    
    double start_service_time;
    double end_service_time;
    /*for deficit round robin*/
    int deficit_counter;
    int turn;
    
    
    /*for airtime fair round robin*/
    double airtime_counter;
    
    bool activated;
    
    
    //added for wf2q+ functionality
    int qmaxSize_;     /* maximum queue size (in bytes) */
    int qcrtSize_;     /* current queue size (in bytes) */
    double new_weight_;
    double old_weight_;
    bool weight_change_;
    double weight_start_time_;
    double weight_;    /* Weight of the flow */
    double Start_time_;         /* Starting time of flow , not checked for wraparound*/
    double Finish_time_; 

    
    
public:
    RRQueue():dst(-1),prev(NULL),next(NULL),pktcount(0),bytecount(0),pktcount_dropped(0),bytecount_dropped(0),pktcount_served(0),bytecount_served(0),
            start_service_time(0.0),end_service_time(0.0),deficit_counter(0),turn(0),airtime_counter(0.0),activated(false),
            qmaxSize_(10000),qcrtSize_(0),new_weight_(1.0),old_weight_(1.0),weight_change_(false),weight_start_time_(0.0),weight_(1.0),Start_time_(0.0),Finish_time_(0.0)
    {
        q=new list<Packet*>();
        if(q==NULL)
        {
            cerr<<"Cannot create packet queue!"<<endl;
            exit(0);
        }
    }
    ~RRQueue()
    {
        if(q!=NULL)
        {
            q->clear();
            delete q;
        }
        
        if(activated)
        {
            cout<<"queue with dst: "<<dst<<endl;
            cout<<"packets served: "<<pktcount_served<<" total bytes: "<<bytecount_served<<endl;
            cout<<"packets dropped: "<<pktcount_dropped<<" total bytes: "<<bytecount_dropped<<endl;
        }
    }
    
    int size(){return pktcount;}
    
    void enque_packet(Packet *p)
    {   
        assert(p);
        pktcount++;
        hdr_cmn *header=hdr_cmn::access(p);
        bytecount+=header->size();
        
        qcrtSize_+=header->size();
        
        /*enque this packet pointer to the queue*/
        q->push_back(p);
    }
    void deque_packet(Packet *p)
    {   
        assert(p);
        pktcount--;
        hdr_cmn *header=hdr_cmn::access(p);
        bytecount-=header->size();
        
        qcrtSize_-=header->size();
        
        pktcount_served++;
        bytecount_served+=header->size();
        /*remove this packet pointer from the queue*/
        q->remove(p);
        
        if(pktcount==0)
        {
            turn=0;
            deficit_counter=0;
        }
        
    }
    
    void update_weight()
    {
        if(!weight_change_ || weight_start_time_==0.0 || (NOW-weight_start_time_)>1.0)
            return;
        
        weight_=old_weight_+(NOW-weight_start_time_)*(new_weight_-old_weight_);
        
        
        
    }
    
    Packet *deque_packet()
    {
       if(pktcount==0)
           return NULL;
       
       
        Packet *p=q->front();
        pktcount--;
        hdr_cmn *header=hdr_cmn::access(p);
        bytecount-=header->size();
        
        qcrtSize_-=header->size();
        
        
        
        pktcount_served++;
        bytecount_served+=header->size();
        /*remove this packet pointer from the queue*/
        q->remove(p);
        
        if(pktcount==0)
        {
            turn=0;
            deficit_counter=0;
        }
        
        return p;
       
    }
    
    Packet *get_front()
    {
        if(pktcount==0)
            return NULL;
        
        return q->front();
    }
    
    void drop_packet(Packet *p)
    {
        assert(p);
        pktcount--;
        hdr_cmn *header=hdr_cmn::access(p);
        bytecount-=header->size();
        
        qcrtSize_-=header->size();
        
        pktcount_dropped++;
        pktcount_dropped+=header->size();
         /*remove this packet pointer from the queue*/
        q->remove(p);
        
        if(pktcount==0)
        {
            turn=0;
            deficit_counter=0;
        }
    }
    
    
    inline RRQueue * activate(RRQueue *head) {
        
        if(activated==false)
            activated=true;
        
        
		if (head) {
			this->prev = head->prev;
			this->next = head;
			head->prev->next = this;
			head->prev = this;
			return head;
		}
		this->prev = this;
		this->next = this;
		return this;
	}
    inline RRQueue * idle (RRQueue *head) {
		if (head == this) {
			if (this->next == this)
				return 0;
			this->next->prev = this->prev;
			this->prev->next = this->next;
			return this->next;
		}
		this->next->prev = this->prev;
		this->prev->next = this->next;
		return head;
	}
   
    
};




class DashPriQueue : public PriQueue {
public:
        DashPriQueue();
        ~DashPriQueue();

        
        
        void    recv_data_packet(Packet *p,Handler *h, int high);
        
        int  find_queue_number(Packet *p);
        
        /*packet round robin*/
        Packet *select_packet_rr();
        Packet *select_packet_prio_first();
        
        /*FIFO*/
        Packet *select_packet_fifo();
        
        /*deficit round robin*/
        Packet *select_packet_drr();
        
        /*airtime-fair round robin*/
        Packet *select_packet_atrr();
        
        /*worst case weighted fair queuing*/
        /*current scenario assume the weights to be equal?*/
        Packet *select_packet_wf2q();
        
        
        void    QueueTrace(Packet *p,QUEUE_EVENT e,int queue_no,int subq_length, int queue_length);
        
        void    set_weight(int queue,double weight);
        
        void    set_weight_direct(int queue,double weight);
        
        virtual void resume();
        virtual void recv(Packet *p, Handler *h);
        virtual int  command(int argc, const char*const* argv);
        virtual void recvHighPriority(Packet *p, Handler *h);
        virtual void enque(Packet* p);
        virtual Packet* deque();
        
        
protected:
    
        int mac_addr;
        int max_dst;
        int current_dst;
        static const int NUM_BUCKETS=64;
        int quantum;
        int num_flows;
        
        /*airtime */
        double airtime_quantum;
        
       
        
        RRQueue *rr_cur; //current RRQueue;
        RRQueue *rr_drr; //all RRQueue;
        RRQueue *rr_high_prio; //high priority queue, for routing, broadcasting etc.
        RRQueue *return_rrq(int dst);
        

	
        FILE *queue_trace_fp;
        char queue_trace_name[300];
        bool queue_trace;
        bool queue_trace_initialized;
        int trace_count;
        Mac *_mac;
        
        
        //for wf2q+
        double _virtualclock;
        
       
        
        

};

#endif /* !_dashpriqueue_h */
