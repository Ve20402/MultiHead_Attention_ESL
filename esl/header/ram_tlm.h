#ifndef RAM_TLM_H
#define RAM_TLM_H

#include <systemc.h>
#include <tlm.h>
#include <tlm_utils/multi_passthrough_target_socket.h>

using namespace tlm;
using namespace tlm_utils;

SC_MODULE(RamTLM) {
	multi_passthrough_target_socket<RamTLM> socket;
	
	unsigned char* mem;
    uint64_t size;
	
	void b_transport(int id, tlm_generic_payload& trans, sc_time& delay);
	void direct_write(uint64_t addr, unsigned char* data, int len);
	void direct_read(uint64_t addr, unsigned char* data, int len);
	bool get_direct_mem_ptr(int id, tlm_generic_payload& trans, tlm_dmi& dmi_data);
	
	RamTLM(sc_module_name name);
	~RamTLM();
};

#endif