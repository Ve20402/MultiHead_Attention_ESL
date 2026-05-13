#ifndef INTERCONNECT_H
#define INTERCONNECT_H

#include <systemc.h>
#include <tlm.h>
#include <tlm_utils/multi_passthrough_target_socket.h>
#include <tlm_utils/simple_initiator_socket.h>

using namespace tlm;
using namespace tlm_utils;

static const uint64_t RAM_BASE  = 0x00000000ULL;
static const uint64_t RAM_SIZE  = 0x10000000ULL; 

static const uint64_t BRAM_BASE = 0x10000000ULL;
static const uint64_t BRAM_SIZE = 0x02000000ULL; 

static const uint64_t ADDR_Q     = RAM_BASE  + 0x00000000ULL;
static const uint64_t ADDR_K     = RAM_BASE  + 0x00A00000ULL;
static const uint64_t ADDR_V     = RAM_BASE  + 0x01400000ULL;
static const uint64_t ADDR_Y     = RAM_BASE  + 0x01E00000ULL;
static const uint64_t ADDR_W_OUT = BRAM_BASE + 0x00000000ULL;
static const uint64_t ADDR_B_OUT = BRAM_BASE + 0x00200000ULL;


SC_MODULE(Interconnect) {

    multi_passthrough_target_socket<Interconnect> target_socket;

    simple_initiator_socket<Interconnect> ram_socket;
    simple_initiator_socket<Interconnect> bram_socket;

    void b_transport(int id, tlm_generic_payload& trans, sc_time& delay);
	bool get_direct_mem_ptr(int id, tlm::tlm_generic_payload& trans, tlm::tlm_dmi& dmi_data);

    SC_CTOR(Interconnect)
        : target_socket("target_socket"),
          ram_socket("ram_socket"),
          bram_socket("bram_socket")
    {
        target_socket.register_b_transport(this, &Interconnect::b_transport);
		target_socket.register_get_direct_mem_ptr(this, &Interconnect::get_direct_mem_ptr);
    }
};

#endif