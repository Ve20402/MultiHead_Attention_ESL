#ifndef SINGLE_HEAD_H
#define SINGLE_HEAD_H

#include <systemc.h>
#include <tlm.h>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/tlm_quantumkeeper.h>
#include <vector>
#include "datatypes.h"
#include "matrix_multiplier.h"

using namespace tlm;
using namespace tlm_utils;

SC_MODULE(SingleHeadModule) {
	
	tlm_utils::tlm_quantumkeeper qk;
	simple_initiator_socket<SingleHeadModule> socket;
	sc_event ev_start;
	sc_event ev_done;
	tlm_dmi dmi_cache;
	bool dmi_ptr_valid = false;
	
	uint64_t q_addr, k_addr, v_addr, out_addr;
    uint32_t rows, cols;
	
	MatrixMultiplier mat_mul;
	void trigger_start() { ev_start.notify(); };
	
	float mem_read(uint64_t addr);
	void mem_write(uint64_t addr, float val);
	void mem_read_block(uint64_t addr, float* data_ptr, uint32_t num_elements);
    void mem_write_block(uint64_t addr, float* data_ptr, uint32_t num_elements);
	void single_head_process();
	
	SC_HAS_PROCESS(SingleHeadModule);
	SingleHeadModule(sc_module_name name);	
};

#endif