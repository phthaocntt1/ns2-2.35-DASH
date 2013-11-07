/*
 * Copyright (c) 2007 Regents of the SIGNET lab, University of Padova.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University of Padova (SIGNET lab) nor the 
 *    names of its contributors may be used to endorse or promote products 
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED 
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR 
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "arf.h"
#include"mac-802_11mr.h"

static class ARFClass : public TclClass {
public:
	ARFClass() : TclClass("RateAdapter/ARF") {}
	TclObject* create(int, const char*const*) {
		return (new ARF());
	}
} class_arf;



ARF::ARF() 
  : nSuccess_(0), nFailed_(0), 
    justTimedOut(false), justIncr(false),
        _useful_bitrate_fp(NULL),_file_initialized(false),_total_useful_size(0),
        SAMPLE_INT(10)
{
  bind("timeout_", &timeout_);
  bind("numSuccToIncrRate_", &numSuccToIncrRate_);
  bind("numFailToDecrRate_", &numFailToDecrRate_);
  bind("numFailToDecrRateAfterTout_", &numFailToDecrRateAfterTout_);
  bind("numFailToDecrRateAfterIncr_", &numFailToDecrRateAfterIncr_);
  
  
  
  _prev_sample_time=NOW;
}

ARF::~ARF()
{
    
    if(_useful_bitrate_fp)
        fclose(_useful_bitrate_fp);
}


int ARF::command(int argc, const char*const* argv)
{
  
  // Add TCL commands here...


  // Fallback for unknown commands
  return RateAdapter::command(argc, argv);
}



void ARF::counterEvent(CounterId id)
{

  if(debug_>3) 
    cerr << "ARF::counterEvent()" << endl;

  switch(id) 
    {

    case ID_MPDUTxSuccessful :    

      TxSuccessfulAction();

      break;

    case ID_ACKFailed:
    case ID_RTSFailed:   
   
      TxFailedAction();

      break;

    default:
      break;
      
    }
}


void ARF::counterEvent(CounterId id,int dst)
{

  if(debug_>3) 
    cerr << "ARF::counterEvent()" << endl;
  
   u_int32_t src = (u_int32_t)(mac_->addr());
   PeerStats *stat=getPeerStats(src,dst);
   

  switch(id) 
    {

    case ID_MPDUTxSuccessful :    

      TxSuccessfulAction(stat,dst,0);

      break;

    case ID_ACKFailed:
    case ID_RTSFailed:   
   
      TxFailedAction(stat,dst);

      break;
      
    case ID_MPDUTxFailed:
      
        
      TxFrameFailedAction(stat,dst);
          

    default:
      break;
      
    }
}


void ARF::counterEvent(CounterId id,int dst,int size)
{

  if(debug_>3) 
    cerr << "ARF::counterEvent()" << endl;
  
   u_int32_t src = (u_int32_t)(mac_->addr());
   PeerStats *stat=getPeerStats(src,dst);
   

  switch(id) 
    {

    case ID_MPDUTxSuccessful :    

      TxSuccessfulAction(stat,dst,size);

    default:
      break;
      
    }
}

void ARF::sendDATA(Packet* p)
{
    
    
    
    hdr_cmn* ch = HDR_CMN(p);
    MultiRateHdr *mrh = HDR_MULTIRATE(p);
    struct hdr_mac802_11* dh = HDR_MAC802_11(p);

    u_int32_t dst = (u_int32_t)ETHER_ADDR(dh->dh_ra);
    u_int32_t src = (u_int32_t)(mac_->addr());


   if((u_int32_t)ETHER_ADDR(dh->dh_ra) == MAC_BROADCAST) 
    {
      // Broadcast packets are always sent at the basic rate
      return;
    }
    
    PeerStats *stat=getPeerStats(src,dst);
    
    
    if(!stat->arf->_initialized)
    {
        
        stat->arf->setCurrentIndex(curr_mode_index);
        stat->arf->setTimeout(timeout_);
        stat->arf->setMaxIndex(numPhyModes_-1);
        stat->arf->_dst=dst;
        stat->arf->_src=this->mac_->addr();
        stat->arf->_log=this->file;
        stat->arf->_initialized=true;
    }
    
    int idx=stat->arf->current_index;
    
    memset(&_transinfo,0,sizeof(TransInfo));
    
    _transinfo.dst=dst;
    _transinfo.start_time=NOW;
    
    setModeAtIndex(idx);
    
}

void ARF::TxSuccessfulAction()
{
  nSuccess_++;
  nFailed_ = 0;
  justTimedOut = 0;
  justIncr = 0;
      
  if(nSuccess_ == numSuccToIncrRate_)
    {	  
      if(incrMode() == 0)
	{
	  resched(timeout_);
	  justIncr = 1;
	}
      nSuccess_ = 0;
    }

}

void ARF::TxSuccessfulAction(PeerStats* stat,int index,int size)
{
    /*update trans info*/
    assert(index==_transinfo.dst);
    _transinfo.duration=NOW-_transinfo.start_time;
    _transinfo.dropped=0;
    _transinfo.size=size;
    
    _total_useful_size+=size;
    
    assert(size>0);
    
    assert(_transinfo.duration>0.0);
    
    if(_file_initialized==false)
    {
    if(this->mac_->addr()==this->mac_->bss_id())
    {
  char filename[500];
  sprintf(filename,"total_bitrate.txt");
  _useful_bitrate_fp=fopen(filename,"w");
  if(_useful_bitrate_fp==NULL)
   {
      fprintf(stderr,"cannot create file %s\n",filename);
      exit(0);
    }
    }
    
    _file_initialized=true;
    }
    
    if(_useful_bitrate_fp)
    {
        if(NOW-_prev_sample_time>=SAMPLE_INT)
        {   
            double bitrate=_total_useful_size/(NOW-_prev_sample_time)/125.0;
            fprintf(_useful_bitrate_fp,"%f %f\n",NOW,bitrate);
            _total_useful_size=0;
            _prev_sample_time=NOW;
        }
    }
    
    
    if(!stat->arf->_initialized)
    {
       
        stat->arf->setCurrentIndex(curr_mode_index);
        stat->arf->setTimeout(timeout_);
        stat->arf->setMaxIndex(numPhyModes_-1);
        stat->arf->_dst=index;
        stat->arf->_src=this->mac_->addr();
        stat->arf->_log=this->file;
        
        stat->arf->_initialized=true;
    }
    
    stat->arf->transq_enque(_transinfo);
    memset(&_transinfo,0,sizeof(TransInfo));
    
    
    stat->arf->_nSuccess_++;
    stat->arf->_nFailed_=0;
    stat->arf->_justTimedOut=false;
    stat->arf->_justIncr=true;
    
    if(stat->arf->_nSuccess_==numSuccToIncrRate_)
    {
        if(stat->arf->incrMode()==0)
        {
            stat->arf->resched(timeout_);
            stat->arf->_justIncr=true;
            
           
            
        }
        
        stat->arf->_nSuccess_=0;
    }
}



void ARF::TxFailedAction()
{

  nFailed_++;
  nSuccess_ = 0;

  if ((justIncr && (nFailed_ >= numFailToDecrRateAfterIncr_))
      || (justTimedOut && (nFailed_ >= numFailToDecrRateAfterTout_))
      || (nFailed_ >= numFailToDecrRate_))
    {	  
      if (decrMode() == 0)
	{
	  resched(timeout_);
	}
      nFailed_ = 0;
      justTimedOut = 0;
      justIncr = 0;
    }
}

void ARF::TxFailedAction(PeerStats *stat,int index)
{
    
     /*update trans info*/
    assert(index==_transinfo.dst);
    _transinfo.resend_count++;
    
    
    if(!stat->arf->_initialized)
    {   
        
        
        stat->arf->setCurrentIndex(curr_mode_index);
        stat->arf->setTimeout(timeout_);
        stat->arf->setMaxIndex(numPhyModes_-1);
        stat->arf->_dst=index;
        stat->arf->_src=this->mac_->addr();
        stat->arf->_log=this->file;
        stat->arf->_initialized=true;
    }
    
    
  stat->arf->_nFailed_++;
  stat->arf->_nSuccess_ = 0;

  if ((stat->arf->_justIncr && (stat->arf->_nFailed_ >= numFailToDecrRateAfterIncr_))
      || (stat->arf->_justTimedOut && (stat->arf->_nFailed_ >= numFailToDecrRateAfterTout_))
      || (stat->arf->_nFailed_ >= numFailToDecrRate_))
    {	  
      if (stat->arf->decrMode() == 0)
	{
	  stat->arf->resched(timeout_);
	}
      stat->arf->_nFailed_ = 0;
      stat->arf->_justTimedOut = 0;
      stat->arf->_justIncr = 0;
    }
}


void ARF::TxFrameFailedAction(PeerStats* stat, int index)
{
    /*update trans info*/
    assert(index==_transinfo.dst);
    _transinfo.duration=NOW-_transinfo.start_time;
    assert(_transinfo.duration>0.0);
    _transinfo.dropped=1;
    _transinfo.size=0;
    
    if(!stat->arf->_initialized)
    {   
        
        
        stat->arf->setCurrentIndex(curr_mode_index);
        stat->arf->setTimeout(timeout_);
        stat->arf->setMaxIndex(numPhyModes_-1);
        stat->arf->_dst=index;
        stat->arf->_src=this->mac_->addr();
        stat->arf->_log=this->file;
        stat->arf->_initialized=true;
    }
    
    stat->arf->transq_enque(_transinfo);
    
    memset(&_transinfo,0,sizeof(TransInfo));
    
}




void ARF::expire(Event *e)
{

  nFailed_ = 0;
  justIncr = 0;
  if(incrMode())
    {
      justIncr = 1;
      resched(timeout_);
    }
  nSuccess_ = 0;
} 


void ARF::chBusy(double)
{
}
