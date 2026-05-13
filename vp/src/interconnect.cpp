#include "header/interconnect.h"
#include <iostream>

using namespace std;
using namespace tlm;

void Interconnect::b_transport(int id, tlm_generic_payload& trans, sc_time& delay) {

    uint64_t global_addr = trans.get_address();

    if (global_addr >= RAM_BASE && global_addr < RAM_BASE + RAM_SIZE) {
        trans.set_address(global_addr - RAM_BASE);
        ram_socket->b_transport(trans, delay);
        trans.set_address(global_addr);
        return;
    }

    if (global_addr >= BRAM_BASE && global_addr < BRAM_BASE + BRAM_SIZE) {
        trans.set_address(global_addr - BRAM_BASE);
        bram_socket->b_transport(trans, delay);
        trans.set_address(global_addr);
        return;
    }

    cerr << "[Interconnect] ERROR: unmapped address 0x"
         << hex << global_addr << dec << endl;
    trans.set_response_status(TLM_ADDRESS_ERROR_RESPONSE);
}

bool Interconnect::get_direct_mem_ptr(int id,
                                      tlm_generic_payload& trans,
                                      tlm_dmi& dmi_data)
{
    uint64_t global_addr = trans.get_address();

    if (global_addr >= RAM_BASE && global_addr < RAM_BASE + RAM_SIZE) {
        trans.set_address(global_addr - RAM_BASE);
        bool ok = ram_socket->get_direct_mem_ptr(trans, dmi_data);
        trans.set_address(global_addr);
        if (ok) {
            dmi_data.set_start_address(dmi_data.get_start_address() + RAM_BASE);
            dmi_data.set_end_address  (dmi_data.get_end_address()   + RAM_BASE);
        }
        return ok;
    }

    if (global_addr >= BRAM_BASE && global_addr < BRAM_BASE + BRAM_SIZE) {
        trans.set_address(global_addr - BRAM_BASE);
        bool ok = bram_socket->get_direct_mem_ptr(trans, dmi_data);
        trans.set_address(global_addr);
        if (ok) {
            dmi_data.set_start_address(dmi_data.get_start_address() + BRAM_BASE);
            dmi_data.set_end_address  (dmi_data.get_end_address()   + BRAM_BASE);
        }
        return ok;
    }

    return false;
}