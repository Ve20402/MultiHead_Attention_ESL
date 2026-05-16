#ifndef MULTI_HEAD_H
#define MULTI_HEAD_H

#include <systemc.h>
#include <tlm.h>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/tlm_quantumkeeper.h>
#include <vector>
#include <math.h>
#include "single_head.h"
#define NUM_HEADS 8

using namespace tlm;
using namespace tlm_utils;

SC_MODULE(MultiHeadModule) {
	
	tlm_utils::tlm_quantumkeeper qk;
	simple_initiator_socket<MultiHeadModule> socket;
	sc_event ev_done, ev_start;
	
	uint64_t reg_q_addr, reg_k_addr, reg_v_addr, reg_y_addr, reg_w_addr, reg_b_addr;
    bool reg_ctrl_start;
	const uint32_t ROWS_W = 1500;
    const uint32_t COLS_W = 512;
	const uint64_t HEAD_MEM_SPACE = 0xA00000;
	tlm_dmi dmi_cache;
	bool dmi_ptr_valid = false;
	volatile bool done_flag = false;
	
	sc_vector<SingleHeadModule> heads;
	void trigger_start() { ev_start.notify(); };
	
	void mem_read_block(uint64_t addr, float* data_ptr, uint32_t num_elements);
    void mem_write_block(uint64_t addr, float* data_ptr, uint32_t num_elements);
	float mem_read(uint64_t addr);
	void mem_write(uint64_t addr, float val);
	void multi_head_process();
	
	SC_HAS_PROCESS(MultiHeadModule);
    MultiHeadModule(sc_module_name name);	
};


#endif