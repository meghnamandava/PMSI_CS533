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

#include <algorithm>

#include "base/cast.hh"
#include "base/random.hh"
#include "debug/RubyNetwork.hh"
#include "mem/ruby/network/MessageBuffer.hh"
#include "mem/ruby/network/simple/PerfectSwitch.hh"
#include "mem/ruby/network/simple/SimpleNetwork.hh"
#include "mem/ruby/network/simple/Switch.hh"
#include "mem/ruby/slicc_interface/NetworkMessage.hh"

using namespace std;

const int PRIORITY_SWITCH_LIMIT = 128;

// Operator for helper class
bool
operator<(const LinkOrder& l1, const LinkOrder& l2)
{
    return (l1.m_value < l2.m_value);
}

PerfectSwitch::PerfectSwitch(SwitchID sid, Switch *sw, uint32_t virt_nets)
    : Consumer(sw)
{
    m_switch_id = sid;
    m_round_robin_start = 0;
    m_wakeups_wo_switch = 0;
    m_virtual_networks = virt_nets;
}

void
PerfectSwitch::init(SimpleNetwork *network_ptr)
{
    m_network_ptr = network_ptr;

    for(int i = 0;i < m_virtual_networks;++i) {
        m_pending_message_count.push_back(0);
    }
}

void
PerfectSwitch::addInPort(const vector<MessageBuffer*>& in)
{
    NodeID port = m_in.size();
    m_in.push_back(in);

    for (int i = 0; i < in.size(); ++i) {
        if (in[i] != nullptr) {
            in[i]->setConsumer(this);

            string desc =
                csprintf("[Queue from port %s %s %s to PerfectSwitch]",
                         to_string(m_switch_id), to_string(port),
                         to_string(i));

            in[i]->setDescription(desc);
            in[i]->setIncomingLink(port);
            in[i]->setVnet(i);
        }
    }
}

void
PerfectSwitch::addOutPort(const vector<MessageBuffer*>& out,
                          const NetDest& routing_table_entry)
{
    // Setup link order
    LinkOrder l;
    l.m_value = 0;
    l.m_link = m_out.size();
    m_link_order.push_back(l);

    // Add to routing table
    m_out.push_back(out);
    m_routing_table.push_back(routing_table_entry);
}

PerfectSwitch::~PerfectSwitch()
{
}

void
PerfectSwitch::operateVnet(int vnet)
{
    MsgPtr msg_ptr;
    NetworkMessage* net_msg_ptr = NULL;

    // This is for round-robin scheduling
    int incoming = m_round_robin_start;
		DPRINTF(RubyNetwork, "vnet %s \n", vnet);
		DPRINTF(RubyNetwork, "m_round_robin_start: %s \n", m_round_robin_start);
		DPRINTF(RubyNetwork, "m_in.size(): %s \n", m_in.size());

    m_round_robin_start++;
    if (m_round_robin_start >= m_in.size()) {
        m_round_robin_start = 0;
    }

		DPRINTF(RubyNetwork, "m_pending_message_count[vnet]: %s %s \n", vnet, m_pending_message_count[vnet]);
    if(m_pending_message_count[vnet] > 0) {
        // for all input ports, use round robin scheduling
        for (int counter = 0; counter < m_in.size(); counter++) {
            // Round robin scheduling
            incoming++;
            if (incoming >= m_in.size()) {
                incoming = 0;
            }

            // temporary vectors to store the routing results
            vector<LinkID> output_links;
            vector<NetDest> output_link_destinations;

						DPRINTF(RubyNetwork, "Size of m_in[incoming] %d %d %d", incoming, vnet, m_in[incoming].size());

            // Is there a message waiting?
            if (m_in[incoming].size() <= vnet) {
                continue;
            }
						
            MessageBuffer *buffer = m_in[incoming][vnet];
            if (buffer == nullptr) {
                continue;
            }

            while (buffer->isReady()) {
                DPRINTF(RubyNetwork, "incoming: %d\n", incoming);

                // Peek at message
                msg_ptr = buffer->peekMsgPtr();
                net_msg_ptr = safe_cast<NetworkMessage*>(msg_ptr.get());
                DPRINTF(RubyNetwork, "Message: %s\n", (*net_msg_ptr));

                output_links.clear();
                output_link_destinations.clear();
                NetDest msg_dsts = net_msg_ptr->getInternalDestination();

                // Unfortunately, the token-protocol sends some
                // zero-destination messages, so this assert isn't valid
                // assert(msg_dsts.count() > 0);
								//

								DPRINTF(RubyNetwork, "msg_dsts %s \n", msg_dsts);
								DPRINTF(RubyNetwork, "m_link_order.size(): %s\n", m_link_order.size());
                assert(m_link_order.size() == m_routing_table.size());
                assert(m_link_order.size() == m_out.size());

                if (m_network_ptr->getAdaptiveRouting()) {
                    if (m_network_ptr->isVNetOrdered(vnet)) {
                        // Don't adaptively route
												DPRINTF(RubyNetwork, "This network is ordered\n");
                        for (int out = 0; out < m_out.size(); out++) {
                            m_link_order[out].m_link = out;
                            m_link_order[out].m_value = 0;
                        }
                    } else {
                        // Find how clogged each link is
                        for (int out = 0; out < m_out.size(); out++) {
                            int out_queue_length = 0;
                            for (int v = 0; v < m_virtual_networks; v++) {
                                out_queue_length += m_out[out][v]->getSize();
                            }
                            int value =
                                (out_queue_length << 8) |
                                random_mt.random(0, 0xff);
                            m_link_order[out].m_link = out;
                            m_link_order[out].m_value = value;
                        }

                        // Look at the most empty link first
                        sort(m_link_order.begin(), m_link_order.end());
                    }
                }
								
                for (int i = 0; i < m_routing_table.size(); i++) {
                    // pick the next link to look at
                    int link = m_link_order[i].m_link;
                    NetDest dst = m_routing_table[link];

                    if (!msg_dsts.intersectionIsNotEmpty(dst))
                        continue;

                    // Remember what link we're using
                    DPRINTF(RubyNetwork, "link: %s dst: %s msg_dsts: %s\n", link, dst, msg_dsts);

                    output_links.push_back(link);

                    // Need to remember which destinations need this message in
                    // another vector.  This Set is the intersection of the
                    // routing_table entry and the current destination set.  The
                    // intersection must not be empty, since we are inside "if"
                    	
										output_link_destinations.push_back(msg_dsts.AND(dst));


                    // Next, we update the msg_destination not to include
                    // those nodes that were already handled by this link
										
										// ANI HACK
                    //msg_dsts.removeNetDest(dst);
                }



								// ANI HACK
                //assert(msg_dsts.count() == 0);

                // Check for resources - for all outgoing queues
                bool enough = true;
                for (int i = 0; i < output_links.size(); i++) {
                    int outgoing = output_links[i];

                    if (!m_out[outgoing][vnet]->areNSlotsAvailable(1))
                        enough = false;

                    DPRINTF(RubyNetwork, "Checking if node is blocked ..."
                            "outgoing: %d, vnet: %d, enough: %d\n",
                            outgoing, vnet, enough);
                }

                // There were not enough resources
                if (!enough) {
                    scheduleEvent(Cycles(1));
                    DPRINTF(RubyNetwork, "Can't deliver message since a node "
                            "is blocked\n");
                    DPRINTF(RubyNetwork, "Message: %s\n", (*net_msg_ptr));
                    break; // go to next incoming port
                }

                MsgPtr unmodified_msg_ptr;

                if (output_links.size() > 1) {
                    // If we are sending this message down more than one link
                    // (size>1), we need to make a copy of the message so each
                    // branch can have a different internal destination we need
                    // to create an unmodified MsgPtr because the MessageBuffer
                    // enqueue func will modify the message

                    // This magic line creates a private copy of the message
                    unmodified_msg_ptr = msg_ptr->clone();
                }

                // Dequeue msg
                buffer->dequeue();
                m_pending_message_count[vnet]--;
								
								
								// ANI HACK
								// The idea is to not allow self router operations.
								// So, for this check if incoming == outgoing for all router nodes excluding crossbar
								// For cross bar, iterate over all the nodes and send the msg_ptr
								DPRINTF(RubyNetwork, "M switch id: %d\n", m_switch_id);
         				for (int i=0; i<output_links.size(); i++) {
                    int outgoing = output_links[i];

                    if (i > 0) {
                        // create a private copy of the unmodified message
                        msg_ptr = unmodified_msg_ptr->clone();
                    }

                    // Change the internal destination set of the message so it
                    // knows which destinations this link is responsible for.
                    net_msg_ptr = safe_cast<NetworkMessage*>(msg_ptr.get());
                    net_msg_ptr->getInternalDestination() =
                        output_link_destinations[i];

										DPRINTF(RubyNetwork, "Last enqueued time: %d, delayed ticks: %d, getTime: %d\n", msg_ptr->getLastEnqueueTime(), msg_ptr->getDelayedTicks(), msg_ptr->getTime());

                    // Enqeue msg
										// The value in the comparison should reflect the number of cores + memory
										// 6 means, 4 cores with 1 shared L2 and memory
										//
if (m_switch_id < 8) {
											if (incoming != outgoing) {
                    		DPRINTF(RubyNetwork, "Enqueuing net msg from "
                    		        "inport[%d][%d] to outport [%d][%d].\n",
                    		        incoming, vnet, outgoing, vnet);
										
                    		m_out[outgoing][vnet]->enqueue(msg_ptr);
												// Ani hack
												//m_out[outgoing][vnet]->enqueue(msg_ptr, Cycles(0));
											}
										}
										else {
	               			DPRINTF(RubyNetwork, "Enqueuing net msg from "
                    		        "inport[%d][%d] to outport [%d][%d].\n",
                    		        incoming, vnet, outgoing, vnet);
										
											// Ani hack
                    	m_out[outgoing][vnet]->enqueue(msg_ptr);									
											//m_out[outgoing][vnet]->enqueue(msg_ptr, Cycles(0));
										}	
                }
	
								/*
                // Enqueue it - for all outgoing queues
                for (int i=0; i<output_links.size(); i++) {
                    int outgoing = output_links[i];

                    if (i > 0) {
                        // create a private copy of the unmodified message
                        msg_ptr = unmodified_msg_ptr->clone();
                    }

                    // Change the internal destination set of the message so it
                    // knows which destinations this link is responsible for.
                    net_msg_ptr = safe_cast<NetworkMessage*>(msg_ptr.get());
                    net_msg_ptr->getInternalDestination() =
                        output_link_destinations[i];

										DPRINTF(RubyNetwork, "Last enqueued time: %d, delayed ticks: %d, getTime: %d\n", msg_ptr->getLastEnqueueTime(), msg_ptr->getDelayedTicks(), msg_ptr->getTime());

                    // Enqeue msg
                    DPRINTF(RubyNetwork, "Enqueuing net msg from "
                            "inport[%d][%d] to outport [%d][%d].\n",
                            incoming, vnet, outgoing, vnet);

										
                    m_out[outgoing][vnet]->enqueue(msg_ptr);
                }
								*/
								
            }
        }
    }
}

void
PerfectSwitch::wakeup()
{
    // Give the highest numbered link priority most of the time
    m_wakeups_wo_switch++;
    int highest_prio_vnet = m_virtual_networks-1;
    int lowest_prio_vnet = 0;
    int decrementer = 1;

    // invert priorities to avoid starvation seen in the component network
    if (m_wakeups_wo_switch > PRIORITY_SWITCH_LIMIT) {
        m_wakeups_wo_switch = 0;
        highest_prio_vnet = 0;
        lowest_prio_vnet = m_virtual_networks-1;
        decrementer = -1;
    }

    // For all components incoming queues
    for (int vnet = highest_prio_vnet;
         (vnet * decrementer) >= (decrementer * lowest_prio_vnet);
         vnet -= decrementer) {
        operateVnet(vnet);
    }
}

void
PerfectSwitch::storeEventInfo(int info)
{
    m_pending_message_count[info]++;
}

void
PerfectSwitch::clearStats()
{
}
void
PerfectSwitch::collateStats()
{
}


void
PerfectSwitch::print(std::ostream& out) const
{
    out << "[PerfectSwitch " << m_switch_id << "]";
}
