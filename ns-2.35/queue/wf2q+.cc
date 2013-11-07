/*
 * Carnegie Mellon University
 * 1999-2000
 *
 * Worst-case weighted fair Queueing.
 * Shahzad Ali
 * 
 * All of the code for wf2q+ is in this file.  Make sure to 
 * add wf2q+.o to the Makefile.
 * See http://www.cs.cmu.edu/~cheeko/wf2q+/ for further details
 * and references.
 */


#include <stdlib.h>
#include <string.h>
#include "queue.h"
#include "delay.h"

/* helpful functions */
#define max(x,y) (x<y)?y:x
#define min(x,y) (x>y)?y:x
#define MAXDOUBLE 1000000000000.0

/* maximum numbers of flows */
#define MAXFLOWS 200

/* default size of the per-flow queue (in bytes) */
#define DEF_QUEUE_SIZE 10000 

/* default flow weight */
#define DEF_FLOW_WEIGHT   0.0

class LinkDelay;

class WF2Q : public Queue {
public: 
  WF2Q();
  virtual int command(int argc, const char*const* argv);
  Packet *deque(void);
  void   enque(Packet *pkt);

protected:

  /* flow structure */
  struct flowState {
    PacketQueue q_;    /* packet queue associated to the corresponding flow */
    int qmaxSize_;     /* maximum queue size (in bytes) */
    int qcrtSize_;     /* current queue size (in bytes) */
    double weight_;    /* Weight of the flow */
    double S_;         /* Starting time of flow , not checked for wraparound*/
    double F_;         /* Ending time of flow, not checked for wraparound */
  } fs_[MAXFLOWS];

  double V;            /* Virtual time , not checked for wraparound!*/
  LinkDelay* link_;    /* To get the txt time of a packet */
  int off_ip_;
  int off_cmn_;
};


static class WF2QClass : public TclClass {
public:
	WF2QClass() : TclClass("Queue/WF2Q") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new WF2Q);
	}
} class_wf2q;


WF2Q::WF2Q()
{
  /* initialize flow's structure */
  for (int i = 0; i < MAXFLOWS; ++i) {
    fs_[i].qmaxSize_        = DEF_QUEUE_SIZE;
    fs_[i].qcrtSize_        = 0;
    fs_[i].weight_          = DEF_FLOW_WEIGHT;
    fs_[i].S_               = 0.0;
    fs_[i].F_               = 0.0;
  }
  V       = 0.0;
  link_   = NULL;
  bind("off_ip_", &off_ip_);
  bind("off_cmn_",&off_cmn_);
}


/* 
 *  entry points from OTcL to set per flow state variables
 *   - $q set-queue-size flow_id flow_queue_size 
 *   - $q set-flow-weight flow_id flow_weight
 *
 *  NOTE: $q represents the discipline queue variable in OTcl.
 */
int WF2Q::command(int argc, const char*const* argv)
{
  if (argc == 4) { 
    if (strcmp(argv[1], "set-queue-size") == 0) {

      int flowId     = atoi(argv[2]);
   
      if (flowId >= MAXFLOWS) {
        fprintf(stderr, "WF2Q: Flow id=%d too large; it should be < %d!\n",
          flowId, MAXFLOWS);
        abort();
      }
  
      fs_[flowId].qmaxSize_ = atoi(argv[3]); 	
      return (TCL_OK);
     
    }
    else if (strcmp(argv[1], "set-flow-weight") == 0) {
      int flowId = atoi(argv[2]);

       if (flowId >= MAXFLOWS) {
        fprintf(stderr, "WF2Q: Flow id=%d too large; it should be < %d!\n",
          flowId, MAXFLOWS);
        abort();
      }
       
       double flowweight = atof(argv[3]);

       if (flowweight <= 0) {
        fprintf(stderr, "WF2Q: Flow Weight=%.4f should >  0\n",
          flowweight);
        abort();
       }
       
       fs_[flowId].weight_ = flowweight;
       return (TCL_OK);
    }
  }
  if (argc == 3){
    if (strcmp(argv[1], "link") == 0) {
      LinkDelay* del = (LinkDelay*)TclObject::lookup(argv[2]);
      if (del == 0) {
	fprintf(stderr, "WF2Q: no LinkDelay object %s\n",
		    argv[2]);
	return(TCL_ERROR);
      }
      // set ptc now
      link_ = del;
      return (TCL_OK);
    }
  }
  return (Queue::command(argc, argv));
}

/* 
 * Receive a new packet.
 *
 * 
 */
void WF2Q::enque(Packet* pkt)
{
  hdr_cmn* hdr  = (hdr_cmn*)pkt->access(off_cmn_);
  hdr_ip*  hip  = (hdr_ip*)pkt->access(off_ip_);
  int flowId    = hip->flowid();
  int pktSize   = hdr->size();
  
  if (flowId >= MAXFLOWS) {
    fprintf(stderr, "WF2Q::enqueue-Flow id=%d too large; it should be < %d!\n",
	    flowId, MAXFLOWS);
    drop(pkt);
  }

  /* if buffer full, drop the packet; else enqueue it */
  if (fs_[flowId].qcrtSize_ + pktSize > fs_[flowId].qmaxSize_) {

    /* If the queue is not large enough for this packet */

    drop(pkt); 

  } else { 
    if (!fs_[flowId].qcrtSize_) {

      /* If queue for the flow is empty, calculate start and finish times */

      fs_[flowId].S_ = max(V, fs_[flowId].F_);
      fs_[flowId].F_ = fs_[flowId].S_ + ((double)pktSize/fs_[flowId].weight_);
      /* the weight_ parameter better not be 0! */
      /* update system virtual clock */

      double minS = fs_[flowId].S_;
      for (int i = 0; i < MAXFLOWS; ++i) {
	if (fs_[i].qcrtSize_)
	  if (fs_[i].S_ < minS)
	    minS = fs_[i].S_;
      }

      V = max(minS, V);
    }
    
    fs_[flowId].q_.enque(pkt); 
    fs_[flowId].qcrtSize_ += pktSize;
  }    
}


/* 
 * Dequeue the packet.
 */
Packet* WF2Q::deque()
{
  Packet *pkt = NULL, *nextPkt;
  int     i;
  int     pktSize;
  double minF = MAXDOUBLE;
  int     flow = -1;
  double W    = 0.0;

  /* look for the candidate flow with the earliest finish time */
  for (i = 0; i< MAXFLOWS; i++){
    if (!fs_[i].qcrtSize_)
      continue;
    if (fs_[i].S_ <= V)
      if (fs_[i].F_ < minF){
	flow = i;
	minF = fs_[i].F_;
      }
  }

  if (flow == -1 || minF == MAXDOUBLE)
    return (pkt);

  pkt = fs_[flow].q_.deque();
  pktSize = ((hdr_cmn*)pkt->access(off_cmn_))->size();
  fs_[flow].qcrtSize_ -= pktSize;
  
  /* Set the start and the finish times of the remaining packets in the
   * queue */

  nextPkt = fs_[flow].q_.head();
  if (nextPkt) {
    fs_[flow].S_ = fs_[flow].F_;
    fs_[flow].F_ = fs_[flow].S_ + 
	((((hdr_cmn*)nextPkt->access(off_cmn_))->size())/fs_[flow].weight_);
    /* the weight parameter better not be 0 */
  }

  /* update the virtual clock */
  double minS = fs_[flow].S_;
  for (i = 0; i < MAXFLOWS; ++i) {
    W += fs_[i].weight_;
    if (fs_[i].qcrtSize_)
      if (fs_[i].S_ < minS)
	minS = fs_[i].S_;
  }
  V = max(minS, (V + ((double)pktSize/W)));
  return(pkt);
}
