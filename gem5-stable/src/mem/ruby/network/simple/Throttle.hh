/*
 * Copyright (c) 1999-2008 Mark D. Hill and David A. Wood
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * The class to implement bandwidth and latency throttle. An instance
 * of consumer class that can be woke up. It is only used to control
 * bandwidth at output port of a switch. And the throttle is added
 * *after* the output port, means the message is put in the output
 * port of the PerfectSwitch (a intermediateBuffers) first, then go
 * through the Throttle.
 */

#ifndef __MEM_RUBY_NETWORK_SIMPLE_THROTTLE_HH__
#define __MEM_RUBY_NETWORK_SIMPLE_THROTTLE_HH__

#include <iostream>
#include <string>
#include <vector>

#include "mem/ruby/common/Consumer.hh"
#include "mem/ruby/common/Global.hh"
#include "mem/ruby/network/Network.hh"
#include "mem/ruby/system/System.hh"
#include "cpu/testers/rubytest/Trace.hh"

#include <stdlib.h> // rand function

class MessageBuffer;

class Throttle : public Consumer
{
  public:
    Throttle(int sID, NodeID node, Cycles link_latency,
             int link_bandwidth_multiplier, int endpoint_bandwidth,
             ClockedObject *em);
    Throttle(NodeID node, Cycles link_latency, int link_bandwidth_multiplier,
             int endpoint_bandwidth, ClockedObject *em);
    ~Throttle() {}

    std::string name()
    { return csprintf("Throttle-%i", m_sID); }

		int returnSID() { return m_sID;}

    void addLinks(const std::vector<MessageBuffer*>& in_vec,
                  const std::vector<MessageBuffer*>& out_vec);
    void wakeup();

		void serviceRequest(MessageBuffer* in, MessageBuffer *out, int vnet, int pos);

    // The average utilization (a fraction) since last clearStats()
    const Stats::Scalar & getUtilization() const
    { return m_link_utilization; }
    const Stats::Vector & getMsgCount(unsigned int type) const
    { return m_msg_counts[type]; }

    int getLinkBandwidth() const
    { return m_endpoint_bandwidth * m_link_bandwidth_multiplier; }

    Cycles getLatency() const { return m_link_latency; }

		bool getVnetToken() { return vToken;}
		void setSharedMemBlock() { smBlock = true; }
		void setSharedMemUnblock() { smBlock = false;}
		bool getSharedMemStatus() { return smBlock;}

		void changeVnetToken() { 
			vToken = vToken ^ 1; 
			if (vToken) 
				setSharedMemBlock(); 
			else 
				setSharedMemUnblock(); 
		}

		bool isSharedMemUnblock() { return !smBlock;}
	
		bool isStartOfSlot() {
			unsigned int slot_period = NPROC*SLOT_WIDTH;
			for (unsigned int i = 0; i<NPROC; i++) {
				if (g_system_ptr->curCycle() % slot_period == i*SLOT_WIDTH) return true;
			}
			return false;
		}	

		int getSlotOwner() {
			unsigned int slot_period = NPROC*SLOT_WIDTH;
			for (unsigned int i = 0; i<NPROC; i++) {
				if (g_system_ptr->curCycle() % slot_period < (i+1)*SLOT_WIDTH) return i;
			}
			return -1;
		}

		bool isStartDemandSlot(){
			unsigned int slot_period = NPROC*SLOT_WIDTH;
			for (unsigned int i = 0; i<NPROC; i++) {
				if (g_system_ptr->curCycle() % slot_period == i*SLOT_WIDTH) return true;
			}
			return false;
		}

		bool isStartWBSlot() {
			unsigned int slot_period = NPROC*SLOT_WIDTH;
			for (unsigned int i = 0; i<NPROC; i++) {
				if (g_system_ptr->curCycle() % (2*slot_period) == (i+NPROC)*SLOT_WIDTH) return true;
			}
			return false;
		}


		void setSlotType() {
			if (READ_WB_RANDOM) {
				unsigned int tmp = rand() % 10;
				if (tmp%2 == 0) {
					vToken = false;
				}
				else {
					vToken = true;
				}
			}
			else if (BASELINE) {
				if (isStartOfSlot()) {
					vToken = 0;
				}
			}
			else {
				if (isStartDemandSlot()) {
					vToken = 0;	
					//setSharedMemUnblock(); 
				}
				if (isStartWBSlot()) {
					vToken = 1;
					//setSharedMemBlock(); 
				}
			}
		}

    void clearStats();
    void collateStats();
    void regStats(std::string name);
    void print(std::ostream& out) const;
		NodeID returnNodeID() { return m_node;}
		unsigned int returnVnets() { return m_vnets;}

  private:
    void init(NodeID node, Cycles link_latency, int link_bandwidth_multiplier,
              int endpoint_bandwidth);
    void operateVnet(int vnet, int &bw_remainin, bool &schedule_wakeup,
                     MessageBuffer *in, MessageBuffer *out, bool recheck);

    // Private copy constructor and assignment operator
    Throttle(const Throttle& obj);
    Throttle& operator=(const Throttle& obj);

    std::vector<MessageBuffer*> m_in;
    std::vector<MessageBuffer*> m_out;
    unsigned int m_vnets;
    std::vector<int> m_units_remaining;

    int m_sID;
    NodeID m_node;
    int m_link_bandwidth_multiplier;
    Cycles m_link_latency;
    int m_wakeups_wo_switch;
    int m_endpoint_bandwidth;
		bool doCheck;


    // Statistical variables
    Stats::Scalar m_link_utilization;
    Stats::Vector m_msg_counts[MessageSizeType_NUM];
    Stats::Formula m_msg_bytes[MessageSizeType_NUM];

    double m_link_utilization_proxy;

		// RR work-conserving token between WB and demand requests
		// 0 : demand
		// 1 : WB
		bool vToken;

		// 0: Shared memory is unblocked, can send data responses to cores
		// 1: Shared memory is blocked, cannot send data responses to cores
		bool smBlock;
};

inline std::ostream&
operator<<(std::ostream& out, const Throttle& obj)
{
    obj.print(out);
    out << std::flush;
    return out;
}

#endif // __MEM_RUBY_NETWORK_SIMPLE_THROTTLE_HH__
