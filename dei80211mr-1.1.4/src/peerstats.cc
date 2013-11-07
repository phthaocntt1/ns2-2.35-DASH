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

#include "mac-802_11mr.h"


PeerStats::PeerStats():
_averaged_snr(0.0),
_averaged_snir(0.0),
_lambda(0.7)
        
{
  macmib = new MR_MAC_MIB;
  arf=new ARFPeerStats();
  
}


PeerStats::~PeerStats()
{
  delete macmib;
  
  while(!snr_queue.empty())
      snr_queue.pop();
  
  while(!snir_queue.empty())
      snir_queue.pop();
  
  while(!interf_queue.empty())
      interf_queue.pop();
  
  
}





char* PeerStats::peerstats2string(char* str, int size)
{
  char yastr[300];
  assert(macmib);
  macmib->counters2string(yastr, 300);
  snprintf(str, size, "%6.3f %6.3f %6.3f  %s",
	   getSnr(),
	   getSnir(),
	   getInterf(),	    
	   yastr);

  str[size-1]='\0';
  return str;  
}


PeerStatsDB::PeerStatsDB()
{
  bind("VerboseCounters_", &VerboseCounters_);
}


PeerStatsDB::~PeerStatsDB()
{
}

int PeerStatsDB::command(int argc, const char*const* argv)
{
  Tcl& tcl = Tcl::instance();

  //cerr << "StaticPeerStatsDB::command(\"" << argv[1] << "\")" << endl;
  
  if (argc == 4) {
    if(strcasecmp(argv[1], "getPeerStats") == 0) {
      int src = atoi(argv[2]);
      int dst = atoi(argv[3]);
      char str[500];
      PeerStats* ps = getPeerStats(src,dst);
      assert(ps);      
      ps->macmib->VerboseCounters_ = VerboseCounters_;
      ps->peerstats2string(str,500);
      tcl.resultf("%s",str);
      return (TCL_OK);
    }      
  }

}

void PeerStats::updateSnr(double value)       
{ 
    STATS_RECORD r;
    r.value=value;
    r.time=Scheduler::instance().clock();
    
    snr_queue.push(r);
    
    _averaged_snr=(1-_lambda)*_averaged_snr+_lambda*value;
    
    
    
    if(snr_queue.size()>RECORD_LENGTH)
        snr_queue.pop();
    
}



void PeerStats::updateSnir(double value)      
{  
    STATS_RECORD r;
    r.value=value;
    r.time=Scheduler::instance().clock();
    
    snir_queue.push(r);
    
     _averaged_snir=(1-_lambda)*_averaged_snir+_lambda*value;
    
    if(snir_queue.size()>RECORD_LENGTH)
        snir_queue.pop();
}


void PeerStats::updateInterf(double value)    
{  
    STATS_RECORD r;
    r.value=value;
    r.time=Scheduler::instance().clock();
    
    interf_queue.push(r);
    
    
    
    if(interf_queue.size()>RECORD_LENGTH)
        interf_queue.pop();
}

double PeerStats::getSnr()    
{   
    if(snr_queue.empty())
        return 0.0;
    STATS_RECORD r=snr_queue.front();
    snr=r.value;
    
    return snr; 
}
double PeerStats::getSnir()   
{  
    if(snir_queue.empty())
        return 0.0;
    STATS_RECORD r=snir_queue.front();
    snir=r.value;
    return snir; 
}
double PeerStats::getInterf() 
{  
    if(interf_queue.empty())
        return 0.0;
    STATS_RECORD r=interf_queue.front();
    interf=r.value;
    return interf; 
}

bool compare_transinfo(TransInfo info1, TransInfo info2)
{
    if(info1.duration<info2.duration)
        return false;
    else if(info1.duration>=info2.duration)
        return true;
}

