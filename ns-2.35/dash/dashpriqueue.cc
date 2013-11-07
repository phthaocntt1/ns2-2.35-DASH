#include <list>
#include <iterator>
#include <math.h>
#include <object.h>
#include <queue.h>
#include <drop-tail.h>
#include <packet.h>
#include <cmu-trace.h>
#include <../mac/mac-802_11.h>

#include "dashpriqueue.h"


#define MAX_DOUBLE 1000000000000.0

using namespace std;
 
static class DashPriQueueClass : public TclClass {
public:
  DashPriQueueClass() : TclClass("Queue/DropTail/DashPriQueue") {}
  TclObject* create(int, const char*const*) {
    return (new DashPriQueue);
  }
} class_DashPriQueue; 


DashPriQueue::DashPriQueue() : PriQueue()
{       
        
        
        queue_trace=true;
        queue_trace_initialized=false;
        
        //default value for (deficit) round robin
        
        quantum=250;
        
        num_flows=0;
        
        /*airtime quantum*/
        airtime_quantum=0.0001;
        
        max_dst=-1;
        current_dst=0;
        mac_addr=-1;
        
        trace_count=0;
        _mac=NULL;
        
        
        
        
        rr_drr=new RRQueue[NUM_BUCKETS];
        rr_cur=NULL;
        rr_high_prio=new RRQueue;
        if(rr_drr==NULL || rr_high_prio==NULL)
        {
            cerr<<"Cannot create RRQueue"<<endl;
            exit(0);
        } 
        
        rr_high_prio->dst=-1;
        int i;
        for(i=0;i<NUM_BUCKETS;i++)
            rr_drr[i].dst=i;
        
        
        _virtualclock=0.0;
        
        
}

DashPriQueue::~DashPriQueue()
{

    if(rr_drr!=NULL)
        delete rr_drr;
    
    if(rr_high_prio!=NULL)
        delete rr_high_prio;
   
   if(queue_trace_fp)
        fclose(queue_trace_fp);
    

}


int
DashPriQueue::command(int argc, const char*const* argv)
{
  
  return PriQueue::command(argc, argv);
}

void
DashPriQueue::QueueTrace(Packet *p,QUEUE_EVENT e, int queue_no, int subq_length, int queue_length)
{   
    
    
    char temp[500];
    int offset;
    struct hdr_cmn *ch = HDR_CMN(p);
    struct hdr_mac802_11 *ch80211=HDR_MAC802_11(p);
    
    trace_count++;
    switch(e)
    {
      case QUEUE_DROP: 
          offset=sprintf(temp,"[ %d %p %3d %3d %3d ] D  %.15g  %d [ %x %x %3d %3d ]", trace_count,p,queue_no,subq_length,queue_length,Scheduler::instance().clock(),
                  ch->ptype_,ETHER_ADDR(ch80211->dh_ta),ETHER_ADDR(ch80211->dh_ra),ch->uid_,ch->size_);
          break;
      case QUEUE_ENQUE:
          offset=sprintf(temp,"[ %d %p %3d %3d %3d ] +  %.15g  %d [ %x %x %3d %3d ]", trace_count,p,queue_no,subq_length,queue_length,Scheduler::instance().clock(),
                  ch->ptype_,ETHER_ADDR(ch80211->dh_ta),ETHER_ADDR(ch80211->dh_ra),ch->uid_,ch->size_);
          break;
      case QUEUE_DEQUE:
          offset=sprintf(temp,"[ %d %p %3d %3d %3d ] -  %.15g  %d [ %x %x %3d %3d ]", trace_count,p,queue_no,subq_length,queue_length,Scheduler::instance().clock(),
                  ch->ptype_,ETHER_ADDR(ch80211->dh_ta),ETHER_ADDR(ch80211->dh_ra),ch->uid_,ch->size_);
          break;
        default:
          offset=sprintf(temp,"[ %d %p %3d %3d %3d ] U  %.15g  %d [ %x %x %3d %3d ]", trace_count,p,queue_no,subq_length,queue_length,Scheduler::instance().clock(),
                  ch->ptype_,ETHER_ADDR(ch80211->dh_ta),ETHER_ADDR(ch80211->dh_ra),ch->uid_,ch->size_);
            
    }
    
    
    
    if(queue_trace && queue_trace_initialized && queue_trace_fp!=NULL)
    offset=fprintf(queue_trace_fp,"%s\n",temp);
    
   
    fflush(queue_trace_fp);
    
    
    
                        
            
}












RRQueue*
DashPriQueue::return_rrq(int queue_number)
{
    
    if(queue_number==-1)
        return rr_high_prio;
    
    
    else return &rr_drr[queue_number];
    
    
}






void  
DashPriQueue::recv_data_packet(Packet *p,Handler *h, int high)
{
    
    
    
    if(p==NULL)
    {
        cerr<<"packet is null!"<<endl;
        return;
    }
    
   
    
    

    
    double now = Scheduler::instance().clock();
    
    int queue_number=find_queue_number(p);
    RRQueue *subq=return_rrq(queue_number);
    
    if(subq==NULL)
    {
        cerr<<"Error in finding subq for packet with dest "<<queue_number<<endl;
        exit(0);
    }
    
    
    
   
    
    
    
    //=========================================
    if (summarystats) {
                Queue::updateStats(qib_?q_->byteLength():q_->length());
	}

	int qlimBytes = qlim_ * mean_pktsize_;
	if ((!qib_ && (q_->length() + 1) >= qlim_) ||
  	(qib_ && (q_->byteLength() + hdr_cmn::access(p)->size()) >= qlimBytes))
        {
		// if the queue would overflow if we added this packet...
		if (1/*drop_front_*/) { /* remove from head of queue */
                    
                    
                    
			q_->enque(p);
                       
                        //enque packet, increase size
                        subq->enque_packet(p);
                        
                        if(subq->size()==1)
                        {
                            rr_cur=subq->activate(rr_cur);
                            subq->deficit_counter=0;
                            num_flows++;
                            
                        }
                        
                        //=================for wf2q==============
                         /* If queue for the flow is empty, calculate start and finish times */

                        if(subq->size()==1)
                        {
                            subq->update_weight();
                            
                            subq->Start_time_=max(_virtualclock,subq->Start_time_);
                            subq->Finish_time_=subq->Start_time_+hdr_cmn::access(p)->size()/subq->weight_;
                            
                            
                            
                            double minS = subq->Start_time_;
                            for (int i = 0; i < NUM_BUCKETS; ++i) 
                            {
                                RRQueue *localq=&rr_drr[i];
	                       if (localq->qcrtSize_>0)
	                         if (localq->Start_time_ < minS)
	                          minS = localq->Start_time_;
                            }
                            
                            

                               _virtualclock = max(minS, _virtualclock);
                       }
                        
                        
                       
                        
                        
                       // fprintf(stderr,"A packet %p with dst %d is queued at node %d, subq %d, q %d\n",p,data_queue_number(p),mac_addr,subq->size(),q_->length());
                        QueueTrace(p,QUEUE_ENQUE,queue_number,subq->size(),q_->length());
                        
                        
                        
                        /*randomly drops a packet*/
                        int packet_index=rand()%qlim_;
                        
                        Packet *pp=q_->lookup(packet_index);
                        q_->remove(pp);
                       
                        
			
                        
                        /*find the corresponding subq of this to-drop packet*/
                        int qnum=find_queue_number(pp);
                        
                           
                           subq=return_rrq(qnum);
                           
                           if(subq==NULL)
                           {
                               cerr<<"Error in retrieving subq with qnum "<<qnum<<", going to exit!"<<endl;
                               exit(0);
                           }
                           
                        
                           
                           subq->drop_packet(p);
                           
                           if(subq->size()==0)
                           {
                               rr_cur=subq->idle(rr_cur);
                               num_flows--;
                           }
                           
    
                          
                           
                              // fprintf(stderr,"packets for qnum %d is to be dropped!\n",qnum);
                        
                        QueueTrace(pp,QUEUE_DROP,qnum,subq->size(),q_->length());
                        
                        
			drop(pp);
		} else {
                    
                    
                       /*directly drop the incoming packet*/
                    
                        QueueTrace(p,QUEUE_DROP,queue_number,subq->size(),q_->length());
                       
		        drop(p);
		}
	} 
        else 
        {     
            
                       
		        q_->enque(p);
                        
                        
                        subq->enque_packet(p);
                        
                        if(subq->size()==1)
                        {
                            rr_cur=subq->activate(rr_cur);
                            subq->deficit_counter=0;
                            num_flows++;
                            
                        }
                        
                        //=================for wf2q==============
                         /* If queue for the flow is empty, calculate start and finish times */

                        if(subq->size()==1)
                        {
                            subq->update_weight();
                            
                            subq->Start_time_=max(_virtualclock,subq->Start_time_);
                            subq->Finish_time_=subq->Start_time_+hdr_cmn::access(p)->size()/subq->weight_;
                            
                            
                            
                            double minS = subq->Start_time_;
                            for (int i = 0; i < NUM_BUCKETS; ++i) 
                            {
                                RRQueue *localq=&rr_drr[i];
	                       if (localq->qcrtSize_>0)
	                         if (localq->Start_time_ < minS)
	                          minS = localq->Start_time_;
                            }
                            
                            

                               _virtualclock = max(minS, _virtualclock);
                       }
                        
                       
                        
                       
                        
                       // fprintf(stderr,"B packet with dst %d is queued at node %d, subq %d, q %d ",find_queue_number(p),mac_addr,subq->size(),q_->length());
                       
                        QueueTrace(p,QUEUE_ENQUE,queue_number,subq->size(),q_->length());
	}
        

        
        /*select a packet to send to the lower layer*/
        
#if 1
        if (!blocked_) {
		/*
		 * We're not blocked.  Get a packet and send it on.
		 * We perform an extra check because the queue
		 * might drop the packet even if it was
		 * previously empty!  (e.g., RED can do this.)
		 */
            
            
                /*first always select packets with dst -1 (routing and broadcast packets ) */
                p=NULL;
                
                p=select_packet_prio_first();
                
                
                              
                
		if (p != 0) {
			utilUpdate(last_change_, now, blocked_);
			last_change_ = now;
			blocked_ = 1;
                        
                        
                        
                       
			target_->recv(p, &qh_);
		}
	}
#else
   
    if (!blocked_) {
		/*
		 * We're not blocked.  Get a packet and send it on.
		 * We perform an extra check because the queue
		 * might drop the packet even if it was
		 * previously empty!  (e.g., RED can do this.)
		 */
		p = deque();
                
                if(p)
                {
                int qnum=data_queue_number(p);
                        
                   
                subq=return_rrq(qnum);
                
                if(subq==NULL)
                {
                    fprintf(stderr,"2 error in retrieving subq with qnum %d, going to exit!\n",qnum);
                    exit(0);
                }
                subq->remove(p);
                }
		if (p != 0) {
			utilUpdate(last_change_, now, blocked_);
			last_change_ = now;
			blocked_ = 1;
                        
                        //QueueTrace(p,QUEUE_DEQUE,0,pq_->length());
                        
                        
			target_->recv(p, &qh_);
		}
	}
        
#endif
    
   

    
}




int
DashPriQueue::find_queue_number(Packet *p)
{
    int number;
    
    struct hdr_cmn *ch = HDR_CMN(p);
    
    if(Prefer_Routing_Protocols) 
    {

                switch(ch->ptype()) {
		case PT_DSR:
		case PT_MESSAGE:
                case PT_TORA:
                case PT_AODV:
                    /*not routing protocol, but necessary for arp packets to have priority */
                case PT_ARP:
		case PT_AOMDV:
		case PT_MDART:
                    
                    number=-1;
                    
                    
                    
                    return number;
                    
                    break;

                default:
                    break;
                }
   }
    
    
    struct hdr_mac802_11 *ch80211=HDR_MAC802_11(p);
    
    number=ETHER_ADDR(ch80211->dh_ra);
    
    
    
    
    
    return number%NUM_BUCKETS;
    
    
}




void 
DashPriQueue::resume()
{
    
    
   
   
	double now = Scheduler::instance().clock();
	Packet* p=NULL;
        
        
        
        p=select_packet_prio_first();
        
        
        
        
        
	if (p != 0) {
		target_->recv(p, &qh_);
	} else {
		if (unblock_on_resume_) {
			utilUpdate(last_change_, now, blocked_);
			last_change_ = now;
                        
                       
			blocked_ = 0;
		}
		else {
			utilUpdate(last_change_, now, blocked_);
			last_change_ = now;
			blocked_ = 1;
		}
	}
}


Packet *
DashPriQueue::select_packet_prio_first()
{
    Packet *p=NULL;
    
    /*first check the priority queue*/
    RRQueue *subq=return_rrq(-1);
    
    
                if(subq!=NULL && subq->size()!=0)
                {
                   
                    p=subq->deque_packet();
                      if(subq->size()==0)
                      {
                      num_flows--;
                      rr_cur=subq->idle(rr_cur);
                      }
    
                    
                     /*remove the packet from primary queue*/
                    q_->remove(p);
                    
                    
                    //fprintf(stderr,"packet %p with dst %d is dequeued at node %d, subq %d, q %d\n",p,data_queue_number(p),mac_addr,subq->size(),q_->length());
                    QueueTrace(p,QUEUE_DEQUE,find_queue_number(p),subq->size(),q_->length());
                    
                    return p;
                     
                }
    
    
               
             if(_mac && num_flows>0)
             p=_mac->select_packet_mac();
                    
 
               /*remove the packet from the main queue*/ 
                if(p)
                {
                    
                    q_->remove(p);
                    
                    //fprintf(stderr,"packet %p with dst %d is dequeued at node %d, subq %d, q %d\n",p,data_queue_number(p),mac_addr,subq->size(),q_->length());
                    QueueTrace(p,QUEUE_DEQUE,find_queue_number(p),subq->size(),q_->length());
                
                }
    
              // fprintf(stderr,"at node %d, dequeued packet is to dst %d\n",mac_addr,find_queue_number(p));;
    
               return p;
    
}


Packet *
DashPriQueue::select_packet_fifo()
{
    Packet *p=NULL;
    RRQueue *subq;
    
    p=q_->lookup(0);
    
     if(p)
     {
         
    subq=return_rrq(find_queue_number(p));
    
    subq->deque_packet(p);
    
    if(subq->size()==0)
       {
        num_flows--;
        rr_cur=subq->idle(rr_cur);
       }
    
    
     }
    
    
    return p;
    
}

/*round robin selection*/
Packet *
DashPriQueue::select_packet_rr()
{
    
    Packet *p=NULL;
    
    
    
    
                 while(p==NULL)
                    
                {   
                     if(rr_cur->size()>0)
                     {
                         
                     p=rr_cur->deque_packet();
                     if(rr_cur->size()==0)
                     {
                         num_flows--;
                         rr_cur=rr_cur->idle(rr_cur);
                         
                     }
                     }
                     
                     
                     if(rr_cur!=NULL)
                        rr_cur=rr_cur->next;   
                }
                
               
               return p;
}


Packet *
DashPriQueue::select_packet_drr()
{
    Packet *pkt=NULL;
    hdr_cmn *hr;
    
               while(pkt==NULL)
               {
                   if(rr_cur->turn==0)
                   {
                       rr_cur->turn=1;
                       rr_cur->deficit_counter+=quantum;
                   }
                   
                   pkt=rr_cur->get_front();
                   hr=hdr_cmn::access(pkt);
                   
                   if(rr_cur->deficit_counter>=hr->size())
                   {
                       rr_cur->deficit_counter-=hr->size();
                       rr_cur->deque_packet(pkt);
                       
                       if(rr_cur->size()==0)
                       {
                           num_flows--;
                           rr_cur=rr_cur->idle(rr_cur);
                       }
                   }
                   else
                   {
                       rr_cur->turn=0;
                       rr_cur=rr_cur->next;
                       pkt=NULL;
                       
                       
                   }
                   
               }
    
    
    return pkt;
}

Packet *
DashPriQueue::select_packet_atrr()
{
    Packet *pkt=NULL;
    hdr_cmn *hr;
    
               while(pkt==NULL)
               {
                   if(rr_cur->turn==0)
                   {
                       rr_cur->turn=1;
                       rr_cur->airtime_counter+=airtime_quantum;
                   }
                   
                   pkt=rr_cur->get_front();
                   hr=hdr_cmn::access(pkt);
                   
                   if(rr_cur->airtime_counter>=hr->size())
                   {
                       rr_cur->airtime_counter-=hr->size();
                       rr_cur->deque_packet(pkt);
                       
                       if(rr_cur->size()==0)
                       {
                           num_flows--;
                           rr_cur=rr_cur->idle(rr_cur);
                       }
                   }
                   else
                   {
                       rr_cur->turn=0;
                       rr_cur=rr_cur->next;
                       pkt=NULL;
                       
                       
                   }
                   
               }
    
    
    return pkt;
}



void
DashPriQueue::recv(Packet *p, Handler *h)
{
    
    //fprintf(stderr,"DashPriQueue received packet %p\n",p);
        struct hdr_cmn *ch = HDR_CMN(p);
        
        if(queue_trace)
            if(!queue_trace_initialized)
            {
                _mac=dynamic_cast<Mac*>(target_);
                if(_mac)
                {
                    fprintf(stderr,"queue log with mac address %d established\n",_mac->addr());
                    sprintf(queue_trace_name,"node_%d_queue.queue.log",_mac->addr());
                    
                    mac_addr=_mac->addr();
                }
                else
                {
                    fprintf(stderr,"Fatal error as mac address cannot be recognized!\n");
                    exit(-1);
                }
                
                queue_trace_fp=fopen(queue_trace_name,"w");
                if(queue_trace_fp)
                queue_trace_initialized=true;
                    
            }
        
        
       
        
        

        if(Prefer_Routing_Protocols) { 

                switch(ch->ptype()) {
		case PT_DSR:
		case PT_MESSAGE:
                case PT_TORA:
                case PT_AODV:
                    /*not routing protocol, but necessary for arp packets to have high priority */
                case PT_ARP:
		case PT_AOMDV:
		case PT_MDART:
			recvHighPriority(p, h);
                        break;

                default:
                        recv_data_packet(p, h,0);
                }
        }
        else {
            
            recv_data_packet(p,h,0);
        }
}




void 
DashPriQueue::recvHighPriority(Packet *p, Handler *h)
  // insert packet at front of queue
{
    
recv_data_packet(p,h,1);      
 
}



void DashPriQueue::enque(Packet* p)
{
	if (summarystats) {
                Queue::updateStats(qib_?q_->byteLength():q_->length());
	}

	int qlimBytes = qlim_ * mean_pktsize_;
	if ((!qib_ && (q_->length() + 1) >= qlim_) ||
  	(qib_ && (q_->byteLength() + hdr_cmn::access(p)->size()) >= qlimBytes)){
		// if the queue would overflow if we added this packet...
		if (drop_front_) { /* remove from head of queue */
			q_->enque(p);
                        
                        QueueTrace(p,QUEUE_ENQUE,0,q_->length(),q_->length());
                        
                        
			Packet *pp = q_->deque();
                        
                        
                        QueueTrace(pp,QUEUE_DROP,0,q_->length(),q_->length());
                        
                        
			drop(pp);
		} else {
                    
                        QueueTrace(p,QUEUE_DROP,0, q_->length(), q_->length());
                       
		        drop(p);
		}
	} else {
		q_->enque(p);
                
                        QueueTrace(p,QUEUE_ENQUE,0,q_->length(), q_->length());
	}
}


Packet* DashPriQueue::deque()
{
    
    
    
        if (summarystats && &Scheduler::instance() != NULL) {
                Queue::updateStats(qib_?q_->byteLength():q_->length());
        }
        
        Packet *p=q_->deque();
        if(p)
            QueueTrace(p,QUEUE_DEQUE,0,q_->length(),q_->length());
	return p;
}


Packet *DashPriQueue::select_packet_wf2q()
{
  Packet *pkt = NULL, *nextPkt;
  int     i;
  int     pktSize;
  double minF = MAX_DOUBLE;
  int     flow = -1;
  double W    = 0.0;

  /* look for the candidate flow with the earliest finish time */
  for (i = 0; i< NUM_BUCKETS; i++){
    if (rr_drr[i].qcrtSize_==0)
      continue;
    if (rr_drr[i].Start_time_ <= _virtualclock)
      if (rr_drr[i].Finish_time_< minF){
	flow = i;
	minF = rr_drr[i].Finish_time_;
      }
  }
  
  
  

  if (flow == -1 || minF == MAX_DOUBLE)  
  { 
      
      
    return (pkt);
  }
  
  
 
  pkt = rr_drr[flow].deque_packet();
  
  hdr_cmn *header=hdr_cmn::access(pkt);
  pktSize=header->size();
  
  
  /* Set the start and the finish times of the remaining packets in the
   * queue */

  nextPkt = rr_drr[flow].get_front();
  
  if (nextPkt) {
      
      rr_drr[flow].update_weight();
      
    
    rr_drr[flow].Start_time_ = rr_drr[flow].Finish_time_;
    rr_drr[flow].Finish_time_ = rr_drr[flow].Start_time_ + 
	(pktSize/rr_drr[flow].weight_);
    /* the weight parameter better not be 0 */
  }
  
  
  for(i=0;i<NUM_BUCKETS;i++)
  
      rr_drr[i].update_weight();
  

  /* update the virtual clock */
  double minS = rr_drr[flow].Start_time_;
  for (i = 0; i < NUM_BUCKETS; ++i) {
      if(rr_drr[i].qcrtSize_==0)
          continue;
    W += rr_drr[i].weight_;
    if (rr_drr[i].qcrtSize_!=0)
      if (rr_drr[i].Start_time_ < minS)
	minS = rr_drr[i].Start_time_;
  }
  
  if(W>0.0)
  _virtualclock = max(minS, (_virtualclock + ((double)pktSize/W)));
  
  
  
  return(pkt);
}


void DashPriQueue::set_weight_direct(int queue, double weight)
{
    if(queue<0 || queue>=NUM_BUCKETS)
    {
        fprintf(stderr,"Error, no queue with dst %d\n",queue);
        exit(0);
    }
    
    if(weight<=0.0)
    {
        fprintf(stderr,"Error, negative weight %f set for queue %d\n",weight,queue);
        exit(0);
    }
    
    rr_drr[queue].weight_=weight;
    
    
}


void DashPriQueue::set_weight(int queue, double weight)
{
    if(queue<0 || queue>=NUM_BUCKETS)
    {
        fprintf(stderr,"Error, no queue with dst %d\n",queue);
        exit(0);
    }
    
    if(weight<=0.0)
    {
        fprintf(stderr,"Error, negative weight %f set for queue %d\n",weight,queue);
        exit(0);
    }
    
    rr_drr[queue].new_weight_=weight;
    rr_drr[queue].old_weight_=rr_drr[queue].weight_;
    
    rr_drr[queue].weight_start_time_=NOW;
    
   
    if(fabs(rr_drr[queue].new_weight_-rr_drr[queue].old_weight_)<0.01)
        rr_drr[queue].weight_change_=false;
    else 
        
        rr_drr[queue].weight_change_=true;
    
 //   rr_drr[queue].weight_=weight;
    
    
}


