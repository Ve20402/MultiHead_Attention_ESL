#ifndef HARDWARE_H
#define HARDWARE_H

#include <systemc.h>
#include <tlm.h>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/tlm_quantumkeeper.h>
#include <vector>
#include <math.h>
#include "datatypes.h"
#define NUM_HEADS 8

using namespace tlm;
using namespace tlm_utils;

SC_MODULE(MatrixMultiplier) {
	
	tlm_utils::tlm_quantumkeeper qk;
	simple_initiator_socket<MatrixMultiplier> socket;
	sc_event ev_start;
	sc_event ev_done;
	tlm_dmi dmi_cache;
	bool dmi_ptr_valid = false;
	uint64_t X_addr, W_addr, Y_addr;
	uint32_t rows, cols, w_rows;
    uint32_t stride_X, stride_W, stride_Y;
	bool transpose_W;
	
	void trigger_start() { ev_start.notify(); }
	
	void mem_read_block(uint64_t addr, float* data_ptr, uint32_t num_elements);
    void mem_write_block(uint64_t addr, float* data_ptr, uint32_t num_elements);
	
	float mem_read(uint64_t addr);
	void mem_write(uint64_t addr, float val);
	void multiply();
	
	SC_HAS_PROCESS(MatrixMultiplier);
	MatrixMultiplier(sc_module_name name);
};

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

SC_MODULE(MultiHeadModule) {
	
	tlm_utils::tlm_quantumkeeper qk;
	simple_initiator_socket<MultiHeadModule> socket;
	sc_event ev_done, ev_start;
	
	uint64_t reg_q_addr, reg_k_addr, reg_v_addr, reg_y_addr, reg_w_addr, reg_b_addr;
    bool reg_ctrl_start;
	const uint32_t ROWS_W = 1500;
    const uint32_t COLS_W = 512;
	const uint64_t HEAD_MEM_SPACE = 0xA00000; // 10 MB
	tlm_dmi dmi_cache;
	bool dmi_ptr_valid = false;
	
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