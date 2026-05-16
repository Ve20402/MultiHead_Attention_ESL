#include "header/vp.h"
#include <iostream>

using namespace sc_core;
using namespace tlm;
using namespace std;

vp::vp(sc_module_name name)
    : sc_module(name),
      s_cpu("s_cpu"),
      s_bus("s_bus"),
      bus("bus"),
      ram("ram"),
      bram("bram"),
      multi_head("multi_head")
{
    s_cpu.register_b_transport(this, &vp::b_transport);

    s_bus.bind(bus.target_socket);

    bus.ram_socket.bind(ram.socket);
    bus.bram_socket.bind(bram.socket);

    multi_head.socket.bind(bus.target_socket);

    for (int i = 0; i < NUM_HEADS; i++) {
        multi_head.heads[i].socket.bind(bus.target_socket);
        multi_head.heads[i].mat_mul.socket.bind(bus.target_socket);
    }

    multi_head.reg_q_addr   = ADDR_Q;
    multi_head.reg_k_addr   = ADDR_K;
    multi_head.reg_v_addr   = ADDR_V;
    multi_head.reg_w_addr   = ADDR_W_OUT;
    multi_head.reg_b_addr   = ADDR_B_OUT;
    multi_head.reg_y_addr   = ADDR_Y;

    SC_REPORT_INFO("VP", "Virtual Platform initialized");
}

void vp::b_transport(tlm_generic_payload& pl, sc_time& offset) {
    uint64_t addr = pl.get_address();

    if (addr == ADDR_CTRL && pl.get_command() == TLM_WRITE_COMMAND) {
        unsigned char cmd = *pl.get_data_ptr();
        if (cmd & 0x01) {
            multi_head.done_flag = false; 
            multi_head.trigger_start();
        }
        pl.set_response_status(TLM_OK_RESPONSE);
        offset += sc_time(10, SC_NS);  
        return;
    }

    if (addr == ADDR_STATUS && pl.get_command() == TLM_READ_COMMAND) {
        *pl.get_data_ptr() = multi_head.done_flag ? 1u : 0u;
        pl.set_response_status(TLM_OK_RESPONSE);
        offset += sc_time(10, SC_NS);   
        return;
    }

    s_bus->b_transport(pl, offset);
}