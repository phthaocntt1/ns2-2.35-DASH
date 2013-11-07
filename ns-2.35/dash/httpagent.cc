#include <list>
#include <queue>
#include "httpagent.h"
#include "media.h"

#include "ip.h"
#include "tcp-full.h"
#include "flags.h"
#include "random.h"
#include "template.h"
#include "media.h"



using namespace std;

static class HttpFullTcpClass : public TclClass { 
public:
	HttpFullTcpClass() : TclClass("Agent/TCP/FullTcp/HTTP") {}
	TclObject* create(int, const char*const*) { 
		return (new DHttpAgent());
	}
} class_full_http;




int DHttpAgent::command(int argc, const char* const *argv)
{
    return FullTcpAgent::command(argc,argv);
}

void DHttpAgent::recv(Packet* pkt, Handler*)
{
    
   
        hdr_tcp *tcph = hdr_tcp::access(pkt);	// TCP header
	hdr_cmn *th = hdr_cmn::access(pkt);	// common header (size, etc)
	hdr_flags *fh = hdr_flags::access(pkt);	// flags (CWR, CE, bits)

	int needoutput = FALSE;
	int ourfinisacked = FALSE;
	int dupseg = FALSE;			// recv'd dup data segment
	int todrop = 0;				// duplicate DATA cnt in seg

	last_state_ = state_;

	int datalen = th->size() - tcph->hlen(); // # payload bytes
	int ackno = tcph->ackno();		 // ack # from packet
	int tiflags = tcph->flags() ; 		 // tcp flags from packet

//if (state_ != TCPS_ESTABLISHED || (tiflags&(TH_SYN|TH_FIN))) {
//fprintf(stderr, "%d in state %s recv'd this packet: \n", this->getfid(), statestr(state_));
//}

	/* 
	 * Acknowledge FIN from passive closer even in TCPS_CLOSED state
	 * (since we lack TIME_WAIT state and RST packets,
	 * the loss of the FIN packet from the passive closer will make that
	 * endpoint retransmit the FIN forever)
	 * -F. Hernandez-Campos 8/6/00
	 */
	if ( (state_ == TCPS_CLOSED) && (tiflags & TH_FIN) ) {
		goto dropafterack;
	}

	/*
	 * Don't expect to see anything while closed
	 */

	if (state_ == TCPS_CLOSED) {
                if (debug_) {
		        fprintf(stderr, "%f: FullTcp(%s): recv'd pkt in CLOSED state: ",
			    now(), name());
		        prpkt(pkt);
                }
		goto drop;
	}

        /*
         * Process options if not in LISTEN state,
         * else do it below
         */
	if (state_ != TCPS_LISTEN)
		dooptions(pkt);

	/*
	 * if we are using delayed-ACK timers and
	 * no delayed-ACK timer is set, set one.
	 * They are set to fire every 'interval_' secs, starting
	 * at time t0 = (0.0 + k * interval_) for some k such
	 * that t0 > now
	 */
	if (delack_interval_ > 0.0 &&
	    (delack_timer_.status() != TIMER_PENDING)) {
		int last = int(now() / delack_interval_);
		delack_timer_.resched(delack_interval_ * (last + 1.0) - now());
	}

	/*
	 * Try header prediction: in seq data or in seq pure ACK
	 *	with no funny business
	 */
	if (!nopredict_ && predict_ok(pkt)) {
                /*
                 * If last ACK falls within this segment's sequence numbers,
                 * record the timestamp.
		 * See RFC1323 (now RFC1323 bis)
                 */
                if (ts_option_ && !fh->no_ts_ &&
		    tcph->seqno() <= last_ack_sent_) {
			/*
			 * this is the case where the ts value is newer than
			 * the last one we've seen, and the seq # is the one
			 * we expect [seqno == last_ack_sent_] or older
			 */
			recent_age_ = now();
			recent_ = tcph->ts();
                }

		//
		// generate a stream of ecnecho bits until we see a true
		// cong_action bit
		//

	    	if (ecn_) {
	    		if (fh->ce() && fh->ect()) {
	    			// no CWR from peer yet... arrange to
	    			// keep sending ECNECHO
	    			recent_ce_ = TRUE;
	    		} else if (fh->cwr()) {
	    			// got CWR response from peer.. stop
	    			// sending ECNECHO bits
	    			recent_ce_ = FALSE;
	    		}
	    	}

		// Header predication basically looks to see
		// if the incoming packet is an expected pure ACK
		// or an expected data segment

		if (datalen == 0) {
			// check for a received pure ACK in the correct range..
			// also checks to see if we are wnd_ limited
			// (we don't change cwnd at all below), plus
			// not being in fast recovery and not a partial ack.
			// If we are in fast
			// recovery, go below so we can remember to deflate
			// the window if we need to
			if (ackno > highest_ack_ && ackno < maxseq_ &&
			    cwnd_ >= wnd_ && !fastrecov_) {
				newack(pkt);	// update timers,  highest_ack_
				send_much(0, REASON_NORMAL, maxburst_);
				Packet::free(pkt);
				return;
			}
		} else if (ackno == highest_ack_ && rq_.empty()) {
			// check for pure incoming segment
			// the next data segment we're awaiting, and
			// that there's nothing sitting in the reassem-
			// bly queue
			// 	give to "application" here
			//	note: DELACK is inspected only by
			//	tcp_fasttimo() in real tcp.  Every 200 ms
			//	this routine scans all tcpcb's looking for
			//	DELACK segments and when it finds them
			//	changes DELACK to ACKNOW and calls tcp_output()
			rcv_nxt_ += datalen;
			flags_ |= TF_DELACK;
                        
                        /*sanity check*/
                        if(!_recvq.empty())
                        
                            Log(DASH_LOG_WARNING,"receiving reassembled queue should be empty!");
                        
			recvBytes(datalen,pkt->userdata()); // notify application of "delivery"
			//
			// special code here to simulate the operation
			// of a receiver who always consumes data,
			// resulting in a call to tcp_output
			Packet::free(pkt);
			if (need_send())
				send_much(1, REASON_NORMAL, maxburst_);
			return;
		}
	} /* header prediction */


	//
	// header prediction failed
	// (e.g. pure ACK out of valid range, SACK present, etc)...
	// do slow path processing

	//
	// the following switch does special things for these states:
	//	TCPS_LISTEN, TCPS_SYN_SENT
	//

	switch (state_) {

        /*
         * If the segment contains an ACK then it is bad and do reset.
         * If it does not contain a SYN then it is not interesting; drop it.
         * Otherwise initialize tp->rcv_nxt, and tp->irs, iss is already
	 * selected, and send a segment:
         *     <SEQ=ISS><ACK=RCV_NXT><CTL=SYN,ACK>
         * Initialize tp->snd_nxt to tp->iss.
         * Enter SYN_RECEIVED state, and process any other fields of this
         * segment in this state.
         */

	case TCPS_LISTEN:	/* awaiting peer's SYN */

		if (tiflags & TH_ACK) {
                        if (debug_) {
		    	        fprintf(stderr,
		    		"%f: FullTcpAgent(%s): warning: recv'd ACK while in LISTEN: ",
				    now(), name());
			        prpkt(pkt);
                        }
			// don't want ACKs in LISTEN
			goto dropwithreset;
		}
		if ((tiflags & TH_SYN) == 0) {
                        if (debug_) {
		    	        fprintf(stderr, "%f: FullTcpAgent(%s): warning: recv'd NON-SYN while in LISTEN\n",
				now(), name());
			        prpkt(pkt);
                        }
			// any non-SYN is discarded
			goto drop;
		}

		/*
		 * must by a SYN (no ACK) at this point...
		 * in real tcp we would bump the iss counter here also
		 */
		dooptions(pkt);
		irs_ = tcph->seqno();
		t_seqno_ = iss_; /* tcp_sendseqinit() macro in real tcp */
		rcv_nxt_ = rcvseqinit(irs_, datalen);
		flags_ |= TF_ACKNOW;

		// check for a ECN-SYN with ECE|CWR
		if (ecn_ && fh->ecnecho() && fh->cong_action()) {
			ect_ = TRUE;
		}


		if (fid_ == 0) {
			// XXX: sort of hack... If we do not
			// have a special flow ID, pick up that
			// of the sender (active opener)
			hdr_ip* iph = hdr_ip::access(pkt);
			fid_ = iph->flowid();
		}

		newstate(TCPS_SYN_RECEIVED);
		goto trimthenstep6;

        /*
         * If the state is SYN_SENT:
         *      if seg contains an ACK, but not for our SYN, drop the input.
         *      if seg does not contain SYN, then drop it.
         * Otherwise this is an acceptable SYN segment
         *      initialize tp->rcv_nxt and tp->irs
         *      if seg contains ack then advance tp->snd_una
         *      if SYN has been acked change to ESTABLISHED else SYN_RCVD state
         *      arrange for segment to be acked (eventually)
         *      continue processing rest of data/controls, beginning with URG
         */

	case TCPS_SYN_SENT:	/* we sent SYN, expecting SYN+ACK (or SYN) */

		/* drop if it's a SYN+ACK and the ack field is bad */
		if ((tiflags & TH_ACK) &&
			((ackno <= iss_) || (ackno > maxseq_))) {
			// not an ACK for our SYN, discard
                        if (debug_) {
			       fprintf(stderr, "%f: FullTcpAgent::recv(%s): bad ACK for our SYN: ",
			        now(), name());
			        prpkt(pkt);
                        }
			goto dropwithreset;
		}

		if ((tiflags & TH_SYN) == 0) {
                        if (debug_) {
			        fprintf(stderr, "%f: FullTcpAgent::recv(%s): no SYN for our SYN: ",
			        now(), name());
			        prpkt(pkt);
                        }
			goto drop;
		}

		/* looks like an ok SYN or SYN+ACK */
                // If ecn_syn_wait is set to 2:
		// Check if CE-marked SYN/ACK packet, then just send an ACK
                //  packet with ECE set, and drop the SYN/ACK packet.
                //  Don't update TCP state. 
		if (tiflags & TH_ACK) 
		{
                        if (ecn_ && fh->ecnecho() && !fh->cong_action() && ecn_syn_wait_ == 2) 
                        // if SYN/ACK packet and ecn_syn_wait_ == 2
			{
	    		        if ( fh->ce() ) 
                                // If SYN/ACK packet is CE-marked
				{
					//cancel_rtx_timer();
					//newack(pkt);
					set_rtx_timer();
					sendpacket(t_seqno_, rcv_nxt_, TH_ACK|TH_ECE, 0, 0);
					goto drop;
				}
	    		}
		}


#ifdef notdef
cancel_rtx_timer();	// cancel timer on our 1st SYN [does this belong!?]
#endif
		irs_ = tcph->seqno();	// get initial recv'd seq #
		rcv_nxt_ = rcvseqinit(irs_, datalen);

		if (tiflags & TH_ACK) {
			// SYN+ACK (our SYN was acked)
                        if (ecn_ && fh->ecnecho() && !fh->cong_action()) {
                                ect_ = TRUE;
	    		        if ( fh->ce() ) 
	    				recent_ce_ = TRUE;
	    		}
                        
                        removeAckedPacket(highest_ack_,ackno);
			highest_ack_ = ackno;
			cwnd_ = initial_window();

#ifdef notdef
/*
 * if we didn't have to retransmit the SYN,
 * use its rtt as our initial srtt & rtt var.
 */
if (t_rtt_) {
	double tao = now() - tcph->ts();
	rtt_update(tao);
}
#endif

			/*
			 * if there's data, delay ACK; if there's also a FIN
			 * ACKNOW will be turned on later.
			 */
			if (datalen > 0) {
				flags_ |= TF_DELACK;	// data there: wait
			} else {
				flags_ |= TF_ACKNOW;	// ACK peer's SYN
			}

                        /*
                         * Received <SYN,ACK> in SYN_SENT[*] state.
                         * Transitions:
                         *      SYN_SENT  --> ESTABLISHED
                         *      SYN_SENT* --> FIN_WAIT_1
                         */

			if (flags_ & TF_NEEDFIN) {
				newstate(TCPS_FIN_WAIT_1);
				flags_ &= ~TF_NEEDFIN;
				tiflags &= ~TH_SYN;
			} else {
                            
                           // fprintf(stderr,"tcp established 2\n");
				newstate(TCPS_ESTABLISHED);
			}

			// special to ns:
			//  generate pure ACK here.
			//  this simulates the ordinary connection establishment
			//  where the ACK of the peer's SYN+ACK contains
			//  no data.  This is typically caused by the way
			//  the connect() socket call works in which the
			//  entire 3-way handshake occurs prior to the app
			//  being able to issue a write() [which actually
			//  causes the segment to be sent].
			sendpacket(t_seqno_, rcv_nxt_, TH_ACK, 0, 0);
		} else {
			// Check ECN-SYN packet
                        if (ecn_ && fh->ecnecho() && fh->cong_action())
                                ect_ = TRUE;

			// SYN (no ACK) (simultaneous active opens)
			flags_ |= TF_ACKNOW;
			cancel_rtx_timer();
			newstate(TCPS_SYN_RECEIVED);
			/*
			 * decrement t_seqno_: we are sending a
			 * 2nd SYN (this time in the form of a
			 * SYN+ACK, so t_seqno_ will have been
			 * advanced to 2... reduce this
			 */
			t_seqno_--;	// CHECKME
		}

trimthenstep6:
		/*
		 * advance the seq# to correspond to first data byte
		 */
		tcph->seqno()++;

		if (tiflags & TH_ACK)
			goto process_ACK;

		goto step6;

	case TCPS_LAST_ACK:
		/* 
		 * The only way we're in LAST_ACK is if we've already
		 * received a FIN, so ignore all retranmitted FINS.
		 * -M. Weigle 7/23/02
		 */
		if (tiflags & TH_FIN) {
			goto drop;
		}
		break;
	case TCPS_CLOSING:
		break;
	} /* end switch(state_) */

        /*
         * States other than LISTEN or SYN_SENT.
         * First check timestamp, if present.
         * Then check that at least some bytes of segment are within
         * receive window.  If segment begins before rcv_nxt,
         * drop leading data (and SYN); if nothing left, just ack.
         *
         * RFC 1323 PAWS: If we have a timestamp reply on this segment
         * and it's less than ts_recent, drop it.
         */

	if (ts_option_ && !fh->no_ts_ && recent_ && tcph->ts() < recent_) {
		if ((now() - recent_age_) > TCP_PAWS_IDLE) {
			/*
			 * this is basically impossible in the simulator,
			 * but here it is...
			 */
                        /*
                         * Invalidate ts_recent.  If this segment updates
                         * ts_recent, the age will be reset later and ts_recent
                         * will get a valid value.  If it does not, setting
                         * ts_recent to zero will at least satisfy the
                         * requirement that zero be placed in the timestamp
                         * echo reply when ts_recent isn't valid.  The
                         * age isn't reset until we get a valid ts_recent
                         * because we don't want out-of-order segments to be
                         * dropped when ts_recent is old.
                         */
			recent_ = 0.0;
		} else {
			fprintf(stderr, "%f: FullTcpAgent(%s): dropped pkt due to bad ts\n",
				now(), name());
			goto dropafterack;
		}
	}

	// check for redundant data at head/tail of segment
	//	note that the 4.4bsd [Net/3] code has
	//	a bug here which can cause us to ignore the
	//	perfectly good ACKs on duplicate segments.  The
	//	fix is described in (Stevens, Vol2, p. 959-960).
	//	This code is based on that correction.
	//
	// In addition, it has a modification so that duplicate segments
	// with dup acks don't trigger a fast retransmit when dupseg_fix_
	// is enabled.
	//
	// Yet one more modification: make sure that if the received
	//	segment had datalen=0 and wasn't a SYN or FIN that
	//	we don't turn on the ACKNOW status bit.  If we were to
	//	allow ACKNOW to be turned on, normal pure ACKs that happen
	//	to have seq #s below rcv_nxt can trigger an ACK war by
	//	forcing us to ACK the pure ACKs
	//
	// Update: if we have a dataless FIN, don't really want to
	// do anything with it.  In particular, would like to
	// avoid ACKing an incoming FIN+ACK while in CLOSING
	//
	todrop = rcv_nxt_ - tcph->seqno();  // how much overlap?

	if (todrop > 0 && ((tiflags & (TH_SYN)) || datalen > 0)) {
         printf("%f(%s): trim 1..todrop:%d, dlen:%d\n",now(), name(), todrop, datalen);
		if (tiflags & TH_SYN) {
			tiflags &= ~TH_SYN;
			tcph->seqno()++;
			th->size()--;	// XXX Must decrease packet size too!!
					// Q: Why?.. this is only a SYN
			todrop--;
		}
		//
		// see Stevens, vol 2, p. 960 for this check;
		// this check is to see if we are dropping
		// more than this segment (i.e. the whole pkt + a FIN),
		// or just the whole packet (no FIN)
		//
		if ((todrop > datalen) ||
		    (todrop == datalen && ((tiflags & TH_FIN) == 0))) {
//printf("%f(%s): trim 2..todrop:%d, dlen:%d\n",now(), name(), todrop, datalen);
                        /*
                         * Any valid FIN must be to the left of the window.
                         * At this point the FIN must be a duplicate or out
                         * of sequence; drop it.
                         */

			tiflags &= ~TH_FIN;

			/*
			 * Send an ACK to resynchronize and drop any data.
			 * But keep on processing for RST or ACK.
			 */

			flags_ |= TF_ACKNOW;
			todrop = datalen;
			dupseg = TRUE;	// *completely* duplicate

		}

		/*
		 * Trim duplicate data from the front of the packet
		 */

		tcph->seqno() += todrop;
		th->size() -= todrop;	// XXX Must decrease size too!!
					// why? [kf]..prob when put in RQ
		datalen -= todrop;

	} /* data trim */

	/*
	 * If we are doing timstamps and this packet has one, and
	 * If last ACK falls within this segment's sequence numbers,
	 * record the timestamp.
	 * See RFC1323 (now RFC1323 bis)
	 */
	if (ts_option_ && !fh->no_ts_ && tcph->seqno() <= last_ack_sent_) {
		/*
		 * this is the case where the ts value is newer than
		 * the last one we've seen, and the seq # is the one we expect
		 * [seqno == last_ack_sent_] or older
		 */
		recent_age_ = now();
		recent_ = tcph->ts();
	}

	if (tiflags & TH_SYN) {
                if (debug_) {
		        fprintf(stderr, "%f: FullTcpAgent::recv(%s) received unexpected SYN (state:%d): ",
		        now(), name(), state_);
		        prpkt(pkt);
                }
		goto dropwithreset;
	}

	if ((tiflags & TH_ACK) == 0) {
		/*
		 * Added check for state != SYN_RECEIVED.  We will receive a 
		 * duplicate SYN in SYN_RECEIVED when our SYN/ACK was dropped.
		 * We should just ignore the duplicate SYN (our timeout for 
		 * resending the SYN/ACK is about the same as the client's 
		 * timeout for resending the SYN), but give no error message. 
		 * -M. Weigle 07/24/01
		 */
		if (state_ != TCPS_SYN_RECEIVED) {
                        if (debug_) {
			        fprintf(stderr, "%f: FullTcpAgent::recv(%s) got packet lacking ACK (state:%d): ",
				now(), name(), state_);
			        prpkt(pkt);
                        }
		}
		goto drop;
	}

	/*
	 * Ack processing.
	 */

	switch (state_) {
	case TCPS_SYN_RECEIVED:	/* want ACK for our SYN+ACK */
		if (ackno < highest_ack_ || ackno > maxseq_) {
			// not in useful range
                        if (debug_) {
		    	        fprintf(stderr, "%f: FullTcpAgent(%s): ack(%d) not in range while in SYN_RECEIVED: ",
			 	now(), name(), ackno);
			        prpkt(pkt);
                        }
			goto dropwithreset;
		}

		if (ecn_ && ect_ && ecn_syn_ && fh->ecnecho() && ecn_syn_wait_ == 2) 
		{
		// The SYN/ACK packet was ECN-marked.
		// Reset the rtx timer, send another SYN/ACK packet
                //  immediately, and drop the ACK packet.
                // Do not move to TCPS_ESTB state or update TCP variables.
			cancel_rtx_timer();
			ecn_syn_next_ = 0;
			foutput(iss_, REASON_NORMAL);
			wnd_init_option_ = 1;
                        wnd_init_ = 1;
			goto drop;
		} 
		if (ecn_ && ect_ && ecn_syn_ && fh->ecnecho() && ecn_syn_wait_ < 2) {
		// The SYN/ACK packet was ECN-marked.
			if (ecn_syn_wait_ == 1) {
				// A timer will be called in ecn().
				cwnd_ = 1;
				use_rtt_ = 1; //KK, wait for timeout() period
			} else {
			        // Congestion window will be halved in ecn().
				cwnd_ = 2;
			}
		} else  {
			cwnd_ = initial_window();
		}
	
                /*
                 * Make transitions:
                 *      SYN-RECEIVED  -> ESTABLISHED
                 *      SYN-RECEIVED* -> FIN-WAIT-1
                 */
                if (flags_ & TF_NEEDFIN) {
			newstate(TCPS_FIN_WAIT_1);
                        flags_ &= ~TF_NEEDFIN;
                } else {
                    
                   // fprintf(stderr,"tcp established 1\n");
                        newstate(TCPS_ESTABLISHED);
                }

		/* fall into ... */


        /*
         * In ESTABLISHED state: drop duplicate ACKs; ACK out of range
         * ACKs.  If the ack is in the range
         *      tp->snd_una < ti->ti_ack <= tp->snd_max
         * then advance tp->snd_una to ti->ti_ack and drop
         * data from the retransmission queue.
	 *
	 * note that state TIME_WAIT isn't used
	 * in the simulator
         */

        case TCPS_ESTABLISHED:
        case TCPS_FIN_WAIT_1:
        case TCPS_FIN_WAIT_2:
	case TCPS_CLOSE_WAIT:
        case TCPS_CLOSING:
        case TCPS_LAST_ACK:

		//
		// look for ECNs in ACKs, react as necessary
		//

		if (fh->ecnecho() && (!ecn_ || !ect_)) {
			fprintf(stderr,
			    "%f: FullTcp(%s): warning, recvd ecnecho but I am not ECN capable!\n",
				now(), name());
		}

                //
                // generate a stream of ecnecho bits until we see a true
                // cong_action bit
                // 
                if (ecn_) {
                        if (fh->ce() && fh->ect())
                                recent_ce_ = TRUE;
                        else if (fh->cwr()) 
                                recent_ce_ = FALSE;
                }

		//
		// If ESTABLISHED or starting to close, process SACKS
		//

		if (state_ >= TCPS_ESTABLISHED && tcph->sa_length() > 0) {
			process_sack(tcph);
		}

		//
		// ACK indicates packet left the network
		//	try not to be fooled by data
		//

		if (fastrecov_ && (datalen == 0 || ackno > highest_ack_))
			pipe_ -= maxseg_;

		// look for dup ACKs (dup ack numbers, no data)
		//
		// do fast retransmit/recovery if at/past thresh
		if (ackno <= highest_ack_) {
			// a pure ACK which doesn't advance highest_ack_
			if (datalen == 0 && (!dupseg_fix_ || !dupseg)) {

                                /*
                                 * If we have outstanding data
                                 * this is a completely
                                 * duplicate ack,
                                 * the ack is the biggest we've
                                 * seen and we've seen exactly our rexmt
                                 * threshhold of them, assume a packet
                                 * has been dropped and retransmit it.
                                 *
                                 * We know we're losing at the current
                                 * window size so do congestion avoidance.
                                 *
                                 * Dup acks mean that packets have left the
                                 * network (they're now cached at the receiver)
                                 * so bump cwnd by the amount in the receiver
                                 * to keep a constant cwnd packets in the
                                 * network.
                                 */

				if ((rtx_timer_.status() != TIMER_PENDING) ||
				    ackno < highest_ack_) {
					// Q: significance of timer not pending?
					// ACK below highest_ack_
					oldack();
				} else if (++dupacks_ == tcprexmtthresh_) {
					// ACK at highest_ack_ AND meets threshold
					//trace_event("FAST_RECOVERY");
					dupack_action(); // maybe fast rexmt
					goto drop;

				} else if (dupacks_ > tcprexmtthresh_) {
					// ACK at highest_ack_ AND above threshole
					//trace_event("FAST_RECOVERY");
					extra_ack();

					// send whatever window allows
					send_much(0, REASON_DUPACK, maxburst_);
					goto drop;
				}
			} else {
				// non zero-length [dataful] segment
				// with a dup ack (normal for dataful segs)
				// (or window changed in real TCP).
				if (dupack_reset_) {
					dupacks_ = 0;
					fastrecov_ = FALSE;
				}
			}
			break;	/* take us to "step6" */
		} /* end of dup/old acks */

		/*
		 * we've finished the fast retransmit/recovery period
		 * (i.e. received an ACK which advances highest_ack_)
		 * The ACK may be "good" or "partial"
		 */

process_ACK:

		if (ackno > maxseq_) {
			// ack more than we sent(!?)
                        if (debug_) {
			        fprintf(stderr, "%f: FullTcpAgent::recv(%s) too-big ACK (maxseq:%d): ",
				now(), name(), int(maxseq_));
			        prpkt(pkt);
                        }
			goto dropafterack;
		}

                /*
                 * If we have a timestamp reply, update smoothed
                 * round trip time.  If no timestamp is present but
                 * transmit timer is running and timed sequence
                 * number was acked, update smoothed round trip time.
                 * Since we now have an rtt measurement, cancel the
                 * timer backoff (cf., Phil Karn's retransmit alg.).
                 * Recompute the initial retransmit timer.
		 *
                 * If all outstanding data is acked, stop retransmit
                 * If there is more data to be acked, restart retransmit
                 * timer, using current (possibly backed-off) value.
                 */
		newack(pkt);	// handle timers, update highest_ack_

		/*
		 * if this is a partial ACK, invoke whatever we should
		 * note that newack() must be called before the action
		 * functions, as some of them depend on side-effects
		 * of newack()
		 */

		int partial = pack(pkt);

		if (partial)
			pack_action(pkt);
		else
			ack_action(pkt);

		/*
		 * if this is an ACK with an ECN indication, handle this
		 * but not if it is a syn packet
		 */
		if (fh->ecnecho() && !(tiflags&TH_SYN) )
		if (fh->ecnecho()) {
			// Note from Sally: In one-way TCP,
			// ecn() is called before newack()...
			ecn(highest_ack_);  // updated by newack(), above
			// "set_rtx_timer();" from T. Kelly.
			if (cwnd_ < 1)
			 	set_rtx_timer();
		}
		// CHECKME: handling of rtx timer
		if (ackno == maxseq_) {
			needoutput = TRUE;
		}

		/*
		 * If no data (only SYN) was ACK'd,
		 *    skip rest of ACK processing.
		 */
		if (ackno == (highest_ack_ + 1))
			goto step6;

		// if we are delaying initial cwnd growth (probably due to
		// large initial windows), then only open cwnd if data has
		// been received
		// Q: check when this happens
                /*
                 * When new data is acked, open the congestion window.
                 * If the window gives us less than ssthresh packets
                 * in flight, open exponentially (maxseg per packet).
                 * Otherwise open about linearly: maxseg per window
                 * (maxseg^2 / cwnd per packet).
                 */
		if ((!delay_growth_ || (rcv_nxt_ > 0)) &&
		    last_state_ == TCPS_ESTABLISHED) {
			if (!partial || open_cwnd_on_pack_) {
                           if (!ect_ || !hdr_flags::access(pkt)->ecnecho())
				opencwnd();
                        }
		}

		if ((state_ >= TCPS_FIN_WAIT_1) && (ackno == maxseq_)) {
			ourfinisacked = TRUE;
		}

		//
		// special additional processing when our state
		// is one of the closing states:
		//	FIN_WAIT_1, CLOSING, LAST_ACK

		switch (state_) {
                /*
                 * In FIN_WAIT_1 STATE in addition to the processing
                 * for the ESTABLISHED state if our FIN is now acknowledged
                 * then enter FIN_WAIT_2.
                 */
		case TCPS_FIN_WAIT_1:	/* doing active close */
			if (ourfinisacked) {
				// got the ACK, now await incoming FIN
				newstate(TCPS_FIN_WAIT_2);
				cancel_timers();
				needoutput = FALSE;
			}
			break;

                /*
                 * In CLOSING STATE in addition to the processing for
                 * the ESTABLISHED state if the ACK acknowledges our FIN
                 * then enter the TIME-WAIT state, otherwise ignore
                 * the segment.
                 */
		case TCPS_CLOSING:	/* simultaneous active close */;
			if (ourfinisacked) {
				newstate(TCPS_CLOSED);
				cancel_timers();
			}
			break;
                /*
                 * In LAST_ACK, we may still be waiting for data to drain
                 * and/or to be acked, as well as for the ack of our FIN.
                 * If our FIN is now acknowledged,
                 * enter the closed state and return.
                 */
		case TCPS_LAST_ACK:	/* passive close */
			// K: added state change here
			if (ourfinisacked) {
				newstate(TCPS_CLOSED);
				finish(); // cancels timers, erc
				reset(); // for connection re-use (bug fix from ns-users list)
				goto drop;
			} else {
				// should be a FIN we've seen
                                if (debug_) {
                                        fprintf(stderr, "%f: FullTcpAgent(%s)::received non-ACK (state:%d): ",
                                        now(), name(), state_);
				        prpkt(pkt);
                                }
                        }
			break;

		/* no case for TIME_WAIT in simulator */
		}  // inner state_ switch (closing states)
	} // outer state_ switch (ack processing)

step6:

	/*
	 * Processing of incoming DATAful segments.
	 * 	Code above has already trimmed redundant data.
	 *
	 * real TCP handles window updates and URG data here also
	 */

/* dodata: this label is in the "real" code.. here only for reference */

	if ((datalen > 0 || (tiflags & TH_FIN)) &&
	    TCPS_HAVERCVDFIN(state_) == 0) {

		//
		// the following 'if' implements the "real" TCP
		// TCP_REASS macro
		//

		if (tcph->seqno() == rcv_nxt_ && rq_.empty()) {
			// got the in-order packet we were looking
			// for, nobody is in the reassembly queue,
			// so this is the common case...
			// note: in "real" TCP we must also be in
			// ESTABLISHED state to come here, because
			// data arriving before ESTABLISHED is
			// queued in the reassembly queue.  Since we
			// don't really have a process anyhow, just
			// accept the data here as-is (i.e. don't
			// require being in ESTABLISHED state)
			flags_ |= TF_DELACK;
			rcv_nxt_ += datalen;
			tiflags = tcph->flags() & TH_FIN;

			// give to "application" here
			// in "real" TCP, this is sbappend() + sorwakeup()
                        
                        if(!_recvq.empty())
                            
                           Log(DASH_LOG_WARNING,"receiving reassembled queue should be empty!");
                                
			if (datalen>0)
                            
                            recvBytes(datalen,pkt->userdata()); // notify app. of "delivery"
                        
			needoutput = need_send();
		} else {
			// see the "tcp_reass" function:
			// not the one we want next (or it
			// is but there's stuff on the reass queue);
			// do whatever we need to do for out-of-order
			// segments or hole-fills.  Also,
			// send an ACK (or SACK) to the other side right now.
			// Note that we may have just a FIN here (datalen = 0)
			int rcv_nxt_old_ = rcv_nxt_; // notify app. if changes
			tiflags = reass(pkt);
                       
                        if(datalen>0)
                        addtoReceiveQueue(datalen, tcph->seqno(),pkt->userdata());
                        
                        
			if (rcv_nxt_ > rcv_nxt_old_) {
				// if rcv_nxt_ has advanced, must have
				// been a hole fill.  In this case, there
				// is something to give to application
				//recvBytes(rcv_nxt_ - rcv_nxt_old_);
                            findReceivedData(rcv_nxt_old_,rcv_nxt_);
			}
			flags_ |= TF_ACKNOW;

			if (tiflags & TH_PUSH) {
				//
				// ???: does this belong here
				// K: APPLICATION recv
				needoutput = need_send();
			}
		}
	} else {
		/*
		 * we're closing down or this is a pure ACK that
		 * wasn't handled by the header prediction part above
		 * (e.g. because cwnd < wnd)
		 */
		// K: this is deleted
		tiflags &= ~TH_FIN;
	}

	/*
	 * if FIN is received, ACK the FIN
	 * (let user know if we could do so)
	 */

	if (tiflags & TH_FIN) {
		if (TCPS_HAVERCVDFIN(state_) == 0) {
			flags_ |= TF_ACKNOW;
 			rcv_nxt_++;
		}
		switch (state_) {
                /*
                 * In SYN_RECEIVED and ESTABLISHED STATES
                 * enter the CLOSE_WAIT state.
		 * (passive close)
                 */
                case TCPS_SYN_RECEIVED:
                case TCPS_ESTABLISHED:
			newstate(TCPS_CLOSE_WAIT);
                        break;

                /*
                 * If still in FIN_WAIT_1 STATE FIN has not been acked so
                 * enter the CLOSING state.
		 * (simultaneous close)
                 */
                case TCPS_FIN_WAIT_1:
			newstate(TCPS_CLOSING);
                        break;
                /*
                 * In FIN_WAIT_2 state enter the TIME_WAIT state,
                 * starting the time-wait timer, turning off the other
                 * standard timers.
		 * (in the simulator, just go to CLOSED)
		 * (completion of active close)
                 */
                case TCPS_FIN_WAIT_2:
                        newstate(TCPS_CLOSED);
			cancel_timers();
                        break;
		}
	} /* end of if FIN bit on */

	if (needoutput || (flags_ & TF_ACKNOW))
		send_much(1, REASON_NORMAL, maxburst_);
	else if (curseq_ >= highest_ack_ || infinite_send_)
		send_much(0, REASON_NORMAL, maxburst_);
	// K: which state to return to when nothing left?

	if (!halfclose_ && state_ == TCPS_CLOSE_WAIT && highest_ack_ == maxseq_)
		usrclosed();

	Packet::free(pkt);

	// haoboy: Is here the place for done{} of active close? 
	// It cannot be put in the switch above because we might need to do
	// send_much() (an ACK)
	if (state_ == TCPS_CLOSED) 
		Tcl::instance().evalf("%s done", this->name());

	return;

	//
	// various ways of dropping (some also ACK, some also RST)
	//

dropafterack:
	flags_ |= TF_ACKNOW;
	send_much(1, REASON_NORMAL, maxburst_);
	goto drop;

dropwithreset:
	/* we should be sending an RST here, but can't in simulator */
	if (tiflags & TH_ACK) {
		sendpacket(ackno, 0, 0x0, 0, REASON_NORMAL);
	} else {
		int ack = tcph->seqno() + datalen;
		if (tiflags & TH_SYN)
			ack--;
		sendpacket(0, ack, TH_ACK, 0, REASON_NORMAL);
	}
drop:
   	Packet::free(pkt);
	return;
}

/*appdata should be replicated*/
void DHttpAgent::addtoReceiveQueue(int datalen, int seq_no, AppData* data)
{
    TCPBufferedData tcpdata;
    tcpdata.data=data->copy();
    tcpdata.length=datalen;
    tcpdata.start_seq=seq_no;
    
    if(_recvq.empty())
    _recvq.push_back(tcpdata);
    else
    {
        list<TCPBufferedData>::iterator it;
        for(it=_recvq.begin();it!=_recvq.end();it++)
        {
            if((*it).start_seq>seq_no)
                break;
        }
        
        _recvq.insert(it,tcpdata);
        
    }
}


void DHttpAgent::recvBytes(int datalen, AppData* data)
{
    if(app_)
        app_->recv(datalen,data);
}

void DHttpAgent::sendmsg(int sz, AppData* data, const char* flags)
{
    
    _send_applicationq.push_back(data->copy());
    
    
    FullTcpAgent::sendmsg(sz,NULL);
    
}

void DHttpAgent::findReceivedData(int start_seq, int end_seq)
{
    
    
    list<TCPBufferedData>::iterator it=_recvq.begin();
    
    if((*it).start_seq!=start_seq)
        
    {
        Log(DASH_LOG_FATAL,"Error during retrieving cached app data at TCP queue, first cached packets should have seq no %d, but has %d",
                start_seq,(*it).start_seq);
        
        return;
    }
    while(!_recvq.empty())
    {
       
        TCPBufferedData data=_recvq.front();
        if(data.start_seq>=start_seq && data.start_seq<end_seq)
        {
            _recvq.pop_front();
            recvBytes(data.data->size(),data.data);
            
            delete data.data;
        }
        else
            break;
    }
    
}


void
DHttpAgent::sendpacket(int seqno, int ackno, int pflags, int datalen, int reason, Packet *p)
{
    
    AppData *data=NULL;
    if(datalen>0) /*zero data length means ACK,SYN,FIN etc control packets and therefore has nothing to do with application data*/
    {
      
        if(_is_retransmit)
        {
            
            
            data=getRetransmitData(seqno,datalen);
            if(data==NULL)
            {
                Log(DASH_LOG_FATAL,"cannot find in TCP retransmit queue packet with seqno %d",seqno);
                return;
            }
            
        }
        else
        {
            if(_send_applicationq.empty())
            {
                Log(DASH_LOG_FATAL,"nothing in TCP app sending queue but still need to send %d bytes data",datalen);
                return;
            }
            
            
            AppData *app_data=_send_applicationq.front();
            _send_applicationq.pop_front();
             
             
             if(app_data->size()>datalen)
             {
                 /*make sure this is a media packet*/
                 DashMediaChunk *chunk=dynamic_cast<DashMediaChunk*>(app_data);
                 if(chunk==NULL)
                 {
                     Log(DASH_LOG_FATAL,"non-media packet has a size of %d while the tcp packet size is bounded at %d",app_data->size(),datalen);
                     return;
                 }
                 
                 Log(DASH_LOG_DEBUG,"media data has a size %d while datalen is %d",app_data->size(),datalen);
                 
                /*create a new packet that takes part of the application data*/
                 data=new DashMediaChunk(*chunk,datalen);
                 
                        
                 
                 /*update the old chunk and put it back into the head of the list*/
                
                  chunk->_media_packet_size-=datalen-sizeof(DashMediaChunk);
                 
                 
                 
                  chunk->_tcp_segment_id++;
                  
                  Log(DASH_LOG_DEBUG,"media packet size left: %d, tcp segment id %d, media size cut %d",chunk->_media_packet_size,chunk->_tcp_segment_id,datalen-sizeof(DashMediaChunk));
                  
                  _send_applicationq.push_front(app_data);
                  
                  
                 
                 
             }
             else
                 data=app_data;
            
            
            TCPBufferedData tbd;
            tbd.data=data;
            tbd.start_seq=seqno;
            tbd.length=data->size();
            
            _send_waitingq.push_back(tbd);
            
            
            
        }
        
        
      
     
    }
        
    
    
    
        if (!p) p = allocpkt();
        hdr_tcp *tcph = hdr_tcp::access(p);
	hdr_flags *fh = hdr_flags::access(p);

	/* build basic header w/options */

        tcph->seqno() = seqno;
        tcph->ackno() = ackno;
        tcph->flags() = pflags;
        tcph->reason() |= reason; // make tcph->reason look like ns1 pkt->flags?
	tcph->sa_length() = 0;    // may be increased by build_options()
        tcph->hlen() = tcpip_base_hdr_size_;
	tcph->hlen() += build_options(tcph);

	/*
	 * Explicit Congestion Notification (ECN) related:
	 * Bits in header:
	 * 	ECT (EC Capable Transport),
	 * 	ECNECHO (ECHO of ECN Notification generated at router),
	 * 	CWR (Congestion Window Reduced from RFC 2481)
	 * States in TCP:
	 *	ecn_: I am supposed to do ECN if my peer does
	 *	ect_: I am doing ECN (ecn_ should be T and peer does ECN)
	 */

	if (datalen > 0 && ecn_ ){
	        // set ect on data packets 
		fh->ect() = ect_;	// on after mutual agreement on ECT
        } else if (ecn_ && ecn_syn_ && ecn_syn_next_ && (pflags & TH_SYN) && (pflags & TH_ACK)) {
                // set ect on syn/ack packet, if syn packet was negotiating ECT
               	fh->ect() = ect_;
	} else {
		/* Set ect() to 0.  -M. Weigle 1/19/05 */
		fh->ect() = 0;
	}
	if (ecn_ && ect_ && recent_ce_ ) { 
		// This is needed here for the ACK in a SYN, SYN/ACK, ACK
		// sequence.
		pflags |= TH_ECE;
	}
        // fill in CWR and ECE bits which don't actually sit in
        // the tcp_flags but in hdr_flags
        if ( pflags & TH_ECE) {
                fh->ecnecho() = 1;
        } else {
                fh->ecnecho() = 0;
        }
        if ( pflags & TH_CWR ) {
                fh->cong_action() = 1;
        }
	else {
		/* Set cong_action() to 0  -M. Weigle 1/19/05 */
		fh->cong_action() = 0;
	}

	/* actual size is data length plus header length */

        hdr_cmn *ch = hdr_cmn::access(p);
        ch->size() = datalen + tcph->hlen();

        if (datalen <= 0)
                ++nackpack_;
        else {
                ++ndatapack_;
                ndatabytes_ += datalen;
		last_send_time_ = now();	// time of last data
        }
        if (reason == REASON_TIMEOUT || reason == REASON_DUPACK || reason == REASON_SACK) {
                ++nrexmitpack_;
                nrexmitbytes_ += datalen;
        }

	last_ack_sent_ = ackno;

//if (state_ != TCPS_ESTABLISHED) {
//printf("%f(%s)[state:%s]: sending pkt ", now(), name(), statestr(state_));
//prpkt(p);
//}
        
        if(datalen>0 )
        {
            if(data!=NULL)
            {
            p->setdata(data->copy());
            ch->ptype_=PT_HTTP;
            }
            
            else
            ch->ptype_=PT_TCP;
            
            
            
            
            
        }
        
        else if(!(pflags & TH_ACK))
            ch->ptype_=PT_TCP;
        
       // fprintf(stderr,"in http agent %d ",this->getfid());
       
	send(p, 0);
        
        
       

	return;
}

int DHttpAgent::foutput(int seqno, int reason)
{
    _is_retransmit=(seqno < maxseq_);
    
    // if maxseg_ not set, set it appropriately
	// Q: how can this happen?

	if (maxseg_ == 0) 
	   	maxseg_ = size_ - headersize();
	else
		size_ =  maxseg_ + headersize();

	int is_retransmit = (seqno < maxseq_);
	int quiet = (highest_ack_ == maxseq_);
	int pflags = outflags();
	int syn = (seqno == iss_);
	int emptying_buffer = FALSE;
	int buffered_bytes = (infinite_send_) ? TCP_MAXSEQ :
				curseq_ - highest_ack_ + 1;

	int win = window() * maxseg_;	// window (in bytes)
	int off = seqno - highest_ack_;	// offset of seg in window
	int datalen;
	//int amtsent = 0;

	// be careful if we have not received any ACK yet
	if (highest_ack_ < 0) {
		if (!infinite_send_)
			buffered_bytes = curseq_ - iss_;;
		off = seqno - iss_;
	}

	if (syn && !data_on_syn_)
		datalen = 0;
	else if (pipectrl_)
		datalen = buffered_bytes - off;
	else
		datalen = min(buffered_bytes, win) - off;

        if ((signal_on_empty_) && (!buffered_bytes) && (!syn))
	                bufferempty();
       
       
	//
	// in real TCP datalen (len) could be < 0 if there was window
	// shrinkage, or if a FIN has been sent and neither ACKd nor
	// retransmitted.  Only this 2nd case concerns us here...
	//
	if (datalen < 0) {
		datalen = 0;
	} else if (datalen > maxseg_) {
		datalen = maxseg_;
	}

	//
	// this is an option that causes us to slow-start if we've
	// been idle for a "long" time, where long means a rto or longer
	// the slow-start is a sort that does not set ssthresh
	//

	if (slow_start_restart_ && quiet && datalen > 0) {
		if (idle_restart()) {
			slowdown(CLOSE_CWND_INIT);
		}
	}

	//
	// see if sending this packet will empty the send buffer
	// a dataless SYN packet counts also
	//

	if (!infinite_send_ && ((seqno + datalen) > curseq_ || 
	    (syn && datalen == 0))) {
		emptying_buffer = TRUE;
		//
		// if not a retransmission, notify application that 
		// everything has been sent out at least once.
		//
		if (!syn) {
			idle();
			if (close_on_empty_ && quiet) {
				flags_ |= TF_NEEDCLOSE;
			}
		}
		pflags |= TH_PUSH;
		//
		// if close_on_empty set, we are finished
		// with this connection; close it
		//
	} else  {
		/* not emptying buffer, so can't be FIN */
		pflags &= ~TH_FIN;
	}
	if (infinite_send_ && (syn && datalen == 0))
		pflags |= TH_PUSH;  // set PUSH for dataless SYN

	/* sender SWS avoidance (Nagle) */

	if (datalen > 0) {
		// if full-sized segment, ok
		if (datalen == maxseg_)
			goto send;
		// if Nagle disabled and buffer clearing, ok
		if ((quiet || nodelay_)  && emptying_buffer)
			goto send;
		// if a retransmission
		if (is_retransmit)
			goto send;
		// if big "enough", ok...
		//	(this is not a likely case, and would
		//	only happen for tiny windows)
		if (datalen >= ((wnd_ * maxseg_) / 2.0))
			goto send;
	}

	if (need_send())
		goto send;

	/*
	 * send now if a control packet or we owe peer an ACK
	 * TF_ACKNOW can be set during connection establishment and
	 * to generate acks for out-of-order data
	 */

	if ((flags_ & (TF_ACKNOW|TF_NEEDCLOSE)) ||
	    (pflags & (TH_SYN|TH_FIN))) {
		goto send;
	}

        /*      
         * No reason to send a segment, just return.
         */      
	return 0;

send:

	// is a syn or fin?

	syn = (pflags & TH_SYN) ? 1 : 0;
	int fin = (pflags & TH_FIN) ? 1 : 0;

        /* setup ECN syn and ECN SYN+ACK packet headers */
        if (ecn_ && syn && !(pflags & TH_ACK)){
                pflags |= TH_ECE;
                pflags |= TH_CWR;
        }
        if (ecn_ && syn && (pflags & TH_ACK)){
                pflags |= TH_ECE;
                pflags &= ~TH_CWR;
        }
	else if (ecn_ && ect_ && cong_action_ && 
	             (!is_retransmit || SetCWRonRetransmit_)) {
		/* 
		 * Don't set CWR for a retranmitted SYN+ACK (has ecn_ 
		 * and cong_action_ set).
		 * -M. Weigle 6/19/02
                 *
                 * SetCWRonRetransmit_ was changed to true,
                 * allowing CWR on retransmitted data packets.  
                 * See test ecn_burstyEcn_reno_full 
                 * in test-suite-ecn-full.tcl.
		 * - Sally Floyd, 6/5/08.
		 */
		/* set CWR if necessary */
		pflags |= TH_CWR;
		/* Turn cong_action_ off: Added 6/5/08, Sally Floyd. */
		cong_action_ = FALSE;
	}

	/* moved from sendpacket()  -M. Weigle 6/19/02 */
	//
	// although CWR bit is ordinarily associated with ECN,
	// it has utility within the simulator for traces. Thus, set
	// it even if we aren't doing ECN
	//
	if (datalen > 0 && cong_action_ && !is_retransmit) {
		pflags |= TH_CWR;
	}
  
        /* set ECE if necessary */
        if (ecn_ && ect_ && recent_ce_ ) {
		pflags |= TH_ECE;
	}

        /* 
         * Tack on the FIN flag to the data segment if close_on_empty_
         * was previously set-- avoids sending a separate FIN
         */ 
        if (flags_ & TF_NEEDCLOSE) {
                flags_ &= ~TF_NEEDCLOSE;
                if (state_ <= TCPS_ESTABLISHED && state_ != TCPS_CLOSED)
                {
                    pflags |=TH_FIN;
                    fin = 1;  /* FIN consumes sequence number */
                    newstate(TCPS_FIN_WAIT_1);
                }
        }
	sendpacket(seqno, rcv_nxt_, pflags, datalen, reason);

        /*      
         * Data sent (as far as we can tell).
         * Any pending ACK has now been sent.
         */      
	flags_ &= ~(TF_ACKNOW|TF_DELACK);

	/*
	 * if we have reacted to congestion recently, the
	 * slowdown() procedure will have set cong_action_ and
	 * sendpacket will have copied that to the outgoing pkt
	 * CWR field. If that packet contains data, then
	 * it will be reliably delivered, so we are free to turn off the
	 * cong_action_ state now  If only a pure ACK, we keep the state
	 * around until we actually send a segment
	 */

	int reliable = datalen + syn + fin; // seq #'s reliably sent
	/* 
	 * Don't reset cong_action_ until we send new data.
	 * -M. Weigle 6/19/02
	 */
	if (cong_action_ && reliable > 0 && !is_retransmit)
		cong_action_ = FALSE;

	// highest: greatest sequence number sent + 1
	//	and adjusted for SYNs and FINs which use up one number

	int highest = seqno + reliable;
	if (highest > maxseq_) {
		maxseq_ = highest;
		//
		// if we are using conventional RTT estimation,
		// establish timing on this segment
		//
		if (!ts_option_ && rtt_active_ == FALSE) {
			rtt_active_ = TRUE;	// set timer
			rtt_seq_ = seqno; 	// timed seq #
			rtt_ts_ = now();	// when set
		}
	}

	/*
	 * Set retransmit timer if not currently set,
	 * and not doing an ack or a keep-alive probe.
	 * Initial value for retransmit timer is smoothed
	 * round-trip time + 2 * round-trip time variance.
	 * Future values are rtt + 4 * rttvar.
	 */
	if (rtx_timer_.status() != TIMER_PENDING && reliable) {
		set_rtx_timer();  // no timer pending, schedule one
	}

	return (reliable);
}



void DHttpAgent::Log(int log_level, const char* fmt, ...)
{
    
    va_list arg_list;
    va_start(arg_list,fmt);
    DashLog(stdout,log_level,_default_log_level,DASH_COLOR_CYAN,"HTTP-AGENT ", fmt,arg_list);
    va_end(arg_list);
    
    if(log_level==DASH_LOG_FATAL)
        exit(0);
}

AppData* DHttpAgent::getRetransmitData(int seq_no, int datalen)
{
    
    if(_send_waitingq.empty())
    {
        Log(DASH_LOG_WARNING,"retransmit queue is empty!");
        return NULL;
        
    }
    
    list<TCPBufferedData>::iterator it;
    for(it=_send_waitingq.begin();it!=_send_waitingq.end();it++)
    {
        if((*it).start_seq==seq_no && (*it).length<=datalen)
            return (*it).data;
        
    }
    
    return NULL;
}


void
DHttpAgent::newack(Packet* pkt)
{
    
        int old_highest_ack=highest_ack_;
    
	hdr_tcp *tcph = hdr_tcp::access(pkt);

	register int ackno = tcph->ackno();
	int progress = (ackno > highest_ack_);

	if (ackno == maxseq_) {
		cancel_rtx_timer();	// all data ACKd
	} else if (progress) {
		set_rtx_timer();
	}

	// advance the ack number if this is for new data
	if (progress)
		highest_ack_ = ackno;

	// if we have suffered a retransmit timeout, t_seqno_
	// will have been reset to highest_ ack.  If the
	// receiver has cached some data above t_seqno_, the
	// new-ack value could (should) jump forward.  We must
	// update t_seqno_ here, otherwise we would be doing
	// go-back-n.

	if (t_seqno_ < highest_ack_)
		t_seqno_ = highest_ack_; // seq# to send next
        
        
        
        removeAckedPacket(old_highest_ack,highest_ack_);

        /*
         * Update RTT only if it's OK to do so from info in the flags header.
         * This is needed for protocols in which intermediate agents

         * in the network intersperse acks (e.g., ack-reconstructors) for
         * various reasons (without violating e2e semantics).
         */
        hdr_flags *fh = hdr_flags::access(pkt);

	if (!fh->no_ts_) {
                if (ts_option_) {
			recent_age_ = now();
			recent_ = tcph->ts();
			rtt_update(now() - tcph->ts_echo());
			if (ts_resetRTO_ && (!ect_ || !ecn_backoff_ ||
		           !hdr_flags::access(pkt)->ecnecho())) { 
				// From Andrei Gurtov
				//
                         	// Don't end backoff if still in ECN-Echo with
                         	// a congestion window of 1 packet.
				t_backoff_ = 1;
			}
		} else if (rtt_active_ && ackno > rtt_seq_) {
			// got an RTT sample, record it
			// "t_backoff_ = 1;" deleted by T. Kelly.
			rtt_active_ = FALSE;
			rtt_update(now() - rtt_ts_);
                }
		if (!ect_ || !ecn_backoff_ ||
		    !hdr_flags::access(pkt)->ecnecho()) {
			/*
			 * Don't end backoff if still in ECN-Echo with
			 * a congestion window of 1 packet.
			 * Fix from T. Kelly.
			 */
			t_backoff_ = 1;
			ecn_backoff_ = 0;
		}

        }
	return;
}

void DHttpAgent::removeAckedPacket(int old_ack_no, int new_ack_no)
{
    if(old_ack_no>new_ack_no)
    {
        Log(DASH_LOG_ERROR,"error packet seqence number for removal: from %d to %d",old_ack_no,new_ack_no);
        return;
    }
    
    if(old_ack_no==new_ack_no)
    {
        Log(DASH_LOG_NOTE,"new ack number remains same, no packet to remove: %d",new_ack_no);
        return;
    }
    
    if(_send_waitingq.empty())
        return;
    
    
   // Log(DASH_LOG_DEBUG,"removing packets from %d to %d",old_ack_no,new_ack_no);
    list<TCPBufferedData>::iterator it;
    for(it=_send_waitingq.begin();it!=_send_waitingq.end();)
    {
        
        
        if( (*it).start_seq<old_ack_no)
        {
            Log(DASH_LOG_FATAL,"there is packet with seq number smaller than %d: %d",old_ack_no,(*it).start_seq);
            return;
        }
        if((*it).start_seq>=old_ack_no && (*it).start_seq<new_ack_no)
        {   
           // Log(DASH_LOG_DEBUG,"find packet with seq %d, length %d",(*it).start_seq,(*it).length);
            TCPBufferedData data=*it;
            delete data.data;
            it=_send_waitingq.erase(it);
        }
        else
            it++;
        
        
    }
    
    
    
    
}

void DHttpAgent::finish()
{
    Tcl::instance().evalf("%s done", this->name());
}