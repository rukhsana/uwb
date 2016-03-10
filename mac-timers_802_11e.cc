//* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1997 Regents of the University of California.
 * Copyright (c) 2009 University of Victoria.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#undef NDEBUG
#include <assert.h>
#include <delay.h>
#include <connector.h>
#include <packet.h>
#include <random.h>
#include <iostream.h>
#define BDEBUG 0 

//#include <debug.h>
#include <arp.h>
#include <ll.h>
#include <mac.h>
#include <mac/802_11e/mac-timers_802_11e.h>
#include <mac/802_11e/mac-802_11e.h> 

// set to 1 for EDCA, 0 for older EDCF
#define EDCA 1
#define INF   (1e12)
#define ROUND (1e-12)
#define MU(x)    x * (1e+6)


/* ======================================================================
   Timers
   ====================================================================== */

// Written by Ruby

double MacTimer_802_11e::doMod(double aNumber, double aDivisor)
{
      for(;;)
      {
	if(aNumber<=aDivisor)
	{
	    if(rounding(aNumber, aDivisor)) return 0;
	    else return aNumber;
	}
        aNumber = aNumber - aDivisor;
      }
}

//DRP: 101-150 (25601-38400), 176-225 (45056-57600)
//Beacon period: 1-32 (0-8192)

//In case of pkt transmission, change start time
//While decrementing backoff counter, change remaining time  

// Written by Ruby

bool MacTimer_802_11e::checkNearbyDRPorBeacon(double *start_time, double *rem_time, int pkt_tx_duration)
{
	double beacon_period = 0.008192;
	Scheduler &s = Scheduler::instance();
	double cur_time = s.clock();
	cur_time = doMod(cur_time, 0.065536);
        double start_time_ = doMod(*start_time, 0.065536);

	//double tmp_dur2 = doMod(tmp_dur1, 0.065536);

	if(pkt_tx_duration == 0)
	{
		double tmp_dur1 = start_time_ + *rem_time;
		//start_time can be on beacon period
		if(start_time_ <= 0.008192)
		{
			*start_time = *start_time + 0.008193 - start_time_;
		}
		//start_time can be bigger than beacon period
		else if(start_time_ > 0.008192 && tmp_dur1 > 0.065536)
		{
			*rem_time = *rem_time + 0.008192;
		}
	        

		double drp_start = 0.000256*100+0.000001;
		double drp_end = 0.000256*150;


		//start_time can be before DRP period
		if(start_time_ < drp_start && tmp_dur1 >= drp_start)
		{
			*rem_time = *rem_time + drp_start - drp_end + 0.000001;
		}

		//start_time can be on the DRP period
		else if(start_time_ >= drp_start && start_time_ <= drp_end)
		{
			*start_time = *start_time + (start_time_ - drp_end)+0.000001;
		} 	
	}
	else 
	{
		double tmp_dur1 = start_time_ + getFrameTxDur();
		//start_time can be on beacon period
		if(start_time_ <= 0.008192)
		{
			*start_time = *start_time + 0.008193 - start_time_;
		}
		//start_time can be bigger than beacon period
		else if(start_time_ > 0.008192 && tmp_dur1 > 0.065536)
		{
			*start_time = 0.008193;
		}
	        

		double drp_start = 0.000256*100+0.000001;
		double drp_end = 0.000256*150;


		//start_time can be before DRP period
		if(start_time_ < drp_start && tmp_dur1 >= drp_start)
		{
			*start_time = *start_time + (start_time_ - drp_end)+0.000001;
		}

		//start_time can be on the DRP period
		else if(start_time_ >= drp_start && start_time_ <= drp_end)
		{
			*start_time = *start_time + (start_time_ - drp_end)+0.000001;
		} 	
	}		
	//txtime(phymib_.getACKlen(), basicRate_) + DSSS_EDCA_MaxPropagationDelay + sifs_

}

// Written by Ruby

inline bool MacTimer_802_11e::rounding(double x, double y)
{
	/*
	if(BDEBUG>2) printf("now %4.8f Mac: %d in BackoffTimer::rounding\n",
			   Scheduler::instance().clock(), mac->index_);
	*/
	/*
	 * check whether x is within y +/- 1e-12 
	 */
	if(x > y && x < y + ROUND )
		return 1;
	else if(x < y && x > y - ROUND )
		return 1;
	else if (x == y)
		return 1;
	else 
		return 0;
}


void
MacTimer_802_11e::start(double time)
{
	Scheduler &s = Scheduler::instance();
	assert(busy_ == 0);

	busy_ = 1;
	paused_ = 0;
	stime = s.clock();
	rtime = time;
	assert(rtime >= 0.0);
	s.schedule(this, &intr, rtime);
}



void
MacTimer_802_11e::stop(void)
{
	Scheduler &s = Scheduler::instance();

	assert(busy_);

	if(paused_ == 0)
		s.cancel(&intr);

	busy_ = 0;
	paused_ = 0;
	stime = 0.0;
	rtime = 0.0;
}

double MacTimer_802_11e::getFrameTxDur(void)
{
	 double datalen = 520;
	 
	 datalen = (datalen/8) - 28 - 32;
	 datalen = datalen*8;

	 double plcp_preamble = 9.375;
	 double Tsym = 0.3125;
	 int no_of_symbol_plcp_header = 12;
	 double plcp_header_dur = ((double)no_of_symbol_plcp_header)*Tsym;
	 double t = plcp_preamble + plcp_header_dur + 6*ceil((datalen+38)/100)*Tsym;

	 t = t + plcp_preamble + plcp_header_dur + 0.00001 + 0.000012+0.00001 + 2*0.000009;
	 return t;

}
/* ======================================================================
   Defer Timer
   ====================================================================== */
void
DeferTimer_802_11e::start(int pri, double time)
{
	Scheduler &s = Scheduler::instance();
	bool another = 0;
	assert(defer_[pri] == 0);
	for(int i = 0; i < MAX_PRI; i++ ){	
		if(defer_[i] && busy_) {
			another = 1;
		}
	}
	
	if(another){
		pause();
	}
	busy_ = 1;

	defer_[pri] = 1;
	stime_[pri] = s.clock();
	rtime_[pri] = time;
	//Added by Ruby
	//checkNearbyDRPorBeacon(&stime_[pri], &rtime_[pri], 0); 
	double delay = INF;
	int prio = MAX_PRI + 1;
	for(int pri = 0; pri < MAX_PRI; pri++){
		if(defer_[pri]){
			stime_[pri] = s.clock();
			double delay_ = rtime_[pri]; 
			if((delay_ < delay)) {
				delay = delay_;
				prio = pri;
			}
		}
	}
	if(prio < MAX_PRI + 1) {
		assert(rtime_[prio] >=0);
		s.schedule(this, &intr, rtime_[prio]);
		paused_ = 0;
	} else {
		exit(0);

	}
	if(mac->mhBeacon_.get_beacon_period_flag() == 1 || mac->mhDRP_.get_drp_period_flag() == 1)
	{
		stop(2);
	}
        /* End */
}
/*
Written by Ruby
flag:1, when stop is needed due to busy channel
flag:2, when stop is needed due to drp or beacon period
*/
void 
DeferTimer_802_11e::stop(int flag)
{
	busy_ = 0;
	for(int pri = 0; pri < MAX_PRI;pri++){
		if(flag == 1)
		{
			if(defer_[pri]) mac->defer_stop(pri);
			rtime_[pri] = 0;
		}
		else if(flag == 2)
		{
			if(defer_[pri])
				set_int_flag(pri, 1);			
		}
		defer_[pri] = 0;
		stime_[pri] = 0;
		
	}
	Scheduler &s = Scheduler::instance();
	s.cancel(&intr);
}
void
DeferTimer_802_11e::pause()
{
	Scheduler &s = Scheduler::instance();
	for(int pri = 0; pri < MAX_PRI; pri++ ){	
		if(defer_[pri] && busy_){
			double st = s.clock();            //now
			double rt = stime_[pri];   // Start-Time of defer
			double sr = st - rt;
			assert(busy_ && !paused_);
			if(rtime_[pri] - sr >= 0.0) {
				rtime_[pri] -= sr;
				assert(rtime_[pri] >= 0.0);	
			} else{
				if(rtime_[pri] + ROUND - sr >= 0.0){ 
					rtime_[pri] = 0;
				} else {
					cout<<"ERROR in DeferTimer::pause(), rtime_["<<pri<<"]  is "<<rtime_[pri]<<", sr:"<<sr<<"  \n";
					exit(0);
				}
			}
		}
	}
	s.cancel(&intr);
	paused_ = 1;
}


void    
DeferTimer_802_11e::handle(Event *)
{       
	Scheduler &s = Scheduler::instance();
	double delay = INF;
	int prio = MAX_PRI + 1;

	for( int pri = 0; pri < MAX_PRI; pri++) {
		
		if(rtime_[pri] >= 0 && defer_[pri]){
			
			double delay_ = rtime_[pri];
			if((delay_ < delay) && defer_[pri]) {
				delay = delay_;
				prio = pri;
			}
		}
	}

	if(prio < MAX_PRI + 1){
		busy_ = 0;
		paused_ = 0;
		defer_[prio] = 0;
		stime_[prio] = 0.0;
		rtime_[prio] = 0.0;
		//printf("\ndeferHandle:%lf", Scheduler::instance().clock());
		mac->deferHandler(prio);
	} else {
		cout<<"handling ERROR in DeferTimer::handler \n"; 
		exit(0);
	}
	
        //check if there is another DeferTimer active
	for( int pri = 0; pri < MAX_PRI; pri++) {
		if(rtime_[pri] >= 0 && defer_[pri]){
		  rtime_[pri] = 0.0;
		  stime_[pri] = 0.0;
		  defer_[pri] = 0;
			printf("\ndeferStop:%lf", Scheduler::instance().clock());
		  mac->defer_stop(pri);
		}
	}	
}

int
DeferTimer_802_11e::defer(int pri) {
	return defer_[pri];
}

// Written by Ruby 
double DeferTimer_802_11e::get_rtime(int pri)
{
	return rtime_[pri];
}

// Written by Ruby
int DeferTimer_802_11e::get_int_flag(int pri)
{
	return int_flag[pri];
}

// Written by Ruby
void DeferTimer_802_11e::set_int_flag(int pri, int value)
{
	int_flag[pri] = value;
}

/* ======================================================================
   NAV Timer
   ====================================================================== */

/*
void
NavTimer_802_11e::start(double time)
{
	Scheduler &s = Scheduler::instance();
	assert(busy_ == 0);

	busy_ = 1;
	paused_ = 0;
	stime = s.clock();
	rtime = time;
	assert(rtime >= 0.0);
	//checkNearbyDRPorBeacon(&stime, &rtime, rtime, 1);
	//checkNearbyDRPorBeacon(&stime, &rtime, rtime, 0);
	s.schedule(this, &intr, rtime);
}*/

void
NavTimer_802_11e::start(double time)
{
	Scheduler &s = Scheduler::instance();
	assert(busy_ == 0);

	busy_ = 1;
	paused_ = 0;
	stime = s.clock();
	rtime = time;
	assert(rtime >= 0.0);
	s.schedule(this, &intr, rtime);
	if(mac->mhBeacon_.get_beacon_period_flag() == 1)
	{
		pause();
	}
}

void NavTimer_802_11e::pause(void)
{
	Scheduler &s = Scheduler::instance();
	double now = s.clock();
	rtime = rtime - (now -stime);	
	assert(busy_ == 1);
	paused_ = 1;
	s.cancel(&intr);

}
void NavTimer_802_11e::resume(void)
{
	Scheduler &s = Scheduler::instance();
	assert(paused_ == 1);
	paused_ = 0;
	assert(busy_ == 1);
	s.schedule(this, &intr, rtime);
}

void    
NavTimer_802_11e::handle(Event *)
{       
	busy_ = 0;
	paused_ = 0;
	stime = 0.0;
	rtime = 0.0;

	mac->navHandler();
}


/* ======================================================================
   Receive Timer
   ====================================================================== */
void    
RxTimer_802_11e::handle(Event *)
{       
	busy_ = 0;
	paused_ = 0;
	stime = 0.0;
	rtime = 0.0;
	mac->recvHandler();
}


/* ======================================================================
   Send Timer
   ====================================================================== */
int
TxTimer_802_11e::get_start_flag()
{
	return start_flag;
}
/*
Written by Ruby
flag = 3, if there is enough time available before drp or beacon for successful pkt transmission
flag = 1, if there is no enough time available for transmission before drp or beacon period
flag = 2, when transsion needs to be resumed when drp or beacon is finished
*/
void
TxTimer_802_11e::start(double time, int flag)
{
	Scheduler &s = Scheduler::instance();
	if(flag == 1)
	{
		assert(busy_ == 0);

		busy_ = 1;
		paused_ = 0;
		stime = s.clock();
		rtime = time;
		assert(rtime >= 0.0);
		start_flag = 1;
	}
	else if(flag == 2)
	{
		start_flag = 0;
		s.schedule(this, &intr, rtime);
	}
	else if(flag == 3)
	{
		assert(busy_ == 0);
		busy_ = 1;
		paused_ = 0;
		stime = s.clock();
		rtime = time;
		assert(rtime >= 0.0);
		s.schedule(this, &intr, rtime);
	}
}
void    
TxTimer_802_11e::handle(Event *)
{       
	busy_ = 0;
	paused_ = 0;
	stime = 0.0;
	rtime = 0.0;



	mac->sendHandler();
}
/*
void
TxTimer_802_11e::start(double time)
{
	Scheduler &s = Scheduler::instance();
	assert(busy_ == 0);

	busy_ = 1;
	paused_ = 0;
	stime = s.clock();
	rtime = time;
	//Added by Ruby
	//checkNearbyDRPorBeacon(&stime, &rtime, 1);
	assert(rtime >= 0.0);
	s.schedule(this, &intr, rtime);
}

*/

/* ======================================================================
   Interface Timer
   ====================================================================== */
int
IFTimer_802_11e::get_start_flag()
{
	return start_flag;
}
/*
Written by Ruby
flag = 3, if there is enough time available for transmission before drp or beacon
flag = 1, if there is no enough time available for transmission before drp or beacon period
flag = 2, when transsion needs to be resumed when drp or beacon is finished
*/
void
IFTimer_802_11e::start(double time, int flag)
{
	Scheduler &s = Scheduler::instance();
	if(flag == 1)
	{
		//printf("\nflag=1, clock:%lf, obj_name:%s", s.clock(), mac->et_obj_name);
		assert(busy_ == 0);

		busy_ = 1;
		paused_ = 0;
		stime = s.clock();
		rtime = time;
		assert(rtime >= 0.0);
		start_flag = 1;
	}
	else if(flag == 2)
	{
		//printf("\nflag=2, clock:%lf, obj_name:%s, rtime:%lf", s.clock(), mac->et_obj_name, rtime);
		start_flag = 0;
		s.schedule(this, &intr, rtime);
	}
	else if(flag == 3)
	{
		//printf("\nflag=3, clock:%lf, obj_name:%s, rtime: %lf", s.clock(), mac->et_obj_name, time);
		assert(busy_ == 0);

		busy_ = 1;
		paused_ = 0;
		stime = s.clock();
		rtime = time;
		assert(rtime >= 0.0);
		s.schedule(this, &intr, rtime);
	}
}
void
IFTimer_802_11e::handle(Event *)
{
	Scheduler &s = Scheduler::instance();
	busy_ = 0;
	paused_ = 0;
	stime = 0.0;
	rtime = 0.0;
	mac->txHandler();
	
}

/* ======================================================================
 Defer Timer for SIFS
   ====================================================================== */
void
SIFSTimer_802_11e::start(int pri, double time)
{

	Scheduler &s = Scheduler::instance();
	if(busy_ == 1){cout<<"Mac "<<mac->index_<<", ERROR in SIFSTimer!"; exit(0);}
	assert(sifs_[pri] == 0);
	
	busy_ = 1;

        sifs_[pri] = 1;
	stime_[pri] = s.clock();
	rtime_[pri] = time; 
	s.schedule(this, &intr, rtime_[pri]);
}

void
SIFSTimer_802_11e::handle(Event *)
{
	Scheduler &s = Scheduler::instance();
	busy_ = 0;
	prio = 0;
		
	for(int pri = 0; pri < MAX_PRI; pri++){
		if(sifs_[pri]) { 
			prio = pri;
			break;
		}
	}
	
	for(int i = 0; i < MAX_PRI; i++){	
		sifs_[i]  = 0;
		stime_[i] = 0.0;
		rtime_[i] = 0.0;
	}
	mac->deferHandler(prio);
	
}
/* ======================================================================
   Backoff Timer
   ====================================================================== */

/*
 * round to the next slot, 
 * this is needed if a station initiates a transmission
 * and has not been synchronized by the end of last frame on channel
 */

inline void 
BackoffTimer_802_11e::round_time(int pri)
{
	//assert(slottime);
	
	if(BDEBUG>2) printf("now %4.8f Mac: %d in BackoffTimer::round_time\n",
			   Scheduler::instance().clock(), mac->index_);
	if(BDEBUG==1) printf("1 now %4.8f Mac: %d slottime %2.2f rtime_[pri] %2.2f \n", 
	       Scheduler::instance().clock(),mac->index_,MU(slottime),MU(rtime_[pri]));
	//	double slottime = mac->phymib_.getSlotTime();
	double rmd = remainder(rtime_[pri], slottime);
	if(rmd + ROUND < 0){
		printf("ERROR: in BackoffTimer_802_11e:: round_time, remainder < 0 ?! rmd: %4.20f\n",rmd);
		exit(1);
	}
	if(rmd > ROUND){
		if(BDEBUG==1) printf("2 now %4.8f Mac: %d round_time slot-duration: %2.2f rtime: %2.2f r/s %f floor %f \n", 
		       Scheduler::instance().clock(),mac->index_, MU(slottime),MU(rtime_[pri]),rtime_[pri]/slottime,floor(rtime_[pri]/slottime));
		rtime_[pri] = ceil(rtime_[pri]/slottime)*slottime;
	} else {
		if(BDEBUG==1) printf("3 now %4.8f Mac: %d round_time slot-duration: %2.2f rtime: %2.2f r/s %f ceil %f \n", 
		       Scheduler::instance().clock(),mac->index_, MU(slottime),MU(rtime_[pri]),rtime_[pri]/slottime, ceil(rtime_[pri]/slottime));
		rtime_[pri] = floor(rtime_[pri]/slottime)*slottime;
	}
	if(BDEBUG==1) printf("4 now %4.8f Mac: %d round_time slot-duration: %2.2f us remainder: %2.2f us slots %2.2f \n", 
	       Scheduler::instance().clock(),mac->index_,MU(slottime),MU(rmd), rtime_[pri]/slottime);
}

void
BackoffTimer_802_11e::handle(Event *)
{
	if(BDEBUG>2) printf("now %4.8f Mac: %d begin of BackoffTimer::handle\n",
			   Scheduler::instance().clock(), mac->index_);
	Scheduler &s = Scheduler::instance();
	paused_ = 0;
	double delay = INF;
	int prio = MAX_PRI + 1;
	
	for(int pri = 0; pri < MAX_PRI; pri++){
		
		double delay_ = rtime_[pri] + AIFSwait_[pri];
		if((delay_ < delay) && backoff_[pri] && !rounding(delay,delay_)) {
			delay = delay_;
			prio = pri;
		}
	}
	
	if(!EDCA && !rounding(s.clock(), stime_[prio] + rtime_[prio] + AIFSwait_[prio])){
	       	printf("now %4.8f Mac: %d BackoffTimer::handle but no priority matches ?! stime_[%d] %2.8f rtime_[%d] %2.2f AIFSwait_[%d] %2.2f \n",
		       s.clock(), mac->index_, prio, stime_[prio], prio, MU(rtime_[prio]), prio, MU(AIFSwait_[prio]));
		exit(1);
	}
	if(prio < MAX_PRI + 1) {
			busy_ = 0;
		        stime_[prio] = 0;
			rtime_[prio] = 0;
			backoff_[prio] = 0;
			AIFSwait_[prio] = 0.0;
			decremented_[prio]=0;
		//mac->backoffHandler(prio); now done further down at the end of Handler!	
	} else {
		cout<<"ERROR: wrong priority in BackoffTimer::handler()";
		exit(0);
	}
	
	busy_ = 0;
	bool another=0;

	for(int pri = 0; pri < MAX_PRI; pri++){
		if(backoff_[pri]) { //check if there is another backoff process
			
			if(rounding(rtime_[pri] + AIFSwait_[pri],delay)) {
				// check if other priority is not a postbackoff, 
				// i.e., packet to tx is available
				if(mac->pktTx_[pri]) {
					mac->inc_cw(pri);
					if(EDCA) {
						if(mac->inc_retryCounter(pri)) {
							// maximum retry limit exceeded,
							// deletion of packet done in inc_retryCounter(),
							// now reset Backoff variables
							stime_[pri] = 0;
							rtime_[pri] = 0;
							backoff_[pri] = 0;
							AIFSwait_[pri] = 0.0;
						} else {
							AIFSwait_[pri] = 0;
							stime_[pri] = s.clock(); //Start-Time
							rtime_[pri] = (Random::random() % (mac->getCW(pri) + 1)) * slottime; 
							//Added by Ruby

							//checkNearbyDRPorBeacon(&stime_[pri], &rtime_[pri], 0);
							backoff_[pri] = 1;
							another=1;
						}
					} else {
				 		AIFSwait_[pri] = 0;
						stime_[pri] = s.clock(); //Start-Time
						rtime_[pri] = (Random::random() % (mac->getCW(pri) + 1)) * slottime; 
						//Added by Ruby
						//checkNearbyDRPorBeacon(&stime_[pri], &rtime_[pri], 0);
						backoff_[pri] = 1;
						another=1;
					}
				} else {
					// end of postbackoff: reset stime_, rtime_, ...
					stime_[pri] = 0;
					rtime_[pri] = 0;
					backoff_[pri] = 0;
					AIFSwait_[pri] = 0.0;
				}
			} else
				another=1;
		}
	}	
        if(another) busy_=1;

	if(busy_ && !paused_){
		if(BDEBUG>2) printf("now %4.8f Mac: %d BackoffTimer::handle calling pause and restart\n",
				    Scheduler::instance().clock(), mac->index_);
		pause(); //pause + restart because the finished backoff could have been a postbackoff!
		//printf("\nbackoffHandler, clock:%lf, prio:%d", s.clock(), prio);
		restart();
		
	}

	mac->backoffHandler(prio);	
	if(BDEBUG>2) printf("now %4.8f Mac: %d end of BackoffTimer::handle\n",
			   Scheduler::instance().clock(), mac->index_);
}

void
BackoffTimer_802_11e::start(int pri, int cw, int idle)
{
	//printf("\nslottime: %lf", slottime);
	//printf("\ncur time: %lf", Scheduler::instance().clock());
	assert(slottime);
	if(BDEBUG>2) printf("now %4.8f Mac: %d in BackoffTimer::start for pri %d busy_ %d \n",
			    Scheduler::instance().clock(), mac->index_,pri, busy_);

	Scheduler &s = Scheduler::instance();
	//printf("\nstart:%lf", s.clock());
	
	if(busy_) { //already a backoff running!
		stime_[pri] = s.clock(); //Start-Time
		rtime_[pri] = (Random::random() % (cw + 1)) * slottime;
		//Added by Ruby
		//checkNearbyDRPorBeacon(&stime_[pri], &rtime_[pri], 0);
		AIFSwait_[pri] = 0.0;
		decremented_[pri]=0;
		if(BDEBUG>2)printf("now: %4.8f Mac: %d starting backoff during others: setting AIFS_[%d] to zero\n",
		       s.clock(), mac->index_, pri);
		backoff_[pri] = 1;
		if(idle == 0){
			if(!paused_){
				pause();
			}
		}
		else {
			assert(rtime_[pri] >= 0.0);
			if(!paused_) {
				pause();
				AIFSwait_[pri] = mac->getAIFS(pri); 
				//cout<<"Mac "<<mac->index_<<": now: "<<Scheduler::instance().clock()<<"pause and restart for prio "<<pri<<"\n";
				restart();
			}
		}
		if(BDEBUG>2) printf("now %4.8f Mac: %d end of BackoffTimer::start stime_[%d] %4.8f rtime_[%d] %2.2f AIFSwait_[%d] %2.2f\n",
				    Scheduler::instance().clock(), mac->index_, pri, stime_[pri], pri, MU(rtime_[pri]), pri, MU(AIFSwait_[pri]));
	} else {
		backoff_[pri] = 1;
		busy_ = 1;
		stime_[pri] = s.clock(); //Start-Time
		rtime_[pri] = (Random::random() % (cw + 1)) * slottime;
		
		//Added by Ruby
		//checkNearbyDRPorBeacon(&stime_[pri], &rtime_[pri], 0);

		AIFSwait_[pri] = mac->getAIFS(pri);  

		if(idle == 0){
			paused_ = 1;
		}
		else {
			assert(rtime_[pri] >= 0.0);
			s.schedule(this, &intr, rtime_[pri] + AIFSwait_[pri]);
		}
		if(BDEBUG>2) printf("now %4.8f Mac: %d end of BackoffTimer::start, timer scheduled stime_[%d] %4.8f rtime_[%d] %2.8f AIFSwait_[%d] %2.2f \n",
				    Scheduler::instance().clock(), mac->index_, pri, stime_[pri], pri, MU(rtime_[pri]), pri, MU(AIFSwait_[pri]));
	}
	if(mac->mhBeacon_.get_beacon_period_flag() == 1 || mac->mhDRP_.get_drp_period_flag() == 1)
	{
		pause();
	}
}
   
inline bool BackoffTimer_802_11e::rounding(double x, double y)
{
	if(BDEBUG>2) printf("now %4.8f Mac: %d in BackoffTimer::rounding\n",
			   Scheduler::instance().clock(), mac->index_);
	/*
	 * check whether x is within y +/- 1e-12 
	 */
	if(x > y && x < y + ROUND )
		return 1;
	else if(x < y && x > y - ROUND )
		return 1;
	else if (x == y)
		return 1;
	else 
		return 0;
}

void
BackoffTimer_802_11e::pause()
{
	Scheduler &s = Scheduler::instance();
	int check_pause = 10;
	//the caculation below make validation pass for linux though it
	// looks dummy

	for (int pri = 0; pri < MAX_PRI; pri++) {
		if(backoff_[pri] && !rounding(stime_[pri],s.clock()) && !rounding(decremented_[pri],s.clock())) {
			check_pause = pri;
			double st = s.clock();            //now
			double rt = stime_[pri] + AIFSwait_[pri]; // start-Time of backoff + waiting time
			double sr = st - rt; // is < 0,  if stime is back for less than AIFSwait, thus a slot 
			                     // decrement must not appear
		
			int slots =0;
		
			if(BDEBUG>2) printf("now %4.8f Mac: %d BackoffTimer::pause beginning stime_[%d] %4.8f rtime_[%d] %2.2f AIFSwait[%d] %2.2f slottime %2.2f diff/slottime %2.2f \n",
					    s.clock(), mac->index_, pri, stime_[pri], pri, MU(rtime_[pri]), pri, MU(AIFSwait_[pri]),MU(slottime), sr/slottime);
			if(rounding(sr,0.0)){ // now - stime is equal to AIFSWAIT
				// include EDCA rule: slot decrement due to full AIFS period
				if(EDCA && (rtime_[pri] > slottime) ) {
					rtime_[pri] -= slottime;
					//printf("\nEDCA");
					//Added by Ruby
					//checkNearbyDRPorBeacon(&stime_[pri], &rtime_[pri], 0);
					/* P802.11e/D13.0
					 * Section 9.9.1.3:
					 * one and only one of the following actions is allowed at slot boundaries:
					 * - decrement of backoff timer, or
					 * - initiate transmission, or
					 * - backoff procedure due to internal collision, or
					 * - do nothing.
					 * Thus we need to assure that a station does not initiate a transmission 
					 * immediately after decreasing rtime.The check is done in restart()/resume() 
					 * by comparing the time of decrement and simulated time.
					 */
					decremented_[pri]=st; // time of decrement 
				} else if (!rounding(stime_[pri],st)){ 
					// this backoff has just been started in this moment, while others are busy,
					// do nothing
					if(BDEBUG>2) printf("now %4.8f Mac: %d BackoffTimer::pause new EDCA rule stime_[%d] %4.8f rtime_[%d] %2.2f AIFSwait[%d] %2.2f slottime %2.2f diff/slottime %2.2f \n",
							    s.clock(), mac->index_, pri, stime_[pri], pri, MU(rtime_[pri]), pri, MU(AIFSwait_[pri]),MU(slottime), sr/slottime); 

				} else {
					if(BDEBUG>2) printf("now %4.8f Mac: %d BackoffTimer::pause new EDCA rule, but rtime < slottime! stime_[%d] %4.8f rtime_[%d] %2.2f AIFSwait[%d] %2.2f slottime %2.2f diff/slottime %2.2f\n",
							    s.clock(), mac->index_, pri, stime_[pri], pri, MU(rtime_[pri]), pri, MU(AIFSwait_[pri]),MU(slottime), sr/slottime); 
					exit(1);
				}
			} else if(sr > 0 + ROUND) {
				slots = int (floor((sr + ROUND)/slottime)); 
			}

			assert(busy_ && !paused_);
			if( (rtime_[pri] - (slots * slottime) > 0.0) || (rtime_[pri] - (slots * slottime) > ROUND) ) {
				if(EDCA)rtime_[pri] -= ((slots + 1) * slottime); // +1 slot for new EDCA rule
				else rtime_[pri] -= (slots * slottime); 
				//Added by Ruby
				//checkNearbyDRPorBeacon(&stime_[pri], &rtime_[pri], 0);
				decremented_[pri]=st;
				assert(rtime_[pri] >= 0.0);	
			} else{
				if(rounding(rtime_[pri] - (slots * slottime),0.0)) { // difference within rounding errors 
					rtime_[pri] = 0;
					decremented_[pri]=st;
				} else {
					printf("ERROR in BackoffTimer::pause(), rtime_[%d]  %2.4f sr %2.4f slots: %d, SlotTime: %2.4f slots*slottime %2.4f \n", 
					       pri, MU(rtime_[pri]), MU(sr), slots, slottime, MU(slots*slottime));
					exit(0);
				}	
			}
			/*
			 * decrease of AIFSwait is required because pause() and restart() can
			 * be called immediately afterwards (in case of postbackoff), so that
			 * normal backoff operation must proceed
			 */
			if(st - stime_[pri] >= mac->getAIFS(pri)) AIFSwait_[pri] = 0.0;
			else{
				AIFSwait_[pri] -= st - stime_[pri];
				if(AIFSwait_[pri] < 0) AIFSwait_[pri] = 0;
			}
			if(BDEBUG>2) printf("now %4.8f Mac: %d BackoffTimer::pause end stime_[%d] %4.8f rtime_[%d] %2.2f AIFSwait[%d] %2.2f slottime %2.2f diff/slottime %2.2f \n",
					    s.clock(), mac->index_,pri, stime_[pri], pri, MU(rtime_[pri]), pri, MU(AIFSwait_[pri]),MU(slottime), sr/slottime);
		}
	}
	s.cancel(&intr);
	//printf("\npause method:%lf, check_pause:%d", s.clock(), check_pause);
	paused_ = 1;
}


void
BackoffTimer_802_11e::resume()
{
	if(BDEBUG>2) printf("now %4.8f Mac: %d in BackoffTimer::resume\n",
			   Scheduler::instance().clock(), mac->index_);
	double delay = INF;
	int prio = MAX_PRI + 1;
	Scheduler &s = Scheduler::instance();

	//printf("\nresume:%lf", s.clock());
	
	for(int pri = 0; pri < MAX_PRI; pri++){
		if(backoff_[pri]){
			double delay_=0;
			round_time(pri);
			AIFSwait_[pri] = mac->getAIFS(pri);
			stime_[pri] = s.clock();
			if(EDCA && rounding(decremented_[pri],s.clock())) delay_ = rtime_[pri] + AIFSwait_[pri] + slottime;
			else delay_ = rtime_[pri] + AIFSwait_[pri];
			//decremented_[pri]=0;
			if((delay_ < delay) && backoff_[pri]) {
				delay = delay_;
				prio = pri;
			}
			if(BDEBUG>2) printf("now %4.8f Mac: %d BackoffTimer::resume check for smallest delay stime_[%d] %4.8f rtime_[%d] %2.2f AIFSwait[%d] %2.2f slottime %2.2f \n",
					    s.clock(), mac->index_, pri, stime_[pri], pri, MU(rtime_[pri]), pri, MU(AIFSwait_[pri]),MU(slottime));
		}
	}
	if(prio < MAX_PRI + 1) {
		assert(rtime_[prio] + AIFSwait_[prio] >=0);
		s.schedule(this, &intr, rtime_[prio] + AIFSwait_[prio]);
		paused_ = 0;
	} else {
		cout<<"ERROR: wrong priority in BackoffTimer::resume() \n";
		exit(0);
	}
	if(BDEBUG>2) printf("now %4.8f Mac: %d BackoffTimer::resume next: prio %d stime_[%d] %4.8f rtime_[%d] %2.2f AIFSwait[%d] %2.2f slottime %2.2f \n",
			    s.clock(), mac->index_,prio, prio, stime_[prio], prio, MU(rtime_[prio]), prio, MU(AIFSwait_[prio]),MU(slottime));
	if(mac->mhBeacon_.get_beacon_period_flag() == 1 || mac->mhDRP_.get_drp_period_flag() == 1)
	{
		pause();
	}
}

void
BackoffTimer_802_11e::restart()
{
	if(BDEBUG>2) printf("now %4.8f Mac: %d in BackoffTimer::restart\n",
			   Scheduler::instance().clock(), mac->index_);
	busy_ = 1;
	double delay = INF;
	int prio = MAX_PRI + 1;
	Scheduler &s = Scheduler::instance();

	//printf("\nrestart:%lf", s.clock());
	
	for(int pri = 0; pri < MAX_PRI; pri++){
		if(backoff_[pri]) {
			double delay_=0;
			round_time(pri);
			stime_[pri] = s.clock();
			if(EDCA && rounding(decremented_[pri],s.clock())) delay_ = rtime_[pri] + AIFSwait_[pri] + slottime;
			else delay_ = rtime_[pri] + AIFSwait_[pri];
			//decremented_[pri]=0;
			if(delay_ < delay) {
				delay = delay_;
				prio = pri;
			}
		}
	}
	if(prio < MAX_PRI + 1) {
		assert(rtime_[prio] + AIFSwait_[prio] >=0);
		//printf("\nrtime:%lf, clock:%lf, prio:%d", rtime_[prio], s.clock(), prio);
		s.schedule(this, &intr, rtime_[prio] + AIFSwait_[prio]);
		paused_ = 0;
	} else {
		cout<<"ERROR: wrong priority in BackoffTimer::restart() \n";
		exit(0);
	}
}


int
BackoffTimer_802_11e::backoff(int pri)
{
	return backoff_[pri];
}

/*
 * Timer for throughput measurement 
 */ 
void AkaroaTimer::start()
{
	Scheduler &s = Scheduler::instance();
	s.schedule(this, &intr,mac->interval_);
}

void AkaroaTimer::handle(Event *)
{
	mac->calc_throughput();
}

// Written by Ruby
int BeaconTimer_UWB::check_beacon_start()
{
	Scheduler &s = Scheduler::instance();
	double cur_time = s.clock();
	
	double res = doMod(cur_time, SUPER_FRAME_TIME);
	//if(cur_time == 0.196608)
	//	printf("\ncheck_beacon_start: %lf, res:%lf", cur_time, res);
	if(rounding(res, 0.0))
		return 1;
	else return 0;
	
}

// Written by Ruby
void
BeaconTimer_UWB::handle(Event *)
{
	int pri;
	Scheduler &s = Scheduler::instance();
	//printf("\ninside BeaconTimer Handle");
	
	
	int flag = check_beacon_start();
	//int flag = 0;
	//printf("\nhandle:%d and clock:%lf", flag, s.clock());
	if(flag == 0)
	{
		char eventtype[] = "BeaconTimer Handle flag0";
		mac->trace_event(eventtype, NULL);
		beacon_period_flag = 0;
		//printf("\nbeacon_period_flag0: %lf", s.clock());
		int send_flag = 0;
		if(mac->mhSend_.get_start_flag())
		{
			mac->mhIF_.start(0, 2);
			mac->mhSend_.start(0, 2);
			send_flag = 1;
		}
		for(pri=0;pri<MAX_PRI;pri++)
		{
			if(mac->mhDefer_.get_int_flag(pri) && send_flag==0)
			{
				mac->mhDefer_.set_int_flag(pri, 0);
				mac->mhDefer_.start(pri, mac->mhDefer_.get_rtime(pri));		
			}
		}
		if(mac->mhBackoff_.paused() && send_flag==0)
		{
			//printf("\nmhBeacon handle:%lf", s.clock());
		    	mac->mhBackoff_.resume();
		} 
		if(mac->mhNav_.paused())
		{
		  	mac->mhNav_.resume();
		}
		
		start(0.057344);
	}
	else if(flag == 1)
	{
		char eventtype[] = "BeaconTimer Handle flag1";
		mac->trace_event(eventtype, NULL);
		beacon_period_flag = 1;
		//printf("\nbeacon_period_flag1: %lf", s.clock());
		if(mac->mhDefer_.busy())
		{
			mac->mhDefer_.stop(2);
		}
		if(mac->mhBackoff_.busy() && ! mac->mhBackoff_.paused())
		{  
			mac->mhBackoff_.pause();				       
		}
		if(mac->mhNav_.busy() && !mac->mhNav_.paused())
		{
		  	mac->mhNav_.pause();
		}
		start(0.008192);
	}
	
}


// Written by Ruby
void
BeaconTimer_UWB::start(double time)
{
	Scheduler &s = Scheduler::instance();
	char eventtype[] = "BeaconTimer start";
	mac->trace_event(eventtype, NULL);
	//printf("\ninside BeaconTimer start:%lf", s.clock());
	stime = s.clock();
	rtime = time;
	assert(rtime >= 0.0);
	s.schedule(this, &intr, rtime);
		
}



/*
   Written by Ruby
   return 1 for drp start
   return 2 for drp end
*/
int DRPTimer_UWB::findDRPStartOrEnd(int *drp_index)
{
	int i;
	Scheduler &s = Scheduler::instance();
	double cur_time = s.clock();
	double cur_time_ = doMod(cur_time, SUPER_FRAME_TIME);
	//printf("\ncur time: %lf", cur_time_);
	for(i=0;i<drpListLength;i++)
	{
		double drp_start = drpList[i]*SUPER_FRAME_SLOT_TIME;
		double drp_end = drpList[i]*SUPER_FRAME_SLOT_TIME + SUPER_FRAME_EACH_COL*2;
		//printf("\nindex: %d, drpList: %d", i, drpList[i]);
		//printf("\ndrp_start: %lf, drp_end: %lf", drp_start, drp_end);
		if(rounding(cur_time_, drp_start))
		{
			*drp_index = i;
			return 1;
		}
		if(rounding(cur_time_, drp_end))
		{
			*drp_index = i;
			return 2;
		}
	}
	return 0; 
}

/*
Written by Ruby
Find DRPTimer's remaining by checking whether it is drp start or drp end
*/
double DRPTimer_UWB::findDRPRemTime(int drp_index, int start_end)
{
	Scheduler &s = Scheduler::instance();
	double rtime = 0;
	double cur_time = s.clock();
	double cur_time_ = doMod(cur_time, SUPER_FRAME_TIME);
	/*
	  1 for drp start
	  2 for drp end
	*/
	if(start_end == 1)
	{
		rtime = SUPER_FRAME_EACH_COL*2;
	}
	else if(start_end == 2)
	{
		if(drp_index == drpListLength-1)
		{
			rtime = SUPER_FRAME_TIME - cur_time_ + drpList[0]*SUPER_FRAME_SLOT_TIME;
		}
		else
		{
			rtime = drpList[drp_index+1]*SUPER_FRAME_SLOT_TIME - cur_time_;
		}
	}
	//printf("\ncur time: %lf, rtime: %lf", cur_time, rtime);
	assert(rtime != 0);
	return rtime;
}

// Written by Ruby
void
DRPTimer_UWB::handle(Event *)
{
	int pri, drp_index;
	double rtime;
	Scheduler &s = Scheduler::instance();
	int start_end = findDRPStartOrEnd(&drp_index);
	assert(start_end !=0);
	rtime = findDRPRemTime(drp_index, start_end);
	if(start_end == 2)
	{
		char eventtype[] = "DRPTimer Handle flag2";
		mac->trace_event(eventtype, NULL);
		drp_period_flag = 0;
		//printf("\ndrp_period_flag0: %lf", s.clock());
		int send_flag = 0;
		if(mac->mhSend_.get_start_flag())
		{
			mac->mhIF_.start(0, 2);
			mac->mhSend_.start(0, 2);
			send_flag = 1;
		}
		for(pri=0;pri<MAX_PRI;pri++)
		{
			if(mac->mhDefer_.get_int_flag(pri) && send_flag==0)
			{
				mac->mhDefer_.set_int_flag(pri, 0);
				mac->mhDefer_.start(pri, mac->mhDefer_.get_rtime(pri));		
			}
		}
		if(mac->mhBackoff_.paused() && send_flag==0)
		{
			//printf("\nmhBeacon handle:%lf", s.clock());
		    	mac->mhBackoff_.resume();
		} 
		if(mac->mhNav_.paused())
		{
		  	mac->mhNav_.resume();
		}
		
		start(rtime);
	}
	else if(start_end == 1)
	{
		char eventtype[] = "BeaconTimer Handle flag1";
		mac->trace_event(eventtype, NULL);
		drp_period_flag = 1;
		//printf("\ndrp_period_flag1: %lf", s.clock());
		if(mac->mhDefer_.busy())
		{
			mac->mhDefer_.stop(2);
		}
		if(mac->mhBackoff_.busy() && ! mac->mhBackoff_.paused())
		{  
			mac->mhBackoff_.pause();				       
		}
		if(mac->mhNav_.busy() && !mac->mhNav_.paused())
		{
		  	mac->mhNav_.pause();
		}
		start(rtime);
	}
	
	
	
}

// Written by Ruby
void
DRPTimer_UWB::setDRPList(int drpList_[], int drpListLength_)
{
	int i;
	drpListLength = drpListLength_;
	for(i=0;i<drpListLength;i++)
	{
		drpList[i] = drpList_[i];
	}
	
	
}

// Written by Ruby
void
DRPTimer_UWB::start(double time)
{
	Scheduler &s = Scheduler::instance();
	stime = s.clock();
	rtime = time;
	assert(rtime >= 0.0);
	s.schedule(this, &intr, rtime);
	
}
