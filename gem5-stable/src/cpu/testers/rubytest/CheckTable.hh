/*
 * Copyright (c) 1999-2008 Mark D. Hill and David A. Wood
 * Copyright (c) 2009 Advanced Micro Devices, Inc.
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

#ifndef __CPU_RUBYTEST_CHECKTABLE_HH__
#define __CPU_RUBYTEST_CHECKTABLE_HH__

#include <iostream>
#include <vector>

#include "base/hashmap.hh"
#include "mem/ruby/common/Address.hh"
#include "mem/ruby/common/Global.hh"
#include "cpu/testers/rubytest/Trace.hh"

class Check;
class RubyTester;

class CheckTable
{
  public:
    CheckTable(int _num_writers, int _num_readers, RubyTester* _tester);
    ~CheckTable();

    Check* getRandomCheck();
    Check* getCheck(const Address& address);
    Check* getCheck(const Address& address, NodeID proc);

		/* trace based simulation */
		Check* getOrderedCheck(unsigned int coreID);
		void updatevindex(unsigned int coreID) { vindex[coreID]++;}
		unsigned long getNextTS(unsigned int coreID);
		bool prevReqComplete(unsigned int coreID) {
			if (prevVindex[coreID] == vindex[coreID]) {
				return false;
			}
			return true;
		}

		void updatePrevindex(unsigned int coreID) { prevVindex[coreID] = vindex[coreID];}
		void changeNextArrivalTime(NodeID proc, uint64_t currentCycle);
		bool getOp(unsigned int coreID);
		bool isEnd() { if (vindex[0] == m_c0_check_vector.size()-1) {std::cout<<"\n DONE: "<<m_c0_check_vector.size(); return true;} return false; }

    //  bool isPresent(const Address& address) const;
    //  void removeCheckFromTable(const Address& address);
    //  bool isTableFull() const;
    // Need a method to select a check or retrieve a check

    void print(std::ostream& out) const;

  private:
    void addCheck(const Address& address);

    // Private copy constructor and assignment operator
    CheckTable(const CheckTable& obj);
    CheckTable& operator=(const CheckTable& obj);

    std::vector<Check*> m_check_vector;		
    m5::hash_map<Address, Check*> m_lookup_map;
		m5::hash_map<Address, std::vector<Check*> >m_vec_lookup_map;

    int m_num_writers;
    int m_num_readers;
    RubyTester* m_tester_ptr;

		/* trace based simulation */
		uint64_t vindex[8];
		int64_t prevVindex[8];

		std::vector<unsigned int> core0MemSchedule;
		std::vector<unsigned int> core1MemSchedule;
		std::vector<unsigned int> core2MemSchedule;
		std::vector<unsigned int> core3MemSchedule;
		std::vector<unsigned int> core4MemSchedule;
		std::vector<unsigned int> core5MemSchedule;
		std::vector<unsigned int> core6MemSchedule;
		std::vector<unsigned int> core7MemSchedule;

		std::vector<unsigned int> core0OrigMemSchedule;
		std::vector<unsigned int> core1OrigMemSchedule;
		std::vector<unsigned int> core2OrigMemSchedule;
		std::vector<unsigned int> core3OrigMemSchedule;
		std::vector<unsigned int> core4OrigMemSchedule;
		std::vector<unsigned int> core5OrigMemSchedule;
		std::vector<unsigned int> core6OrigMemSchedule;
		std::vector<unsigned int> core7OrigMemSchedule;


		std::vector<Check*> m_c0_check_vector;
		std::vector<Check*> m_c1_check_vector;
		std::vector<Check*> m_c2_check_vector;
		std::vector<Check*> m_c3_check_vector;
		std::vector<Check*> m_c4_check_vector;
		std::vector<Check*> m_c5_check_vector;
		std::vector<Check*> m_c6_check_vector;
		std::vector<Check*> m_c7_check_vector;

		std::vector<bool> c0_op;
		std::vector<bool> c1_op;
		std::vector<bool> c2_op;
		std::vector<bool> c3_op;
		std::vector<bool> c4_op;
		std::vector<bool> c5_op;
		std::vector<bool> c6_op;
		std::vector<bool> c7_op;
};

inline std::ostream&
operator<<(std::ostream& out, const CheckTable& obj)
{
    obj.print(out);
    out << std::flush;
    return out;
}

#endif // __CPU_RUBYTEST_CHECKTABLE_HH__
