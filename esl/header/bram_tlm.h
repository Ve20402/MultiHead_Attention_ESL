#ifndef BRAM_TLM_H
#define BRAM_TLM_H

#include <systemc.h>
#include <tlm.h>
#include <tlm_utils/multi_passthrough_target_socket.h>

using namespace tlm;
using namespace tlm_utils;

SC_MODULE(BramTLM) {
    multi_passthrough_target_socket<BramTLM> socket;

    unsigned char* mem;
    uint64_t size;

    void b_transport(int id, tlm_generic_payload& trans, sc_time& delay);

    void direct_write(uint64_t local_addr, unsigned char* data, int len);
	bool get_direct_mem_ptr(int id, tlm_generic_payload& trans, tlm_dmi& dmi_data);
    void direct_read (uint64_t local_addr, unsigned char* data, int len);

    BramTLM(sc_module_name name, uint64_t size_bytes = 32 * 1024 * 1024);

    ~BramTLM();
};

#endif