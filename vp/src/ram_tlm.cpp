#include "header/ram_tlm.h"
#include <iostream>
#include <cstring>

using namespace std;

RamTLM::RamTLM(sc_module_name name)
    : sc_module(name),
      socket("socket"),
      size(256 * 1024 * 1024) // 256 MB
{
    mem = new unsigned char[size];
    memset(mem, 0, size);
    socket.register_b_transport(this, &RamTLM::b_transport);
	socket.register_get_direct_mem_ptr(this, &RamTLM::get_direct_mem_ptr);
}

RamTLM::~RamTLM() {
    if (mem) delete[] mem;
}

void RamTLM::b_transport(int id, tlm_generic_payload& trans, sc_time& delay) {
    tlm_command   cmd  = trans.get_command();
    uint64_t      addr = trans.get_address(); 
    unsigned char* ptr = trans.get_data_ptr();
    unsigned int   len = trans.get_data_length();

    if (addr + len > size) {
        trans.set_response_status(TLM_ADDRESS_ERROR_RESPONSE);
        SC_REPORT_ERROR("RAM", "Address out of range!");
        return;
    }

    if (cmd == TLM_READ_COMMAND) {
        memcpy(ptr, &mem[addr], len);
        delay += sc_time(10, SC_NS);
    } else if (cmd == TLM_WRITE_COMMAND) {
        memcpy(&mem[addr], ptr, len);   //podatke na koje pokazuje ptr upisi u memoriju na adresu addr. podaci su duzine len
        delay += sc_time(10, SC_NS);
    }

    trans.set_response_status(TLM_OK_RESPONSE);
}

bool RamTLM::get_direct_mem_ptr(int id, tlm_generic_payload& trans, tlm_dmi& dmi_data) {
    dmi_data.allow_read_write();
    dmi_data.set_dmi_ptr(mem);
    dmi_data.set_start_address(0);
    dmi_data.set_end_address(size - 1);
    dmi_data.set_read_latency(sc_time(10, SC_NS));
    dmi_data.set_write_latency(sc_time(10, SC_NS));
    return true;
}


void RamTLM::direct_write(uint64_t addr, unsigned char* data, int len) {
    if (addr + len <= size) memcpy(&mem[addr], data, len);
}

void RamTLM::direct_read(uint64_t addr, unsigned char* data, int len) {
    if (addr + len <= size) memcpy(data, &mem[addr], len);
}