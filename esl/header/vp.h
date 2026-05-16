#ifndef VP_H
#define VP_H

#include <systemc>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>

#include "datatypes.h"
#include "ram_tlm.h"
#include "bram_tlm.h"
#include "multi_head.h"   
#include "interconnect.h" 

static const uint64_t ADDR_CTRL   = 0x40000000ULL;
static const uint64_t ADDR_STATUS = 0x40000004ULL;

class vp : public sc_core::sc_module {
public:
    tlm_utils::simple_target_socket<vp> s_cpu;

    vp(sc_core::sc_module_name name);

private:
    Interconnect    bus;
    RamTLM          ram;
    BramTLM         bram;
    MultiHeadModule multi_head;

    tlm_utils::simple_initiator_socket<vp> s_bus;

    void b_transport(tlm::tlm_generic_payload& pl, sc_core::sc_time& offset);
};

#endif