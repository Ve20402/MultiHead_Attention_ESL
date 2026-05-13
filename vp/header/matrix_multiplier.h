#ifndef MATRIX_MULTIPLIER_H
#define MATRIX_MULTIPLIER_H

#include <systemc.h>
#include <tlm.h>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/tlm_quantumkeeper.h>
#include <vector>
#include "datatypes.h"

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
	
	void trigger_start() {
         ev_start.notify();
    }
    
	void mem_read_block(uint64_t addr, float* data_ptr, uint32_t num_elements);
    void mem_write_block(uint64_t addr, float* data_ptr, uint32_t num_elements);
	
	float mem_read(uint64_t addr);
	void mem_write(uint64_t addr, float val);
	void multiply();
	
	SC_HAS_PROCESS(MatrixMultiplier);
	MatrixMultiplier(sc_module_name name);
};

#endif