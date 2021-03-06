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

#include <cassert>

#include "base/cast.hh"
#include "base/cprintf.hh"
#include "debug/RubyNetwork.hh"
#include "mem/ruby/network/simple/Throttle.hh"
#include "mem/ruby/network/simple/PerfectSwitch.hh"
#include "mem/ruby/network/MessageBuffer.hh"
#include "mem/ruby/network/Network.hh"
#include "mem/ruby/slicc_interface/NetworkMessage.hh"
#include "mem/ruby/system/System.hh"

using namespace std;

const int MESSAGE_SIZE_MULTIPLIER = 1000;
//const int BROADCAST_SCALING = 4; // Have a 16p system act like a 64p systems
const int BROADCAST_SCALING = 1;
const int PRIORITY_SWITCH_LIMIT = 128;

static int network_message_to_size(NetworkMessage* net_msg_ptr);


Throttle::Throttle(int sID, NodeID node, Cycles link_latency,
                   int link_bandwidth_multiplier, int endpoint_bandwidth,
                   ClockedObject *em)
    : Consumer(em)
{
    init(node, link_latency, link_bandwidth_multiplier, endpoint_bandwidth);
    m_sID = sID;	
		vToken = 0;
		smBlock = false;
		doCheck = false;
}

Throttle::Throttle(NodeID node, Cycles link_latency,
                   int link_bandwidth_multiplier, int endpoint_bandwidth,
                   ClockedObject *em)
    : Consumer(em)
{
    init(node, link_latency, link_bandwidth_multiplier, endpoint_bandwidth);
    m_sID = 0;
}

void
Throttle::init(NodeID node, Cycles link_latency,
               int link_bandwidth_multiplier, int endpoint_bandwidth)
{
    m_node = node;
    m_vnets = 0;

    assert(link_bandwidth_multiplier > 0);
    m_link_bandwidth_multiplier = link_bandwidth_multiplier;

    m_link_latency = link_latency;
    m_endpoint_bandwidth = endpoint_bandwidth;

    m_wakeups_wo_switch = 0;
    m_link_utilization_proxy = 0;
}

void
Throttle::addLinks(const vector<MessageBuffer*>& in_vec,
                   const vector<MessageBuffer*>& out_vec)
{
    assert(in_vec.size() == out_vec.size());

    for (int vnet = 0; vnet < in_vec.size(); ++vnet) {
        MessageBuffer *in_ptr = in_vec[vnet];
        MessageBuffer *out_ptr = out_vec[vnet];

        m_vnets++;
        m_units_remaining.push_back(0);
        m_in.push_back(in_ptr);
        m_out.push_back(out_ptr);

        // Set consumer and description
        in_ptr->setConsumer(this);
        string desc = "[Queue to Throttle " + to_string(m_sID) + " " +
            to_string(m_node) + "]";
        in_ptr->setDescription(desc);
    }
}

void
Throttle::serviceRequest(MessageBuffer *in, MessageBuffer *out, int vnet, int pos) {
	MsgPtr msg_ptr = NULL;
	if (pos == -1) {
		msg_ptr = in->peekMsgPtr();
	}
	else {
		msg_ptr = in->peekMsgPtr(pos);
	}

	NetworkMessage* net_msg_ptr = safe_cast<NetworkMessage*>(msg_ptr.get());
	m_units_remaining[vnet] += network_message_to_size(net_msg_ptr);

	DPRINTF(RubyNetwork, "net_msg_ptr: %s\n", msg_ptr);

	if (pos == -1) {
		in->dequeue();
	}
	else {
		in->dequeue(pos);
	}

	out->enqueue(msg_ptr, m_link_latency);

	m_msg_counts[net_msg_ptr->getMessageSize()][vnet]++;
}

void
Throttle::operateVnet(int vnet, int &bw_remaining, bool &schedule_wakeup,
                      MessageBuffer *in, MessageBuffer *out, bool recheck)
{
		// Get the slotOwner and return if this is start of period

		int slotOwner = getSlotOwner();
		bool slotStart = isStartOfSlot(); 

		MessageBuffer *wbQueue = m_in[6];
		MessageBuffer *demandQueue = m_in[2];
		
		if (!BASELINE) {
if (slotStart && (vnet == 6) && returnSID() <4) {
				setSlotType();
				bool isWBSlot = getVnetToken();
				DPRINTF(RubyNetwork, "m_SID: %s, SlotOwner: %s, slotStart: %s, isWBSlot: %s, smBlock: %s vnet: %s\n", returnSID(), slotOwner, slotStart, isWBSlot, getSharedMemStatus(), vnet);
				if (isWBSlot) {
					// Check if vnet 6 is non zero
					if (!wbQueue->isReady() && demandQueue->isReady()) {
						changeVnetToken();
						setSharedMemUnblock();
						DPRINTF(RubyNetwork, "Changing vnet %s token from WB to Demand: %s shared mem status: %s\n", vnet, getVnetToken(), getSharedMemStatus());
					}
					else {
						// We need to block the shared mem queue
						setSharedMemBlock();
					}
				}
				else {
					if (!demandQueue->isReady() && wbQueue->isReady()) {
						changeVnetToken();
						setSharedMemBlock();
						DPRINTF(RubyNetwork, "Changing vnet %s token from Demand to WB: %s shared mem status: %s\n", vnet, getVnetToken(), getSharedMemStatus());
					}
					else {
						// We need to unblock the shared mem queue
						setSharedMemUnblock();
					}
				}
			}
		}
		else {
if (slotStart && returnSID() <4) {
				setSlotType();
			}
		}

    if (out == nullptr || in == nullptr) {
    	return;
    }

    assert(m_units_remaining[vnet] >= 0);	
    while (bw_remaining > 0 && (in->isReady() || m_units_remaining[vnet] > 0) &&
                                out->areNSlotsAvailable(1)) {
							
			PerfectSwitch* out_consumerSwitch = dynamic_cast<PerfectSwitch*>(out->getConsumer());
			Throttle* in_consumerThrottle = safe_cast<Throttle*>(in->getConsumer());
			
			// Re-read the isWBSlot
			bool isWBSlot = getVnetToken();

			// The arbitration logic is as follows:
			// Round-robin with slot width enough to complete a memory transfer
			// The shared memory knows whether a given slot for a core is for WB or demand request
			// There is a round-robin arbitration between vnets 6 and 2
			// If the token is on 2, then memory can send data to core
			// If the token is on 6, then memory cannot send data to core
			// bool vToken is the rr token between vnets 6 and 2
			
			if (out_consumerSwitch && in_consumerThrottle) {
				
				SwitchID switchID = out_consumerSwitch->returnSwitchID(); // Consider only switchID for central arbiter
				int SID = in_consumerThrottle->returnSID(); // Consider only SIDs of requestors

				DPRINTF(RubyNetwork, "slotOwner: %s, slotStart: %s, isWBSlot: %s, switchID: %s, SID: %s, smBlock: %s, vnet: %s\n", slotOwner, slotStart, isWBSlot, switchID, SID, !isSharedMemUnblock(), vnet);
				//
				// vToken = 1 ==> WB slot
				// vToken = 0 ==> demand request slot
				//
				bool vnetMatch = false;
				if (BASELINE) {
					if (vnet == 2) vnetMatch = true;
				}
				else {
					if (vnet == 6 || vnet == 2) vnetMatch = true;
				}

 if (switchID == 5 && SID < 4 && vnetMatch) {
					// This is for cases when the core sends a request (demand or WB) to the arbiter 
					if (slotOwner == SID && slotStart) {
						// This is the start of the slot of the core under analysis 
						if (vnet == 6) {
							// Vnet 6 is for WB. Check if this is a WB slot 
							if (isWBSlot) {
								if (WB_RANDOM) {
									unsigned int inSize = in->getQSize();
									int randomPos = rand() % inSize;
									if (m_units_remaining[vnet] == 0 && in->isReady()) {
										serviceRequest(in, out, vnet, randomPos);
									}
								}
								else {
									if (m_units_remaining[vnet] == 0 && in->isReady()) {
										serviceRequest(in, out, vnet, -1);
									}
								}
								int diff = m_units_remaining[vnet] - bw_remaining;
								m_units_remaining[vnet] = max(0, diff);
								bw_remaining = max(0, -diff);
							}
							if (in->isReady()) {
								schedule_wakeup = true;
							}
						}
						else {
							// Vnet 2 is for demand requests. Check if this is a demand request slot 
							if (!isWBSlot) {
								DPRINTF(RubyNetwork, "Servicing demand request");
								if (m_units_remaining[vnet] == 0 && in->isReady()) {
									serviceRequest(in, out, vnet, -1);
								}
								int diff = m_units_remaining[vnet] - bw_remaining;
								m_units_remaining[vnet] = max(0, diff);
								bw_remaining = max(0, -diff);
							}
							if (in->isReady()) {
								schedule_wakeup = true;
							}
						}
					}

					// Cannot schedule requests from vnets 2 and 6 other than start of the slot. 
					// Need to handle some special case
					if (vnet == 2 && in->isReady()) {
						MsgPtr msg_ptr = in->peekMsgPtr();
						NetworkMessage* net_msg_ptr = safe_cast<NetworkMessage*>(msg_ptr.get());
						DPRINTF(RubyNetwork, "SINGLE DESTINATION MSG, GETM FROM UPG? %s %s %s %s\n", net_msg_ptr->getDestination(), net_msg_ptr, net_msg_ptr->getDestination().getSize(), net_msg_ptr->getDestination().smallestElement());

						if (WRITE_HITS) {
							if (slotStart) {
								//MachineID ownID = {MachineType_L1Cache, (unsigned int)returnSID()};
								DPRINTF(RubyNetwork, "SPECIAL YO? %s", net_msg_ptr->getDestination().returnSpecial());
								if (net_msg_ptr->getDestination().returnSpecial()) {
									// This is an UPG message
									if (in->isReady()) {
										serviceRequest(in, out, vnet, -1);
									}
									int diff = m_units_remaining[vnet] - bw_remaining;
									m_units_remaining[vnet] = max(0, diff);
									bw_remaining = max(0, -diff);
								}
							}
						}
						else {
							// This is to check if this is a GETM following an UPG. 
							//MachineID dirID = {MachineType_Directory, (unsigned int)0};
							if (net_msg_ptr->getDestination().isSidePacket()) {
								if (m_units_remaining[vnet] == 0 && in->isReady()) {
									serviceRequest(in, out, vnet, -1);
								}
								int diff = m_units_remaining[vnet] - bw_remaining;
								m_units_remaining[vnet] = max(0, diff);
								bw_remaining = max(0, -diff);
							}
						}

						if (in->isReady()) {
							schedule_wakeup = true;
						}
					}
					else {
						schedule_wakeup = true;				
					}
					break;
				}
 else if (switchID == 5 && SID == 4) {
					// This case is for shared memory requests/responses to the arbiter 
					bool isSlotOwner = false;

					MachineID ownerID = {MachineType_L1Cache, (unsigned int)slotOwner};
					
					// Loop over all the ready requests in the message buffer, 
					// Overload isReady to check for index or something
					int pos = -1;
					DPRINTF(RubyNetwork, "QSIZE: %s", in->getQSize());
					for (int i = 0; i<in->getQSize(); i++) {
						if (in->isReady(i)) {
							DPRINTF(RubyNetwork, "RECEIVING SOMETHING FROM SHARED MEM\n");
							MsgPtr msg_ptr = in->peekMsgPtr(i);
							NetworkMessage* net_msg_ptr = safe_cast<NetworkMessage*>(msg_ptr.get());
							DPRINTF(RubyNetwork, "slotOwner: %s, destination: %s smBlock: %s", slotOwner, net_msg_ptr->getDestination(), in_consumerThrottle->getSharedMemStatus());

							// If the destination of the data sent by the shared memory matches the slot and this is not a WB slot for the destination core, send it
							if (net_msg_ptr->getDestination().isElement(ownerID) && in_consumerThrottle->isSharedMemUnblock()) {
								DPRINTF(RubyNetwork, "SETTING ISSLOTOWNER TO TRUE\n");
								isSlotOwner = true;
								pos = (int)i;
								break;
							}
							else {
								DPRINTF(RubyNetwork, "SETTING ISSLOTOWNER TO FALSE\n");
							}
						}
					}

					if (isSlotOwner) {
						DPRINTF(RubyNetwork, "m_units_remaining[vnet] :%s\n", m_units_remaining[vnet]);
						// ANI MOD: Change for picking ready requests in a non-fifo manner

						if (in->isReady(pos)) {
							DPRINTF(RubyNetwork, "SERVICING REQUEST");
							serviceRequest(in, out, vnet, pos);
						}
						int diff = m_units_remaining[vnet] - bw_remaining;
						m_units_remaining[vnet] = max(0, diff);
						bw_remaining = max(0, -diff);

						if (in->isReady()) {
							schedule_wakeup = true;
						}
					}
					else {
						schedule_wakeup = true;
					}
					break;
				}
				else {
					// All other cases 
					if (m_units_remaining[vnet] == 0 && in->isReady()) {
						serviceRequest(in, out, vnet, -1);
					}
					int diff = m_units_remaining[vnet] - bw_remaining;
					m_units_remaining[vnet] = max(0, diff);
					bw_remaining = max(0, -diff);

					if (in->isReady()) schedule_wakeup = true;
				}
				break;
			}
			else {
				// Default case 
				if (m_units_remaining[vnet] == 0 && in->isReady()) {
					serviceRequest(in, out, vnet, -1);
				}
				int diff = m_units_remaining[vnet] - bw_remaining;
				m_units_remaining[vnet] = max(0, diff);
				bw_remaining = max(0, -diff);
				break;
			}
    }

    if (bw_remaining > 0 && (in->isReady() || m_units_remaining[vnet] > 0) &&
                             !out->areNSlotsAvailable(1)) {
        DPRINTF(RubyNetwork, "vnet: %d", vnet);

        // schedule me to wakeup again because I'm waiting for my
        // output queue to become available				
        schedule_wakeup = true;
    }
}

void
Throttle::wakeup()
{
    // Limits the number of message sent to a limited number of bytes/cycle.
    assert(getLinkBandwidth() > 0);
    int bw_remaining = getLinkBandwidth();

    m_wakeups_wo_switch++;
    bool schedule_wakeup = false;

    // variable for deciding the direction in which to iterate
    bool iteration_direction = false;

		/*
		 * Comment this for predictable arbiter
    // invert priorities to avoid starvation seen in the component network
    if (m_wakeups_wo_switch > PRIORITY_SWITCH_LIMIT) {
        m_wakeups_wo_switch = 0;
        iteration_direction = true;
    }
		*/

		DPRINTF(RubyNetwork, "iteration_direction: %s", iteration_direction);

    if (iteration_direction) {
        for (int vnet = 0; vnet < m_vnets; ++vnet) {

            operateVnet(vnet, bw_remaining, schedule_wakeup,
                        m_in[vnet], m_out[vnet], false);
        }
    } else {
        for (int vnet = m_vnets-1; vnet >= 0; --vnet) {

            operateVnet(vnet, bw_remaining, schedule_wakeup,
                        m_in[vnet], m_out[vnet], false);
        }
    }

    // We should only wake up when we use the bandwidth
    // This is only mostly true
    // assert(bw_remaining != getLinkBandwidth());

    // Record that we used some or all of the link bandwidth this cycle
    double ratio = 1.0 - (double(bw_remaining) / double(getLinkBandwidth()));

    // If ratio = 0, we used no bandwidth, if ratio = 1, we used all
    m_link_utilization_proxy += ratio;

    if (bw_remaining > 0 && !schedule_wakeup) {
        // We have extra bandwidth and our output buffer was
        // available, so we must not have anything else to do until
        // another message arrives.
        DPRINTF(RubyNetwork, "%s not scheduled again\n", *this);
    } else {
        DPRINTF(RubyNetwork, "%s scheduled again\n", *this);

        // We are out of bandwidth for this cycle, so wakeup next
        // cycle and continue
        scheduleEvent(Cycles(1));
    }

		// Set the WB and demand slot based on the time stamp
}

void
Throttle::regStats(string parent)
{
    m_link_utilization
        .name(parent + csprintf(".throttle%i", m_node) + ".link_utilization");

    for (MessageSizeType type = MessageSizeType_FIRST;
         type < MessageSizeType_NUM; ++type) {
        m_msg_counts[(unsigned int)type]
            .init(Network::getNumberOfVirtualNetworks())
            .name(parent + csprintf(".throttle%i", m_node) + ".msg_count." +
                    MessageSizeType_to_string(type))
            .flags(Stats::nozero)
            ;
        m_msg_bytes[(unsigned int) type]
            .name(parent + csprintf(".throttle%i", m_node) + ".msg_bytes." +
                    MessageSizeType_to_string(type))
            .flags(Stats::nozero)
            ;

        m_msg_bytes[(unsigned int) type] = m_msg_counts[type] * Stats::constant(
                Network::MessageSizeType_to_int(type));
    }
}

void
Throttle::clearStats()
{
    m_link_utilization_proxy = 0;
}

void
Throttle::collateStats()
{
    m_link_utilization = 100.0 * m_link_utilization_proxy
        / (double(g_system_ptr->curCycle() - g_ruby_start));
}

void
Throttle::print(ostream& out) const
{
    ccprintf(out,  "[%i bw: %i]", m_node, getLinkBandwidth());
}

int
network_message_to_size(NetworkMessage* net_msg_ptr)
{
    assert(net_msg_ptr != NULL);

    int size = Network::MessageSizeType_to_int(net_msg_ptr->getMessageSize());
    size *=  MESSAGE_SIZE_MULTIPLIER;

    // Artificially increase the size of broadcast messages
    if (BROADCAST_SCALING > 1 && net_msg_ptr->getDestination().isBroadcast())
        size *= BROADCAST_SCALING;

    return size;
}
