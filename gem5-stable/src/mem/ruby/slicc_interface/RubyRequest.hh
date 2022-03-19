/*
 * Copyright (c) 2009 Mark D. Hill and David A. Wood
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

#ifndef __MEM_RUBY_SLICC_INTERFACE_RUBY_REQUEST_HH__
#define __MEM_RUBY_SLICC_INTERFACE_RUBY_REQUEST_HH__

#include <ostream>
#include <cmath>

#include "mem/protocol/Message.hh"
#include "mem/protocol/PrefetchBit.hh"
#include "mem/protocol/RubyAccessMode.hh"
#include "mem/protocol/RubyRequestType.hh"
#include "mem/ruby/common/Address.hh"

class RubyRequest : public Message
{
  public:
    Address m_PhysicalAddress;
    Address m_LineAddress;
    RubyRequestType m_Type;
		RubyRequestType m_PType;
    Address m_ProgramCounter;
    RubyAccessMode m_AccessMode;
    int m_Size;
    PrefetchBit m_Prefetch;
    uint8_t* m_Data;
    uint64_t m_Data64Bit;
    PacketPtr pkt;
    unsigned m_contextId;
    //niv cache_approximate
    uint64 m_Offset;	

    RubyRequest(Tick curTime, uint64_t _paddr, uint8_t* _m_Data, int _len,
        uint64_t _pc, RubyRequestType _ptype, RubyRequestType _type, RubyAccessMode _access_mode,
        PacketPtr _pkt, PrefetchBit _pb = PrefetchBit_No,
        unsigned _proc_id = 100)
        : Message(curTime),
          m_PhysicalAddress(_paddr),
          m_Type(_type),
					m_PType(_ptype),
          m_ProgramCounter(_pc),
          m_AccessMode(_access_mode),
          m_Size(_len),
          m_Prefetch(_pb),
          m_Data(_m_Data),
          pkt(_pkt),
          m_contextId(_proc_id)					
    {
      m_LineAddress = m_PhysicalAddress;
      m_LineAddress.makeLineAddress();
      
      //cache_approximate
			 if (m_Data) {
       	uint64_t *tmp = (uint64_t *) m_Data;
       	m_Data64Bit = *tmp;
	//	m_Data64Bit = m_Data64Bit & ((2^(m_Size*8)) - 1);
			if (m_Size < 8) {
       		uint64_t mask = pow(2, m_Size*8) - 1;
					m_Data64Bit = m_Data64Bit & mask;
				}
			 }
			// Data64Bit is updated now by taking the first byte pointed by the Data pointer and typecasting it to 64bits. This is done because Ruby does load and store for 1 byte only. In other cases, should try to use the m_Size to get those many number of bytes and pad the remaining with zeroes. Need to do this -- done
       //uint64_t tmp = (uint64_t) *m_Data;
       //m_Data64Bit = tmp;
      //niv
      //m_Data64Bit =0;
			// Ani: Commenting this out for now
			//std::cout<<"\n m_size: "<<m_Size<<" m_Data: "<<std::hex<<(void*)m_Data<<std::dec;
			/*
			if (m_Data) {
				//std::cout<<"\n m_size: "<<m_Size<<" m_Data: "<<std::hex<<(void*)m_Data<<std::dec;
				//fflush(stdout);
      	//memcpy(&m_Data64Bit,m_Data,m_Size);
      	memcpy((void*)&m_Data64Bit,(void*)m_Data,m_Size);
			}
			*/
      m_Offset = m_PhysicalAddress.getOffset();
    }

    RubyRequest(Tick curTime) : Message(curTime) {}
    MsgPtr clone() const
    { return std::shared_ptr<Message>(new RubyRequest(*this)); }

    const Address& getLineAddress() const { return m_LineAddress; }
    const Address& getPhysicalAddress() const { return m_PhysicalAddress; }
    const RubyRequestType& getType() const { return m_Type; }
    const Address& getProgramCounter() const { return m_ProgramCounter; }
    const RubyAccessMode& getAccessMode() const { return m_AccessMode; }
    const int& getSize() const { return m_Size; }
    const PrefetchBit& getPrefetch() const { return m_Prefetch; }

    void print(std::ostream& out) const;
    bool functionalRead(Packet *pkt);
    bool functionalWrite(Packet *pkt);

		bool bypassBlocked() const { return (m_Type == RubyRequestType_Locked_RMW_Write);}

};

inline std::ostream&
operator<<(std::ostream& out, const RubyRequest& obj)
{
  obj.print(out);
  out << std::flush;
  return out;
}

#endif
