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

#include "base/intmath.hh"
#include "base/random.hh"
#include "cpu/testers/rubytest/Check.hh"
#include "cpu/testers/rubytest/CheckTable.hh"
#include "debug/RubyTest.hh"

#include <iostream>
#include <string>
#include <fstream>

CheckTable::CheckTable(int _num_writers, int _num_readers, RubyTester* _tester)
    : m_num_writers(_num_writers), m_num_readers(_num_readers),
      m_tester_ptr(_tester)
{
	if (TRACE == 0) {
    physical_address_t physical = 0;
    Address address;

    const int size1 = 32;
    const int size2 = 100;

    // The first set is to get some false sharing
    physical = 1000;
    for (int i = 0; i < size1; i++) {
        // Setup linear addresses
        address.setAddress(physical);
        addCheck(address);
        physical += CHECK_SIZE;
    }

    // The next two sets are to get some limited false sharing and
    // cache conflicts
    physical = 1000;
    for (int i = 0; i < size2; i++) {
        // Setup linear addresses
        address.setAddress(physical);
        addCheck(address);
        physical += 256;
    }

    physical = 1000 + CHECK_SIZE;
    for (int i = 0; i < size2; i++) {
        // Setup linear addresses
        address.setAddress(physical);
        addCheck(address);
        physical += 256;
    }
	}
	else {
		// Read from a file say trace file
		std::string input_trace = "trace.trc";
		std::string fline;
		physical_address_t physical = 0;
		Address address;

		/* trace based simulation */
		for (unsigned int i = 0; i<NPROC; i++) {
			vindex[i] = 0;
		}

		for (unsigned int i = 0; i<NPROC; i++) {
			prevVindex[i] = -1;
		}

		std::ifstream myFile(input_trace.c_str());	

		if (myFile.is_open()) {
			while(getline (myFile, fline)) {
				size_t pos = fline.find(" ");
				std::string s = fline.substr(0, pos);
				std::string type = fline.substr(pos+1, 2);
				std::string cyc = fline.substr(pos+3);

				//DPRINTF(RubyTest, "Type: %s\n", type);
				
				if (type == "WR") {
					c0_op.push_back(false);
					c1_op.push_back(false);
					c2_op.push_back(false);
					c3_op.push_back(false);
					c4_op.push_back(false);
					c5_op.push_back(false);
					c6_op.push_back(false);
					c7_op.push_back(false);
					
				}
				else {
					c0_op.push_back(true);
					c1_op.push_back(true);
					c2_op.push_back(true);
					c3_op.push_back(true);	
					c4_op.push_back(true);
					c5_op.push_back(true);
					c6_op.push_back(true);
					c7_op.push_back(true);					
				}

				unsigned long memSchedule = (unsigned long)strtol(cyc.c_str(), NULL, 10);
				//DPRINTF(RubyTest, "Memory schedule :%s\n", memSchedule);
				core0OrigMemSchedule.push_back(memSchedule);
				core1OrigMemSchedule.push_back(memSchedule);
				core2OrigMemSchedule.push_back(memSchedule);
				core3OrigMemSchedule.push_back(memSchedule);
				core4OrigMemSchedule.push_back(memSchedule);
				core5OrigMemSchedule.push_back(memSchedule);
				core6OrigMemSchedule.push_back(memSchedule);
				core7OrigMemSchedule.push_back(memSchedule);
	
				physical = (unsigned long)strtol(s.c_str(), NULL, 16);
				physical = physical & 0x8FFFFFFF;
				//std::cout<<"\n Physical address: "<<physical;
				address.setAddress(physical);
				//DPRINTF(RubyTest, "Address: %s\n", address);
				addCheck(address);
			}
		}
		core0MemSchedule = core0OrigMemSchedule;
		core1MemSchedule = core1OrigMemSchedule;
		core2MemSchedule = core2OrigMemSchedule;
		core3MemSchedule = core3OrigMemSchedule;
		core4MemSchedule = core4OrigMemSchedule;
		core5MemSchedule = core5OrigMemSchedule;
		core6MemSchedule = core6OrigMemSchedule;
		core7MemSchedule = core7OrigMemSchedule;
	}
}

CheckTable::~CheckTable()
{
    int size = m_check_vector.size();
    for (int i = 0; i < size; i++)
        delete m_check_vector[i];
}

void
CheckTable::addCheck(const Address& address)
{
		Check* check_ptr = NULL;
		if (TRACE) {
			std::vector<Check*> cvec;
			// Bound should be maximum number of NPROC
			for (unsigned int i = 0; i<MAX_NPROC; i++) {
				check_ptr = new Check(address, Address(100 + m_check_vector.size()),
																		m_num_writers, m_num_readers, m_tester_ptr);
				m_lookup_map[Address(address.getAddress())] = check_ptr;
				cvec.push_back(check_ptr);				
			}
			m_vec_lookup_map[Address(address.getAddress())] = cvec;
			m_c0_check_vector.push_back(cvec.at(0));
			m_c1_check_vector.push_back(cvec.at(1));
			m_c2_check_vector.push_back(cvec.at(2));
			m_c3_check_vector.push_back(cvec.at(3));
			m_c4_check_vector.push_back(cvec.at(4));
			m_c5_check_vector.push_back(cvec.at(5));
			m_c6_check_vector.push_back(cvec.at(6));
			m_c7_check_vector.push_back(cvec.at(7));

		}
		else {
    	if (floorLog2(CHECK_SIZE) != 0) {
   	    if (address.bitSelect(0, CHECK_SIZE_BITS - 1) != 0) {
           panic("Check not aligned");
        }
    	}

    	for (int i = 0; i < CHECK_SIZE; i++) {
   	    if (m_lookup_map.count(Address(address.getAddress()+i))) {
            // A mapping for this byte already existed, discard the
            // entire check
            return;
        }
    	}

    	check_ptr = new Check(address, Address(100 + m_check_vector.size()),
                                 m_num_writers, m_num_readers, m_tester_ptr);
    	for (int i = 0; i < CHECK_SIZE; i++) {
        // Insert it once per byte
        m_lookup_map[Address(address.getAddress() + i)] = check_ptr;
    	}
    	m_check_vector.push_back(check_ptr);
		}

}

Check*
CheckTable::getRandomCheck()
{
    assert(m_check_vector.size() > 0);
    return m_check_vector[random_mt.random<unsigned>(0, m_check_vector.size() - 1)];
}

Check*
CheckTable::getOrderedCheck(unsigned int coreID) {


		std::vector<Check*> check_vector;
		switch(coreID) {
			case 0:
				check_vector = m_c0_check_vector;
				break;
			case 1:
				check_vector = m_c1_check_vector;
				break;
			case 2:
				check_vector = m_c2_check_vector;
				break;
			case 3:
				check_vector = m_c3_check_vector;
				break;
			case 4:
				check_vector = m_c4_check_vector;
				break;
			case 5:
				check_vector = m_c5_check_vector;
				break;
			case 6:
				check_vector = m_c6_check_vector;
				break;
			case 7:
				check_vector = m_c7_check_vector;
				break;
			default:
				return NULL;
		}
		DPRINTF(RubyTest, "Ordered check index: %s\n", vindex[coreID]);
		return check_vector.at(vindex[coreID]);	

}

void 
CheckTable::changeNextArrivalTime(NodeID proc, uint64_t currentCycle) {
	// First get the difference in arrival time between the current and next ones
	if (proc == 0) {
		if (vindex[proc]+1 < core0MemSchedule.size()) {
			uint64_t arrivalDiff = core0OrigMemSchedule.at(vindex[proc]+1) - core0OrigMemSchedule.at(vindex[proc]);
			
			DPRINTF(RubyTest, "arrivalDiff: %s, currentCycle: %s\n", arrivalDiff, currentCycle);
			DPRINTF(RubyTest, "Changing arrival time from :%s", core0OrigMemSchedule.at(vindex[proc]+1));
			core0MemSchedule.at(vindex[proc]+1) = arrivalDiff + currentCycle;
			DPRINTF(RubyTest, "To :%s\n", core0MemSchedule.at(vindex[proc]+1));
		}
	}
	else if (proc == 1) {
		if (vindex[proc]+1 < core1MemSchedule.size()) {
			uint64_t arrivalDiff = core1OrigMemSchedule.at(vindex[proc]+1) - core1OrigMemSchedule.at(vindex[proc]);
			DPRINTF(RubyTest, "Changing arrival time from :%s", core1OrigMemSchedule.at(vindex[proc]+1));
			core1MemSchedule.at(vindex[proc]+1) = arrivalDiff + currentCycle;
			DPRINTF(RubyTest, "To :%s\n", core1MemSchedule.at(vindex[proc]+1));
		}
	}
	else if (proc == 2) {
		if (vindex[proc]+1 < core2MemSchedule.size()) {
			uint64_t arrivalDiff = core2OrigMemSchedule.at(vindex[proc]+1) - core2OrigMemSchedule.at(vindex[proc]);
			DPRINTF(RubyTest, "Changing arrival time from :%s", core2OrigMemSchedule.at(vindex[proc]+1));
			core2MemSchedule.at(vindex[proc]+1) = arrivalDiff + currentCycle;
			DPRINTF(RubyTest, "To :%s\n", core2MemSchedule.at(vindex[proc]+1));
		}
	}
	else if (proc == 3) {
		if (vindex[proc]+1 < core3MemSchedule.size()) {
			uint64_t arrivalDiff = core3OrigMemSchedule.at(vindex[proc]+1) - core3OrigMemSchedule.at(vindex[proc]);
			DPRINTF(RubyTest, "Changing arrival time from :%s", core3OrigMemSchedule.at(vindex[proc]+1));
			core3MemSchedule.at(vindex[proc]+1) = arrivalDiff + currentCycle;
			DPRINTF(RubyTest, "To :%s\n", core3MemSchedule.at(vindex[proc]+1));
		}
	}

	else if (proc == 4) {
		if (vindex[proc]+1 < core4MemSchedule.size()) {
			uint64_t arrivalDiff = core4OrigMemSchedule.at(vindex[proc]+1) - core4OrigMemSchedule.at(vindex[proc]);
			DPRINTF(RubyTest, "Changing arrival time from :%s", core4OrigMemSchedule.at(vindex[proc]+1));
			core4MemSchedule.at(vindex[proc]+1) = arrivalDiff + currentCycle;
			DPRINTF(RubyTest, "To :%s\n", core4MemSchedule.at(vindex[proc]+1));
		}
	}

	else if (proc == 5) {
		if (vindex[proc]+1 < core5MemSchedule.size()) {
			uint64_t arrivalDiff = core5OrigMemSchedule.at(vindex[proc]+1) - core5OrigMemSchedule.at(vindex[proc]);
			DPRINTF(RubyTest, "Changing arrival time from :%s", core5OrigMemSchedule.at(vindex[proc]+1));
			core5MemSchedule.at(vindex[proc]+1) = arrivalDiff + currentCycle;
			DPRINTF(RubyTest, "To :%s\n", core5MemSchedule.at(vindex[proc]+1));
		}
	}

	else if (proc == 6) {
		if (vindex[proc]+1 < core6MemSchedule.size()) {
			uint64_t arrivalDiff = core6OrigMemSchedule.at(vindex[proc]+1) - core6OrigMemSchedule.at(vindex[proc]);
			DPRINTF(RubyTest, "Changing arrival time from :%s", core6OrigMemSchedule.at(vindex[proc]+1));
			core6MemSchedule.at(vindex[proc]+1) = arrivalDiff + currentCycle;
			DPRINTF(RubyTest, "To :%s\n", core6MemSchedule.at(vindex[proc]+1));
		}
	}

	else if (proc == 7) {
		if (vindex[proc]+1 < core7MemSchedule.size()) {
			uint64_t arrivalDiff = core7OrigMemSchedule.at(vindex[proc]+1) - core7OrigMemSchedule.at(vindex[proc]);
			DPRINTF(RubyTest, "Changing arrival time from :%s", core7OrigMemSchedule.at(vindex[proc]+1));
			core7MemSchedule.at(vindex[proc]+1) = arrivalDiff + currentCycle;
			DPRINTF(RubyTest, "To :%s\n", core7MemSchedule.at(vindex[proc]+1));
		}
	}

}

bool
CheckTable::getOp(unsigned int coreID) {
	if (coreID == 0) {
		return c0_op.at(vindex[coreID]);	
	}
	else if (coreID == 1) {
		return c1_op.at(vindex[coreID]);	
	}
	else if (coreID == 2) {
		return c2_op.at(vindex[coreID]);	
	}
	else if (coreID == 3) {
		return c3_op.at(vindex[coreID]);	
	}

	else if (coreID == 4) {
		return c4_op.at(vindex[coreID]);	
	}
	else if (coreID == 5) {
		return c5_op.at(vindex[coreID]);
	}
	else if (coreID == 6) {
		return c6_op.at(vindex[coreID]);
	}
	else if (coreID == 7) {
		return c7_op.at(vindex[coreID]);
	}

	return false;
}

unsigned long 
CheckTable::getNextTS(unsigned int coreID) {
	if (coreID == 0) {
		return core0MemSchedule.at(vindex[coreID]);
	}
	else if (coreID == 1) {
		return core1MemSchedule.at(vindex[coreID]);
	}
	else if (coreID == 2) {
		return core2MemSchedule.at(vindex[coreID]);
	}
	else if (coreID == 3) {
		return core3MemSchedule.at(vindex[coreID]);
	}

	else if (coreID == 4) {
		return core4MemSchedule.at(vindex[coreID]);
	}
	else if (coreID == 5) {
		return core5MemSchedule.at(vindex[coreID]);
	}
	else if (coreID == 6) {
		return core6MemSchedule.at(vindex[coreID]);
	}
	else if (coreID == 7) {
		return core7MemSchedule.at(vindex[coreID]);
	}

	return 0;
}

Check*
CheckTable::getCheck(const Address& address, NodeID proc)
{
    DPRINTF(RubyTest, "Looking for check by address: %s", address);

    m5::hash_map<Address, std::vector<Check*> >::iterator i = m_vec_lookup_map.find(address);

    if (i == m_vec_lookup_map.end())
        return NULL;

		std::vector<Check*> checkVec = i->second;
		assert(checkVec.size() != 0);
		return checkVec.at(proc);		
}

Check*
CheckTable::getCheck(const Address& address)
{
    DPRINTF(RubyTest, "Looking for check by address: %s", address);

    m5::hash_map<Address, Check*>::iterator i = m_lookup_map.find(address);

    if (i == m_lookup_map.end())
        return NULL;

    Check* check = i->second;
    assert(check != NULL);
    return check;
}

void
CheckTable::print(std::ostream& out) const
{
} 
