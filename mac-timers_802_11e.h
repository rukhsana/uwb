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
/* Ported from CMU/Monarch's code, nov'98 -Padma.*/

#ifndef __mac_timers_802_11e_h__
#define __mac_timers_802_11e_h__

// Added by Ruby

#define MAX_PRI 4 
#define SUPER_FRAME_TIME 0.065536
#define SUPER_FRAME_EACH_COL 0.004096
#define SUPER_FRAME_SLOT_TIME 0.000256


/*
//DRP: 101-150 (25601-38400), 176-225 (45056-57600)
//Beacon period: 1-32 (0-8192)

//In case of pkt transmission, change start time
//While decrementing backoff counter, change remaining time  

double adjustRemTimeForBackoff(double start_time, double cur_time)
{
	double beacon_period = 0.008192;
	double elapsed_time = cur_time - start_time;
	double tmp_elapsed_time = elapsed_time;
	double start_time_ = doMod(start_time, 0.065536);
	double tmp_dur1 = start_time_ + elapsed_time;
	//start_time can be bigger than beacon period
	else if(start_time_ >= 0.008192 && tmp_dur1 > 0.065536)
	{
		//start_time = start_time + 0.008192;
		tmp_elapsed_time = tmp_elapsed_time - 0.008192;
	}
	//start_time_ = doMod(start_time, 0.065536);
	//tmp_dur1 = start_time_ + elapsed_time;
	
	double drp_start = 0.000256*100;
	double drp_end = 0.000256*150;

	//start_time can be before DRP period
	if(start_time_ <= drp_start && tmp_dur1 > drp_start)
	{
		//start_time = start_time + drp_end - drp_start;
		tmp_elapsed_time = tmp_elapsed_time - (drp_end - drp_start);
		
	}
	return tmp_elapsed_time;
}

bool checkNearbyDRPorBeacon(double *start_time, double *erem_time, double tot_rem_time, int pkt_tx_duration)
{
	double beacon_period = 0.008192;
	double start_time_ = doMod(*start_time, 0.065536);
	*erem_time = tot_rem_time;

	//double tmp_dur2 = doMod(tmp_dur1, 0.065536);

	if(pkt_tx_duration == 0)
	{
		double tmp_dur1 = start_time_ + *erem_time;
		//start_time can be on beacon period
		if(start_time_ < 0.008192)
		{
			*start_time = *start_time + 0.008192 - start_time_;
		}
		//start_time can be bigger than beacon period
		else if(start_time_ > 0.008192 && tmp_dur1 > 0.065536)
		{
			*erem_time = *erem_time + 0.008192;
		}
	        

		double drp_start = 0.000256*100;
		double drp_end = 0.000256*150;
		//start_time can be before DRP period
		if(start_time_ <= drp_start && tmp_dur1 > drp_start)
		{
			*erem_time = *erem_time + drp_end - drp_start;
		}

		//start_time can be on the DRP period
		else if(start_time_ >= drp_start && start_time_ < drp_end)
		{
			*start_time = *start_time + (start_time_ - drp_end);
		} 	
	}
	else 
	{
		double tmp_dur1 = start_time_ + getFrameTxDur();
		//start_time can be on beacon period
		if(start_time_ < 0.008192)
		{
			*start_time = *start_time + 0.008192 - start_time_;
		}
		//start_time can be bigger than beacon period
		else if(start_time_ >= 0.008192 && tmp_dur1 > 0.065536)
		{
			*start_time = 0.008192;
		}
	        

		double drp_start = 0.000256*100;
		double drp_end = 0.000256*150;


		//start_time can be before DRP period
		if(start_time_ <= drp_start && tmp_dur1 > drp_start)
		{
			*start_time = *start_time + (start_time_ - drp_end);
		}

		//start_time can be on the DRP period
		else if(start_time_ >= drp_start && start_time_ < drp_end)
		{
			*start_time = *start_time + (start_time_ - drp_end);
		} 	
	}		
	//txtime(phymib_.getACKlen(), basicRate_) + DSSS_EDCA_MaxPropagationDelay + sifs_

}*/
/* End */

/* ======================================================================
   Timers
   ====================================================================== */
class Mac802_11e;

class MacTimer_802_11e : public Handler {
public:
	MacTimer_802_11e(Mac802_11e* m, double s = 0) : mac(m), slottime(s) {
		printf("\ntimer slottime: %lf", slottime);
		busy_ = paused_ = 0; stime = rtime = 0.0;
	}

	virtual void handle(Event *e) = 0;

	virtual void start(double time);
	virtual void stop(void);
	virtual void pause(void) { assert(0); }
	virtual void resume(void) { assert(0); }

	inline int busy(void) { return busy_; } 
	inline int paused(void) { return paused_; }
	inline bool rounding(double x, double y);
	inline double expire(void) {
		return ((stime + rtime) - Scheduler::instance().clock());
	}
	/*
	 *Methods added by Ruby
	 */
	inline double getSlottime(void) { return slottime; } 
	double doMod(double aNumber, double aDivisor);
	bool checkNearbyDRPorBeacon(double *start_time, double *rem_time, int pkt_tx_duration);
	double getFrameTxDur(void);
       /* End */
protected:
	Mac802_11e	*mac;
	int		busy_;
	int		paused_;
	Event		intr;
	double		stime;	// start time
	double		rtime;	// remaining time
	double		slottime;
};


class BackoffTimer_802_11e : public MacTimer_802_11e {
public:
	BackoffTimer_802_11e(Mac802_11e *m, double s) : MacTimer_802_11e(m, s) 
	{
		for (int pri = 0; pri < MAX_PRI; pri++){
			AIFSwait_[pri] = 0.0;
			stime_[pri] = rtime_[pri] = 0;
			backoff_[pri] = 0;
			decremented_[pri] = 0.0;
		}
		levels = 0;
	}

	void	start(int pri, int cw, int idle);
	void	handle(Event *e);
	void	pause(void);
	void	resume();
	int     backoff(int pri);

private:
	inline void round_time(int pri);
	inline bool rounding(double x, double y);
	int     backoff_[MAX_PRI]; //for handler
	void    restart();
	double	AIFSwait_[MAX_PRI];
	double  rtime_[MAX_PRI];
	//Added by Ruby
	double ertime_[MAX_PRI];
	
	double  stime_[MAX_PRI];
	double  decremented_[MAX_PRI];
	int     levels; // get number of levels out of priq.cc or mac-802_11e.cc
	bool    pause_restart;
       // End
};

class DeferTimer_802_11e : public MacTimer_802_11e {
public:
	DeferTimer_802_11e(Mac802_11e *m, double s) : MacTimer_802_11e(m,s) {
		for(int i=0;i<MAX_PRI;i++) {
			defer_[i] = 0;
			rtime_[i] = stime_[i] = 0;
		}
		prio = -1;
	}

	void	start(int pri, double time);
	void    pause(void);
	void    stop(int flag);
	void	handle(Event *e);
	int     defer(int pri);
	//Added by Ruby
	double get_rtime(int pri);
	void set_int_flag(int pri, int value);
	int get_int_flag(int pri);
        // End
	
private:
	int prio;
	int defer_[MAX_PRI];
 	double rtime_[MAX_PRI];
        	
        // Added by Ruby
	double stime_[MAX_PRI];
	int int_flag[MAX_PRI];
        // End
};

class SIFSTimer_802_11e : public MacTimer_802_11e {
public:
	SIFSTimer_802_11e(Mac802_11e *m, double s) : MacTimer_802_11e(m,s) {
		for(int i=0;i<MAX_PRI;i++) {
			sifs_[i] = 0;
			rtime_[i] = stime_[i] = 0;
		}
		prio = -1;
	}

	void	start(int pri, double time);
	void	handle(Event *e);
private:
	int    prio;
	int    sifs_[MAX_PRI];
	double rtime_[MAX_PRI];
	double stime_[MAX_PRI];
};

class IFTimer_802_11e : public MacTimer_802_11e {
private:
	int start_flag;
public:
	IFTimer_802_11e(Mac802_11e *m) : MacTimer_802_11e(m) {}
	int get_start_flag();
	void start(double time, int flag);
	void	handle(Event *e);
};


class NavTimer_802_11e : public MacTimer_802_11e {
public:
	NavTimer_802_11e(Mac802_11e *m) : MacTimer_802_11e(m) {}
	void    start(double time);
	void    pause(void);
	void    resume(void);
	void	handle(Event *e);
};

class RxTimer_802_11e : public MacTimer_802_11e {
public:
	RxTimer_802_11e(Mac802_11e *m) : MacTimer_802_11e(m) {}

	void	handle(Event *e);
};

class TxTimer_802_11e : public MacTimer_802_11e {
private:
	int start_flag;
public:
	TxTimer_802_11e(Mac802_11e *m) : MacTimer_802_11e(m) {}
	
	int get_start_flag();
	void start(double time, int flag);
	void	handle(Event *e);
};

/* Added by Ruby */

class BeaconTimer_UWB : public MacTimer_802_11e {
	int beacon_period_flag;
public:
	BeaconTimer_UWB(Mac802_11e *m, double s) : MacTimer_802_11e(m, s) {beacon_period_flag = 0;}
	inline int get_beacon_period_flag(void) { return beacon_period_flag; } 
	void start(double time);
	void	handle(Event *e);
	int check_beacon_start();
};

class DRPTimer_UWB : public MacTimer_802_11e {
public:
	DRPTimer_UWB(Mac802_11e *m, double s) : MacTimer_802_11e(m, s) {drp_period_flag = 0;}
	
	void start(double time);
	void setDRPList(int drpList_[], int drpListLength_);
	void	handle(Event *e);
	double findDRPRemTime(int drp_index, int start_end);
	inline int get_drp_period_flag(void) { return drp_period_flag; } 
	int findDRPStartOrEnd(int *drp_index);
	int drpList[10];
	int drpListLength;
private:
	int drp_period_flag;

	
};

/* End */


class AkaroaTimer : public MacTimer_802_11e {
public:
	AkaroaTimer(Mac802_11e *m) : MacTimer_802_11e(m) {}
	void start();
	void handle(Event *e);
};
	
#endif /* __mac_timers_802_11e_h__ */
	//void start(double time);

