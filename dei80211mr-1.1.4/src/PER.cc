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

/* -*-	Mode:C++; c-basic-offset:3; tab-width:3; indent-tabs-mode:t -*- */

#include "mac.h"
#include "mac-802_11mr.h"
#include "PER.h"
#include <math.h>
#include <float.h>
#include <rng.h>

#define ABS(x) (((x)>=0) ? (x) : (-x) )


const double WLAN_SIR_PERFECT=10.0;
const double WLAN_SIR_IMPOSSIBLE=0.1;

//Completion for the ADT


/// Container for the length	
class leng : public Object {
	public:
	int len;
	DLList* snr_list;

	leng(int len_){
		len=len_;
		snr_list= new DLList();
	}
	
	~leng() {
		delete snr_list;	
	}
	
};


/// Container for SNR and PER
class Snr : public Object {
	public:
	double snr;
	double per;
	
	Snr(double snr_, double per_){
		snr= snr_;
		per= per_;
	}

};


/**
 * TCL wrapper for the PER class
 * 
 */
static class PERClass : public TclClass {
public:
	PERClass() : TclClass("PER") {}
	TclObject* create(int, const char*const*) {
		return (new PER());
	}
} class_per;


PER::PER()
{
	for(int i = 0 ; i < NumPhyModes ; i++)
		PHYmod[i] = new DLList();

#ifndef COMPILING_TEST_PROGRAM
	bind("debug_", &debug_);
	bind("noise_", &noise_);
#endif

}

PER::~PER(){
 	for(int i = 0 ; i < NumPhyModes ; i++)
		delete PHYmod[i];
}



int PER::command(int argc, const char*const* argv){

	if(debug_>2) printf("Called PER::command argc=%i\n",argc);

	if (argc == 2) {	
		if (strcmp(argv[1], "print") == 0) {
		print_data();
		 return TCL_OK;
	}
	} else if (argc == 6) {

		// Command to add PER entries to PER table
		if (strcmp(argv[1], "add") == 0) {
                       
			PhyMode pm = str2PhyMode(argv[2]);

			if (pm==ModeUnknown) return(TCL_ERROR);

			int len =   atoi(argv[3]);
			double snr = atof(argv[4]);
			double  per = atof(argv[5]);

			if(debug_>2)  printf("%s=%f ------- %s=%f\n",argv[4],snr,argv[5],per);
			
			set_per(pm,len,snr,per);
			return TCL_OK;		   
		}
	}
        
        
        return 1;
}



int PER::get_err(PowerProfile* pp, Packet* pkt, double* snrptr, double* snirptr, double* interfptr){


	if(debug_>3)
		{

			printf("PER::get_err(): dumping everything\n");
			
			for (int i=0; i< 12; i++)
				{
					if(PHYmod[i]!=0)
						{
							NSNode* nl  = (NSNode*)(PHYmod[i]->first());
							while(nl !=0  )
								{
									cout << ((leng*)nl->element())->len << "\n";
									DLList* sl = ((leng*)nl->element())->snr_list;
									NSNode* ns =(NSNode*)sl->first();
									while( ns !=0 )
										{												
											cout << ((Snr*)ns->element())->snr << "  " ;
											cout << ((Snr*)ns->element())->per << "\n" ;
											ns  = (NSNode*)(sl->after(ns));
										}
									nl  = (NSNode*)(PHYmod[i]->after(nl));
								}
							
							
							
						}
				}
		}

	/* Hack needed not to link the whole NS with the test program */ 
#ifdef COMPILING_TEST_PROGRAM         
	double pkt_pow = 0 ;
	int len = 0;
	PhyMode pm=Mode1Mb;
	double duration = 1;
#else        
	struct hdr_cmn *hdr = HDR_CMN(pkt);
	struct MultiRateHdr *hdm = HDR_MULTIRATE(pkt);
	double pkt_pow = pkt->txinfo_.RxPr;
	double duration = hdr->txtime(); 
	PhyMode pm = hdm->mode(); 
	int len = hdr->size(); 
#endif

	double avgpower = avg(pp,duration);
	double interf = avgpower-pkt_pow;
	double snr1=DBL_MAX; 
	double snr2;
	double per1=0.0;
	double per2=0.0;


	/*if (ABS(noise_) > DBL_MIN)
		{
			snr1 = 10*log10(pkt_pow/(noise_));
			per1 = get_per_ns3( pm, pkt_pow/noise_, len);
		}
	else
		per1 = 0;

	if (ABS(noise_ + interf) > DBL_MIN)
		{
			snr2 = 10*log10(pkt_pow/(noise_ + interf));
			per2 = get_per_ns3( pm, pkt_pow/(noise_+interf), len);
		}
	else
		per2=0;
         */
        
        double thermal_noise;
        calculate_snr(pm,pkt_pow,&interf,&snr1,&snr2,&thermal_noise);
        
        per1=get_per_ns3(pm,snr1,len);
        per2=get_per_ns3(pm,snr2,len);
        
       // fprintf(stderr,"PER is %f for mode %s\n",per1,PhyMode2str(pm));

        /*change unit to log level*/
        snr1=10*log10(snr1);
        snr2=10*log10(snr2);
        
        
	/* Return SNR */
	if (snrptr!=NULL)
		*snrptr = snr1;

	/* Return SNIR */
	if (snirptr!=NULL)
		*snirptr = snr2;

	/* Return interfering power */
	if (interfptr!=NULL)
		*interfptr = interf;

	//	double x = (double) rand()/(double)RAND_MAX;
	double x = RNG::defaultrng()->uniform_double();

	//if ((debug_)&&(interf>1e-15))
	if ((debug_))
        
		{       
			cout << "PER::get_err() at " << Scheduler::instance().clock() << "s\n" ;
			cout << "PER::get_err() Average received power      "<< avgpower << " W\n"; 
			cout << "PER::get_err() Signal power                "<< pkt_pow << " W\n";
			cout << "PER::get_err() Noise power                 "<< thermal_noise << " W\n";
			cout << "PER::get_err() Interference power          "<< interf<< " W\n";
			cout << "PER::get_err() SNR (noise only)            "<< snr1<< " dB\n";
			cout << "PER::get_err() SNIR (noise + interference) "<< snr2<< " dB\n";
			cout << "PER::get_err() PER due to noise only "<< per1 << "\n"; 
			cout << "PER::get_err() PER due to noise and interference "<< per2<< "\n";
			cout << "PER::get_err() Rand "<< x<< "\n";
                        cout << "PER::get_err() Physical Mode: "<<PhyMode2str(pm)<<"\n";
                        cout << "PER::get_err() data length: "<<len<<"\n\n";
		}
	
 	if (x<per1) 
		return PKT_ERROR_NOISE; 	
	else if (x<per2) 
		return PKT_ERROR_INTERFERENCE ; 
	else 
		return PKT_OK; 

}




int PER::get_per(PhyMode pm, double tsnr, double tlen, per_set* ps_){
	
	DLList* snrlist;
	NSNode* pl1 =0;
	NSNode* pl2 =0;
	NSNode* ps1 =0;
	NSNode* ps2 =0;
	NSNode* ps3 =0;
	NSNode* ps4 =0;
	
	//cerco la lunghezza
	pl1 = (NSNode*)PHYmod[pm]->first();
	 
	if(pl1 == 0) {
	  cerr << "No PER entry found for PHYMode " << PhyMode2str(pm) << endl;
		return 1;
	}
	
	while (pl1 != PHYmod[pm]->last() && ((leng*)pl1->element())->len < tlen ){
		pl1=(NSNode*)PHYmod[pm]->after(pl1);
		}
	//pl1 o e' il primo maggiore o uguale di tlen o e' l'ultimo
	
	//prendo quello precedente (potrebbe essere nullo)
	if (((leng*)pl1->element())->len > tlen)
		pl2 = (NSNode*)PHYmod[pm]->before(pl1);
	
	
	snrlist = ((leng*)pl1->element())->snr_list;
	//cerco snr nella prima lunghezza
	ps1 = (NSNode*)snrlist->first();
	
	if(ps1 == 0) {
		cerr<<"Tabella non inizializzata 2";
		return 1;
	}
	while ( ps1 != snrlist->last()  && ((Snr*)ps1->element())->snr < tsnr  ) {
		ps1=(NSNode*)PHYmod[pm]->after(ps1);
	}
	//ps1 o e' maggiore di tsnr o e' l'ultimo
	
	//prendo quello precedente
	if (((Snr*)ps1->element())->snr > tsnr)
		ps2 = (NSNode*)snrlist->before(ps1);
	
	
	//se le lunghezze erano due diverse cerco snr anche nella seconda
	if ((pl1 != pl2) & (pl2 != 0)){
	//cerr << "dicversi";
		snrlist = ((leng*)pl2->element())->snr_list;
	//cerco
		ps3 = (NSNode*)snrlist->first();
	
		if(ps3 == 0) {
		cerr<<"Tabella non inizializzata 3";
		return 1;
		}
	
		while (not (((Snr*)ps3->element())->snr >= tsnr or ps3 == snrlist->last() ) ){
			ps3=(NSNode*)PHYmod[pm]->after(ps3);
		}
	//prendo anche quello precedente
		if (((Snr*)ps3->element())->snr > tsnr)
		ps4 = (NSNode*)snrlist->before(ps3);
	
	}
	
	
	//settaggio delle variabili di uscita in base ai risultati della ricerca. 
	//I valori /1 indicano che no e
	if (ps1){
	//cerr << ps_;
	((Snr*)ps1->element())->snr;
	
	ps_->snr[0] = ((Snr*)ps1->element())->snr;
	ps_->per[0] = ((Snr*)ps1->element())->per;
	ps_->len[0] = ((leng*)pl1->element())->len;
	}else{
	ps_->snr[0] = -1;
	ps_->per[0] = -1;
	ps_->len[0] = -1;
	}
	
	
	if (ps2){
	ps_->snr[1] = ((Snr*)ps2->element())->snr;
	ps_->per[1] = ((Snr*)ps2->element())->per;
	ps_->len[1] = ((leng*)pl1->element())->len;
	}else{
	ps_->snr[1] = -1;
	ps_->per[1] = -1;
	ps_->len[1] = -1;
	}
	
	if (ps3){
	ps_->snr[2] = ((Snr*)ps3->element())->snr;
	ps_->per[2] = ((Snr*)ps3->element())->per;
	ps_->len[2] = ((leng*)pl2->element())->len;
	}else{
	ps_->snr[2] = -1;
	ps_->per[2] = -1;
	ps_->len[2] = -1;
	}
	
	if (ps4){
	ps_->snr[3] = ((Snr*)ps4->element())->snr;
	ps_->per[3] = ((Snr*)ps4->element())->per;
	ps_->len[3] = ((leng*)pl2->element())->len;
	}else{
	ps_->snr[3] = -1;
	ps_->per[3] = -1;
	ps_->len[3] = -1;
	}

	if (debug_ > 2) {
		int i;
		//printf("Pointers: %x %x %x %x\n",ps1,ps2,ps3,ps4);
		fprintf(stderr,"4 PER table entries: \n");
		for(i=0; i<4; i++)
			fprintf(stderr,"\t %d %f %f  \n", ps_->len[i], ps_->snr[i], ps_->per[i] );

	}
        
        
        return 1;

}


void PER::set_per(PhyMode phy, int l , double snr, double per)
{

	NSNode* nl  = (NSNode*)(PHYmod[phy]->first());

	if (nl==0){
		leng* le = new leng(l);
		PHYmod[phy]->insertFirst(le);
		le->snr_list->insertFirst(new Snr(snr,per));


	}else{
		while(nl !=0 && l>((leng*)nl->element())->len)
			nl  = (NSNode*)(PHYmod[phy]->after(nl));

		if (nl==0){
			leng* le =  new leng(l);
			PHYmod[phy]->insertLast(le);
			le->snr_list->insertFirst(new Snr(snr,per));

		}else
			{

				if (((leng*)nl->element())->len==l){
					DLList* sl = ((leng*)nl->element())->snr_list;
					NSNode* ns =(NSNode*)sl->first();
					while( ns !=0 && ((Snr*)ns->element())->snr<snr )
						ns  = (NSNode*)(sl->after(ns));
					if (ns==0){
						sl->insertLast(new Snr(snr,per));
					}
					else{
						sl->insertBefore(ns,new Snr(snr,per));
					}

				}else{

					leng* le =  new leng(l);
					PHYmod[phy]->insertBefore(nl,le);
					le->snr_list->insertFirst(new Snr(snr,per));

				}
			}
	}
}




double PER::get_per(PhyMode pm, double tsnr, double tlen){

	struct per_set ps_;

	get_per( pm,  tsnr, tlen, &ps_);


//	int len1 = -1;
//	int len2 = -1;
	double per1 = -1;
	double per2 = -1;
	double per = -1;


	//not found entries are set equal  to the closest one

	//cerr <<  ps_.len[0] << ps_.len[1] << ps_.len[2] << ps_.len[3];

	if(ps_.len[0]==-1){
		ps_.len[0]=ps_.len[1];
		ps_.snr[0]=ps_.snr[1];
		ps_.per[0]=ps_.per[1];
	}

	if(ps_.len[1]==-1){
		ps_.len[1]=ps_.len[0];
		ps_.snr[1]=ps_.snr[0];
		ps_.per[1]=ps_.per[0];
	}

	if(ps_.len[2]==-1){
		ps_.len[2]=ps_.len[3];
		ps_.snr[2]=ps_.snr[3];
		ps_.per[2]=ps_.per[3];
	}

	if(ps_.len[3]==-1){
		ps_.len[3]=ps_.len[2];
		ps_.snr[3]=ps_.snr[2];
		ps_.per[3]=ps_.per[2];
	}

	if (debug_ > 2) {
		int i;
		fprintf(stderr,"4 PER table entries: \n");
		for(i=0; i<4; i++)
			fprintf(stderr,"\t %d %f %f  \n", ps_.len[i], ps_.snr[i], ps_.per[i] );

	}


	// interpolation: 1) linear interpolation at the varying of the SNR for each length
	//		2) linear interpolation among lengths 

	if (ps_.snr[1] != ps_.snr[0])
		per1 = ps_.per[0] +  (ps_.per[1]- ps_.per[0])/(ps_.snr[1]- ps_.snr[0])*(tsnr- ps_.snr[0]);
	else 
		per1 = ps_.per[1];

	if (ps_.snr[3] != ps_.snr[2])
		per2 = ps_.per[2] +  (ps_.per[3]- ps_.per[2])/(ps_.snr[3]- ps_.snr[2])*(tsnr- ps_.snr[2]);
	else
		per2 = ps_.per[2];

	if (per1!=-1 && per2!=-1){
		if (ps_.len[3] != ps_.len[1])
			per = per1 +  (per2- per1)/(ps_.len[3]- ps_.len[1])*(tlen-ps_.len[1]);
		else
			per = per1;
	}
	else 
		if (per1!=-1){
			per = per1;
		}else 
			if (per2!=-1){
				per = per2;
			}else {cerr << "No PER available";}
	return per;
}



//compute the mean average power given by the time power profile.

double PER::avg(PowerProfile* pp,double duration){
    
    //int debug_old=debug_;
    //debug_=2;

  if (debug_>1)  printf("PER::avg()  starting calculations\n");	
	

	double avg_pow = 0 ;
	for (int i=0; i< pp->getNumElements()-1; i++){
		double a =  pp->getElement(i).power;
		//double b = pp->getElement(i+1).power;
		//cerr << "pot ist" << a << " "<< b << "---\n"; 		
		double dt = (pp->getElement(i+1).time -pp->getElement(i).time);
		avg_pow = avg_pow + a * dt;

		if (debug_>1)  printf("PER::avg()  power=%e \t dt=%e total %d\n",a,dt,i);
	}

	double a = pp->getElement(pp->getNumElements()-1).power;
	double dt = (duration -pp->getElement(pp->getNumElements()-1).time);
	
	//	avg_pow = avg_pow + pp->getElement(pp->getNumElements()-1).power * (duration -pp->getElement(pp->getNumElements()-1).time);
	avg_pow = avg_pow + a * dt;

	if (debug_>1)  printf("PER::avg()  power=%e \t dt=%e duration=%e\n",a,dt,duration);

//cerr << "average0" << pp->getElement(1).time - pp->getElement(0).time;

	avg_pow = avg_pow / duration;

	if(debug_>1) printf("PER::avg() finished calculations: at %f average power=%e\n", Scheduler::instance().clock(),avg_pow);
        
       // debug_=debug_old;

	return avg_pow;
}









int PER::get_snr(PhyMode pm, double tper, double tlen, per_set* ps_){
	
	DLList* snrlist;
	NSNode* pl1 =0;
	NSNode* pl2 =0;
	NSNode* ps1 =0;
	NSNode* ps2 =0;
	NSNode* ps3 =0;
	NSNode* ps4 =0;
	
	//cerco la lunghezza
	pl1 = (NSNode*)PHYmod[pm]->first();
	
	if(pl1 == 0) {
		cerr<<"Tabella non inizializzata 1";
		return 1;
	}
	
	while (pl1 != PHYmod[pm]->last() && ((leng*)pl1->element())->len < tlen ){
		pl1=(NSNode*)PHYmod[pm]->after(pl1);
		}
	//pl1 o e' il primo maggiore o uguale di tlen o e' l'ultimo
	
	//prendo quello precedente (potrebbe essere nullo)
	if (((leng*)pl1->element())->len > tlen)
		pl2 = (NSNode*)PHYmod[pm]->before(pl1);
	
	
	snrlist = ((leng*)pl1->element())->snr_list;
	//cerco snr nella prima lunghezza
	ps1 = (NSNode*)snrlist->first();
	
	if(ps1 == 0) {
		cerr<<"Tabella non inizializzata 2";
		return 1;
	}
	while ( ps1 != snrlist->last()  && ((Snr*)ps1->element())->per > tper  ) {
		ps1=(NSNode*)PHYmod[pm]->after(ps1);
	}
	//ps1 o e' minore di tper o e' l'ultimo
	
	//prendo quello precedente
	if (((Snr*)ps1->element())->per < tper)
		ps2 = (NSNode*)snrlist->before(ps1);
	
	
	//se le lunghezze erano due diverse cerco snr anche nella seconda
	if ((pl1 != pl2) & (pl2 != 0)){
	//cerr << "dicversi";
		snrlist = ((leng*)pl2->element())->snr_list;
	//cerco
		ps3 = (NSNode*)snrlist->first();
	
		if(ps3 == 0) {
		cerr<<"Tabella non inizializzata 3";
		return 1;
		}
	
		while (not (((Snr*)ps3->element())->per <= tper or ps3 == snrlist->last() ) ){
			ps3=(NSNode*)PHYmod[pm]->after(ps3);
		}
	//prendo anche quello precedente
		if (((Snr*)ps3->element())->per < tper)
		ps4 = (NSNode*)snrlist->before(ps3);
	
	}
	
	
	//settaggio delle variabili di uscita in base ai risultati della ricerca. 
	//I valori /1 indicano che no e
	if (ps1){
	//cerr << ps_;
	((Snr*)ps1->element())->snr;
	
	ps_->snr[0] = ((Snr*)ps1->element())->snr;
	ps_->per[0] = ((Snr*)ps1->element())->per;
	ps_->len[0] = ((leng*)pl1->element())->len;
	}else{
	ps_->snr[0] = -123456789;
	ps_->per[0] = -1;
	ps_->len[0] = -1;
	}
	
	
	if (ps2){
	ps_->snr[1] = ((Snr*)ps2->element())->snr;
	ps_->per[1] = ((Snr*)ps2->element())->per;
	ps_->len[1] = ((leng*)pl1->element())->len;
	}else{
	ps_->snr[1] = -123456789;
	ps_->per[1] = -1;
	ps_->len[1] = -1;
	}
	
	if (ps3){
	ps_->snr[2] = ((Snr*)ps3->element())->snr;
	ps_->per[2] = ((Snr*)ps3->element())->per;
	ps_->len[2] = ((leng*)pl2->element())->len;
	}else{
	ps_->snr[2] = -123456789;
	ps_->per[2] = -1;
	ps_->len[2] = -1;
	}
	
	if (ps4){
	ps_->snr[3] = ((Snr*)ps4->element())->snr;
	ps_->per[3] = ((Snr*)ps4->element())->per;
	ps_->len[3] = ((leng*)pl2->element())->len;
	}else{
	ps_->snr[3] = -123456789;
	ps_->per[3] = -1;
	ps_->len[3] = -1;
	}
        
        
        return 1;

}




double PER::get_snr(PhyMode pm, double tper, double tlen){

	struct per_set ps_;

	get_snr( pm,  tper, tlen, &ps_);


//	int len1 = -1;
//	int len2 = -1;
	double snr1 = -123456789;
	double snr2 = -123456789;
	double snr = -123456789;


	//not found entries are set equal  to the clesest one

	//cerr <<  ps_.len[0] << ps_.len[1] << ps_.len[2] << ps_.len[3];

	if(ps_.len[0]==-1){
		ps_.len[0]=ps_.len[1];
		ps_.snr[0]=ps_.snr[1];
		ps_.per[0]=ps_.per[1];
	}

	if(ps_.len[1]==-1){
		ps_.len[1]=ps_.len[0];
		ps_.snr[1]=ps_.snr[0];
		ps_.per[1]=ps_.per[0];
	}

	if(ps_.len[2]==-1){
		ps_.len[2]=ps_.len[3];
		ps_.snr[2]=ps_.snr[3];
		ps_.per[2]=ps_.per[3];
	}

	if(ps_.len[3]==-1){
		ps_.len[3]=ps_.len[2];
		ps_.snr[3]=ps_.snr[2];
		ps_.per[3]=ps_.per[2];
	}


	// intepolation: 1) linear interpolation at the varying of the PER for each length
	//		2) linear interpolation among lenghts 
	
	if (ps_.snr[0] == ps_.snr[1])
		snr1 = ps_.snr[0];
	else
		snr1 = ps_.snr[0] +  (ps_.snr[1]- ps_.snr[0])/(ps_.per[1]- ps_.per[0])*(tper- ps_.per[0]);

	if (ps_.snr[2] == ps_.snr[3])
		snr2 = ps_.snr[2];
	else
		snr2 = ps_.snr[2] +  (ps_.snr[3]- ps_.snr[2])/(ps_.per[3]- ps_.per[2])*(tper- ps_.per[2]);
	
	if (snr1!=-123456789 && snr2!=-123456789){
		if (ps_.len[3] != ps_.len[1])
			snr = snr1 +  (snr2- snr1)/(ps_.len[3]- ps_.len[1])*(tlen-ps_.len[1]);
		else 
			snr = snr1;
	}
	else 
		if (snr1!=-123456789){
			snr = snr1;
		}else 
			if (snr2!=-123456789){
				snr = snr2;
			}else {cerr << "No SNR available " << snr << " " << pm << " " << tper <<" " << tlen << "/n";}
	return snr;
}


void PER::print_data(){

for (int i = 0 ; i<= Mode54Mb; i++) {
for (int l = 128 ; l<= 1024; l=l*2) {
for (double per = 0 ; per<1; per=per+0.1) {

printf("%d %d %f %f \n",i,l,per,get_snr((PhyMode)i, per , l));

}
}
}


}

/*mode 1Mb,2Mb: DSSS  const size 2,4
  mode 5.5Mb,11Mb:DSSS  const size 4,4 
  mode 6Mb, 9Mb: OFDM   const size 2,2
  mode 12Mb,18Mb: OFDM+QPSK const size 4,4
  mode 24Mb,36Mb: OFDM+QAM  const size 16,16
  mode 48Mb,54Mb: OFDM+QAM  const size 64,64*/

double PER::get_per_ns3(PhyMode pm, double tsnr, double len)
{  
    
    double per=0.0;
    
    switch(pm)
    {
        case Mode1Mb:
          
            per=1-GetDsssDbpskSuccessRate(tsnr,(uint32_t)8*len);
        
        break;
        case Mode2Mb:
            
            per=1-GetDsssDqpskSuccessRate (tsnr,(uint32_t)(8*len));
            break;
	case Mode5_5Mb:
            per=1-GetDsssDqpskCck5_5SuccessRate(tsnr,(uint32_t)(8*len));
            break;
	case Mode11Mb:
            
            per=1-GetDsssDqpskCck11SuccessRate(tsnr,(uint32_t)(8*len));
            break;
            
        case Mode6Mb:
            
            per=1-get_FEC_BPSK_BER(tsnr,(uint32_t)(8*len),1);
            break;
	case Mode9Mb:
            
            per=1-get_FEC_BPSK_BER(tsnr,(uint32_t)(8*len),3);
            break;
	case Mode12Mb:
            
            
            per=1-get_FEC_QPSK_BER(tsnr,(uint32_t)(8*len),1);;
            break;
	case Mode18Mb:
            
            per=1-get_FEC_QPSK_BER(tsnr,(uint32_t)(8*len),3);
            break;
	case Mode24Mb:
            
            per=1-get_FEC_16QAM_BER(tsnr,(uint32_t)(8*len),1);
            break;
            
	case Mode36Mb:
            
            per=1-get_FEC_16QAM_BER(tsnr,(uint32_t)(8*len),3);
            break;
            
	case Mode48Mb:
            
            per=1-get_FEC_64QAM_BER(tsnr,(uint32_t)(8*len),2);;
            break;
            
	case Mode54Mb:
            
            per=1-get_FEC_64QAM_BER(tsnr,(uint32_t)(8*len),3);
            break;
            
        default:
            break;
            
            
    }
    
    return per;
}




double 
PER::GetDsssDbpskSuccessRate (double sinr, uint32_t nbits)
{
  double EbN0 = sinr * 22000000.0 / 1000000.0; // 1 bit per symbol with 1 MSPS
  double ber = 0.5 * exp (-EbN0);
  return pow ((1.0 - ber), nbits);
}

double
PER::GetDsssDqpskSuccessRate (double sinr,uint32_t nbits)
{
  double EbN0 = sinr * 22000000.0 / 1000000.0 / 2.0; // 2 bits per symbol, 1 MSPS
  double ber = DqpskFunction (EbN0);
  return pow ((1.0 - ber), nbits);
}

double
PER::GetDsssDqpskCck5_5SuccessRate (double sinr,uint32_t nbits)
{
#ifdef ENABLE_GSL
  // symbol error probability
  double EbN0 = sinr * 22000000.0 / 1375000.0 / 4.0;
  double sep = SymbolErrorProb16Cck (4.0 * EbN0 / 2.0);
  return pow (1.0 - sep,nbits / 4.0);
#else
  //NS_LOG_WARN ("Running a 802.11b CCK Matlab model less accurate than GSL model");
  // The matlab model
  double ber;
  if (sinr > WLAN_SIR_PERFECT)
    {
      ber = 0.0;
    }
  else if (sinr < WLAN_SIR_IMPOSSIBLE)
    {
      ber = 0.5;
    }
  else
    {
      // fitprops.coeff from matlab berfit
      double a1 =  5.3681634344056195e-001;
      double a2 =  3.3092430025608586e-003;
      double a3 =  4.1654372361004000e-001;
      double a4 =  1.0288981434358866e+000;
      ber = a1 * exp (-(pow ((sinr - a2) / a3,a4)));
    }
  return pow ((1.0 - ber), nbits);
#endif
}

double
PER::GetDsssDqpskCck11SuccessRate (double sinr,uint32_t nbits)
{
#ifdef ENABLE_GSL
  // symbol error probability
  double EbN0 = sinr * 22000000.0 / 1375000.0 / 8.0;
  double sep = SymbolErrorProb256Cck (8.0 * EbN0 / 2.0);
  return pow (1.0 - sep,nbits / 8.0);
#else
  //NS_LOG_WARN ("Running a 802.11b CCK Matlab model less accurate than GSL model");
  // The matlab model
  double ber;
  if (sinr > WLAN_SIR_PERFECT)
    {
      ber = 0.0;
    }
  else if (sinr < WLAN_SIR_IMPOSSIBLE)
    {
      ber = 0.5;
    }
  else
    {
      // fitprops.coeff from matlab berfit
      double a1 =  7.9056742265333456e-003;
      double a2 = -1.8397449399176360e-001;
      double a3 =  1.0740689468707241e+000;
      double a4 =  1.0523316904502553e+000;
      double a5 =  3.0552298746496687e-001;
      double a6 =  2.2032715128698435e+000;
      ber =  (a1 * sinr * sinr + a2 * sinr + a3) / (sinr * sinr * sinr + a4 * sinr * sinr + a5 * sinr + a6);
    }
  return pow ((1.0 - ber), nbits);
#endif
}


double
PER::DqpskFunction (double x)
{
  return ((sqrt (2.0) + 1.0) / sqrt (8.0 * 3.1415926 * sqrt (2.0)))
         * (1.0 / sqrt (x))
         * exp ( -(2.0 - sqrt (2.0)) * x);
}

double
PER::get_BPSK_BER (double snr) const
{
  double z = sqrt (snr);
  double ber = 0.5 * erfc (z);
  
  return ber;
}

double
PER::get_QPSK_BER (double snr) const
{
  double z = sqrt (snr / 2.0);
  double ber = 0.5 * erfc (z);
  
  return ber;
}

double
PER::get_16QAM_BER (double snr) const
{
  double z = sqrt (snr / (5.0 * 2.0));
  double ber = 0.75 * 0.5 * erfc (z);
  
  return ber;
}

double
PER::get_64QAM_BER (double snr) const
{
  double z = sqrt (snr / (21.0 * 2.0));
  double ber = 7.0 / 12.0 * 0.5 * erfc (z);
  
 
  
  return ber;
}

double
PER::get_FEC_BPSK_BER (double snr, double nbits,
                                   uint32_t bValue) const
{
  double ber = get_BPSK_BER (snr);
  if (ber == 0.0)
    {
      return 1.0;
    }
  double pe = calculate_PE (ber, bValue);
  pe = std::min (pe, 1.0);
  double pms = pow (1 - pe, nbits);
  return pms;
}

double
PER::get_FEC_QPSK_BER (double snr, double nbits,
                                   uint32_t bValue) const
{
  double ber = get_QPSK_BER (snr);
  if (ber == 0.0)
    {
      return 1.0;
    }
  double pe = calculate_PE (ber, bValue);
  pe = std::min (pe, 1.0);
  double pms = pow (1 - pe, nbits);
  return pms;
}

double
PER::calculate_PE (double p, uint32_t bValue) const
{
  double D = sqrt (4.0 * p * (1.0 - p));
  double pe = 1.0;
  if (bValue == 1)
    {
      // code rate 1/2, use table 3.1.1
      pe = 0.5 * ( 36.0 * pow (D, 10.0)
                   + 211.0 * pow (D, 12.0)
                   + 1404.0 * pow (D, 14.0)
                   + 11633.0 * pow (D, 16.0)
                   + 77433.0 * pow (D, 18.0)
                   + 502690.0 * pow (D, 20.0)
                   + 3322763.0 * pow (D, 22.0)
                   + 21292910.0 * pow (D, 24.0)
                   + 134365911.0 * pow (D, 26.0)
                   );
    }
  else if (bValue == 2)
    {
      // code rate 2/3, use table 3.1.2
      pe = 1.0 / (2.0 * bValue) *
        ( 3.0 * pow (D, 6.0)
          + 70.0 * pow (D, 7.0)
          + 285.0 * pow (D, 8.0)
          + 1276.0 * pow (D, 9.0)
          + 6160.0 * pow (D, 10.0)
          + 27128.0 * pow (D, 11.0)
          + 117019.0 * pow (D, 12.0)
          + 498860.0 * pow (D, 13.0)
          + 2103891.0 * pow (D, 14.0)
          + 8784123.0 * pow (D, 15.0)
        );
    }
  else if (bValue == 3)
    {
      // code rate 3/4, use table 3.1.2
      pe = 1.0 / (2.0 * bValue) *
        ( 42.0 * pow (D, 5.0)
          + 201.0 * pow (D, 6.0)
          + 1492.0 * pow (D, 7.0)
          + 10469.0 * pow (D, 8.0)
          + 62935.0 * pow (D, 9.0)
          + 379644.0 * pow (D, 10.0)
          + 2253373.0 * pow (D, 11.0)
          + 13073811.0 * pow (D, 12.0)
          + 75152755.0 * pow (D, 13.0)
          + 428005675.0 * pow (D, 14.0)
        );
    }
  else
    {
      assert(false);
    }
  return pe;
}

double
PER::get_FEC_16QAM_BER (double snr, uint32_t nbits,
                                    uint32_t bValue) const
{
  double ber = get_16QAM_BER (snr);
  if (ber == 0.0)
    {
      return 1.0;
    }
  double pe = calculate_PE (ber, bValue);
  pe = std::min (pe, 1.0);
  double pms = pow (1 - pe, nbits);
  return pms;
}


double
PER::get_FEC_64QAM_BER (double snr, uint32_t nbits,
                                    uint32_t bValue) const
{
  double ber = get_64QAM_BER (snr);
  if (ber == 0.0)
    {
      return 1.0;
    }
  double pe = calculate_PE (ber, bValue);
  pe = std::min (pe, 1.0);
  double pms = pow (1 - pe, nbits);
  return pms;
}






void 
PER::calculate_snr(PhyMode pm,double signal,double *interf,double *snr_ptr,double *snir_ptr,double *thermal_noise_ptr)
{
    // thermal noise at 290K in J/s = W
  static const double BOLTZMANN = 1.3803e-23;
  // Nt is the power of thermal noise in W
  double Nt = BOLTZMANN * 290.0 * get_mode_bandwidth (pm);
  // receiver noise Floor (W) which accounts for thermal noise and non-idealities of the receiver
  double noiseFloor = 10.0 * Nt;
  
  noiseFloor=1.31e-11;
  
  
  /*prevent float point calculation error*/
  
  if(fabs(*interf)<0.1*noiseFloor)
      *interf=0.0;
  
  
  double noise = noiseFloor + *interf;
  double snr1 = signal / noiseFloor;
  double snr2 = signal / noise;
  
  if(0)
  {
      fprintf(stderr,"find an interf %e, signal %e, snr %f, snir %f\n",*interf,signal,10.0*log10(snr1),10.0*log10(snr2));
  }
  
  if(snr1<0 || snr2<0)
  {
      fprintf(stderr,"error!,signal %e, noise floor %e,interf %e\n",signal,noiseFloor,*interf);
    
  }
  if(snr_ptr)
      *snr_ptr=snr1;
  if(snir_ptr)
      *snir_ptr=snr2;
  if(thermal_noise_ptr)
      *thermal_noise_ptr=noiseFloor;
}

int 
PER::get_mode_bandwidth(PhyMode pm)
{
    int bandwidth=20000000;
    
    switch(pm)
    {
        case Mode1Mb:   
        case Mode2Mb:    
        case Mode5_5Mb:    
        case Mode11Mb:
            bandwidth=22000000;
            break;
            
        case Mode6Mb:    
        case Mode9Mb:    
        case Mode12Mb:
        case Mode18Mb:
        case Mode24Mb:
        case Mode36Mb:
        case Mode48Mb:
        case Mode54Mb:
            bandwidth=20000000;
            break;
        default:
            break;
            
    }
    
    
    
    
    return bandwidth;
}


