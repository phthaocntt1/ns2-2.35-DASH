/* -*-  Mode:C++; -*- */
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



#ifndef PEERSTATS_H
#define PEERSTATS_H
#include <iostream>
#include <list>
#include <queue>
#include <vector>
#include <algorithm>
#include "timer-handler.h"
#include "Mac80211EventHandler.h"

#define RECORD_LENGTH 100


enum Medium_State{
    QUALITY_INCREASE,
    QUALITY_DECREASE,
    QUALITY_STABLE,
    QUALITY_UNKNOWN
          
};


class MR_MAC_MIB;

struct STATS_RECORD{
    double time;
    double value;
    
};


struct _TransInfo{
    int dst;
    int size;
    double start_time;
    double duration;
    int resend_count;
    int dropped;
    
};
typedef struct _TransInfo TransInfo;

struct _DoubleExpSmth{
    double smoothed_value;
    
    double trend;
    double forcast;
    double alpha;
    double beta;
    
    int sample_count;
};

typedef struct _DoubleExpSmth DoubleExpSmth;

bool compare_transinfo(TransInfo info1, TransInfo info2);




class ARFPeerStats:public TimerHandler
{
public:
    
   /*for arf*/
  double _timeout_;
  
  int current_index;
  int max_index;
  int _nSuccess_; /** Number of frame trasmitted with succes in a row*/

  int _nFailed_; /** Number of unsucessfully transmitted frame in a
		    row */

  bool _justIncr;		
  bool _justTimedOut;
  bool _initialized;
  
  int _src;
  int _dst;
  FILE *_log;
  
  std::vector<TransInfo> _transinfo_q;
  int TRANSINFOQ_LIMIT;
  double BITRATE_SAMPLE_INT;
  
  Medium_State _medium_state;
  bool _first_time_sample;
  double _prev_time;
  double _current_bitrate;
  std::list<double> _bitrate_q;
  int BITRATE_Q_SIZE;
  FILE *_bitrate_history_ptr;
  bool _bitrate_file_initialized;
  
  DoubleExpSmth _bitrate_des;
  
  
public:
    ARFPeerStats():_timeout_(0.0),current_index(0),max_index(-1),_nSuccess_(0),_nFailed_(0),_justIncr(false),_justTimedOut(false),
            _initialized(false),_src(-1),_dst(-1),_log(NULL),TRANSINFOQ_LIMIT(20),BITRATE_SAMPLE_INT(1.0),
            _medium_state(QUALITY_STABLE),_first_time_sample(true),_current_bitrate(0.0),BITRATE_Q_SIZE(20),_bitrate_history_ptr(NULL),
            _bitrate_file_initialized(false)
    {
        _prev_time=NOW;
        
        memset(&_bitrate_des,0,sizeof(DoubleExpSmth));
        
        _bitrate_des.alpha=0.5;
        _bitrate_des.beta=0.5;
        
        
    }
    
    ~ARFPeerStats()
    {
        if(_log)
            fclose(_log);
        if(_bitrate_history_ptr)
            fclose(_bitrate_history_ptr);
        _transinfo_q.clear();
        _bitrate_q.clear();
    }
               
    void expire(Event *e)
    {
        _nFailed_ = 0;
        _justIncr = 0;
        if(incrMode())
        {
         _justIncr = 1;
         resched(_timeout_);
        }
        
        
        _nSuccess_ = 0;
    }
    
    void setTimeout(double timeout){_timeout_=timeout;}
    void setMaxIndex(int index){max_index=index;};
    void setCurrentIndex(int index){current_index=index;}
    
    void transq_enque(TransInfo ts)
    {
        
        if(!_bitrate_file_initialized && _src==0)
        { 
            char filename[500];
            sprintf(filename,"physical_bitrate_client_%d",_dst);
            _bitrate_history_ptr=fopen(filename,"w");
            
            if(_bitrate_history_ptr==NULL)
            {
                fprintf(stderr,"cannot create file %s\n",filename);
                exit(-1);
            }
            
            _bitrate_file_initialized=true;
        }
        
        
        _transinfo_q.push_back(ts);
        
        
        
        if(NOW-_prev_time>=BITRATE_SAMPLE_INT)
        {
           
            
            if(!_first_time_sample)
            {
            double total_duration=0.0;
            int total_size=0;
            
            int i;
            for(i=0;i<_transinfo_q.size();i++)
            {
                total_size+=_transinfo_q.at(i).size;
                total_duration+=_transinfo_q.at(i).duration;
            }
            
            _current_bitrate=(8*total_size)/total_duration/1000.0;
            
           if(_current_bitrate>0.0)
           _bitrate_q.push_back(_current_bitrate);
           
           if(_bitrate_q.size()>BITRATE_Q_SIZE)
           _bitrate_q.pop_front();
            
            //double exponential smoothing
            
            double smoothed_value_prev=_bitrate_des.smoothed_value;
            
            double predicted_value=_bitrate_des.smoothed_value+_bitrate_des.trend;
            
            if(_bitrate_des.sample_count==0)
            {
                _bitrate_des.smoothed_value=_current_bitrate;
                _bitrate_des.trend=0.0;
                
            }
            else if(_bitrate_des.sample_count==1)
            {
                _bitrate_des.trend=_current_bitrate-_bitrate_des.smoothed_value;
                
                _bitrate_des.smoothed_value=_bitrate_des.alpha*_current_bitrate+(1-_bitrate_des.alpha)*(_bitrate_des.smoothed_value+_bitrate_des.trend);
                
            }
            else
            {
                _bitrate_des.smoothed_value=_bitrate_des.alpha*_current_bitrate+(1-_bitrate_des.alpha)*(_bitrate_des.smoothed_value+_bitrate_des.trend);
                 
                _bitrate_des.trend=_bitrate_des.beta*(_bitrate_des.smoothed_value-smoothed_value_prev)+(1-_bitrate_des.beta)*_bitrate_des.trend;
            }
            
            
            
            _bitrate_des.sample_count++;
            
            if(_bitrate_history_ptr)
            {
                fprintf(_bitrate_history_ptr,"%f %f %f\n",NOW,_current_bitrate,predicted_value);
                fflush(_bitrate_history_ptr);
            }
            
            
            if(ts.dst==1)
            {
             // fprintf(stderr,"at time %f, from %d to dst %d, bitrate %f\n",NOW,_src,ts.dst,_current_bitrate);
           // int i;
           // for(i=0;i<20;i++)
               // fprintf(stderr,"At %d: %f, %d\n",i,_transinfo_q.at(i).duration,_transinfo_q.at(i).resend_count);
            }
            
            }
            
        _prev_time=NOW; 
        _first_time_sample=false;
        
        _transinfo_q.clear();
           
        }
        
        //if(ts.dropped)
        //fprintf(stderr,"to dst %d, time %f, retrans count %d, success %d\n",ts.dst,ts.duration,ts.resend_count,!ts.dropped);
            
    }
    
    int decrMode()
    {
        CheckConfig();
        
        
        if(current_index>0)
        {
            current_index--;
            
            if(_log)
            {
                fprintf(_log,"%.8f %d %d %d\n",NOW,_src,_dst,current_index);
                fflush(_log);
                
               
            }
            
            return 0;
        }
        
        return 1;
    }
    
    int incrMode()
    {
        CheckConfig();
        
        
        if(current_index<max_index)
        {
            current_index++;
            
            if(_log)
            {
                fprintf(_log,"%.8f %d %d %d\n",NOW,_src,_dst,current_index);
                fflush(_log);
            }
           
            return 0;
        }
        
        return 1;
    }
    
    void CheckConfig()
    {  
        
        assert(_timeout_>0.0);
        assert(max_index>0);
        assert(current_index>=0);
        assert(current_index<=max_index);
        
    }
    
    
};

typedef struct STATS_RECORD STATS_RECORD;
/**
 * Statistics related to the communications between two 802.11 instances.
 * which is denoted by the pair of mac addresses (src,dest)
 */
class PeerStats
{
public:
  PeerStats(); 
  ~PeerStats();

  virtual void updateSnr(double value);       
  virtual void updateSnir(double value);      
  virtual void updateInterf(double value);    

  virtual double getSnr();
  virtual double getSnir();
  virtual double getInterf();
  
  virtual double getAveragedSnr(){return _averaged_snr;}
  virtual double getAveragedSnir(){return _averaged_snir;}
  

  char* peerstats2string(char* str, int size);

  MR_MAC_MIB* macmib;  /**< holds all the counters for each (src,dest) */
  ARFPeerStats *arf;


protected:

  /*
   * NOTE: these are last values seen for each (src,dest) 
   * this does not make much sense since values
   * can vary pretty much among subsequent
   * transmissions (especially sinr and interference).
   * These protected member are provided only as the most basic
   * implementation. A better implementation would inherit from
   * PeerStats and overload the virtual public updateValue() and
   * getValue() methods in order to, e.g., return  average values.
   */

  double snr;          /**< Signal to Noise ratio in dB */
  double snir;         /**< Signal to Noise and Interference ratio in dB */
  double interf;       /**< Interference power in W */
  
  double _averaged_snr;
  double _averaged_snir;
  
  double _lambda;
  
  std::queue<STATS_RECORD>  snr_queue;
  std::queue<STATS_RECORD>  snir_queue;
  std::queue<STATS_RECORD>  interf_queue;
  
  
 
  
};



/**
 * Interface to the database containing PeerStats class instances for each connection.
 * Each implementation of this database must be plugged to MAC
 * instances. Depending on the implementation, a global and omniscient
 * database might be plugged in all MAC instances, or a per-MAC
 * instance of the database can be created.
 * 
 */
class PeerStatsDB : public TclObject 
{
public:

  PeerStatsDB();
  virtual ~PeerStatsDB();				

  /** 
   * Returns the PeerStats data related to the communication between
   * src and dst. Note that either src or dst should refer to the MAC
   * calling this method, in order for per-mac PeerStatsDB
   * implementations to implement efficient memory management. 
   * Particular PeerStatsDB implementations might offer a more relaxed
   * behaviour, but the caller of this method should not rely on it if
   * it's not aware of which PeerStatsDB implementation it's using.
   * 
   * @param src MAC address of the sender
   * @param dst MAC address of the receiver
   * 
   * @return pointer to the PeerStats class containing the required stats.
   */
  virtual PeerStats* getPeerStats(u_int32_t src, u_int32_t dst) = 0;

  int command(int argc, const char*const* argv);


protected:
  int VerboseCounters_;


};



#endif /* PEERSTATS_H */
