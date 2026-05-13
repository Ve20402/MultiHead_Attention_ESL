#include <systemc.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>
#include <tlm_utils/tlm_quantumkeeper.h>

#include "header/datatypes.h"
#include "header/ram_tlm.h"
#include "header/bram_tlm.h"
#include "header/interconnect.h" 
#include "header/matrix_multiplier.h"
#include "header/single_head.h"
#include "header/multi_head.h"


SC_MODULE(SimulationControl) {
    MultiHeadModule* uut_ptr;

    void run_test() {
        wait(SC_ZERO_TIME);

        std::cout << "[TB] Triggering MultiHead attention..." << std::endl;
        uut_ptr->trigger_start();

        wait(uut_ptr->ev_done);

        std::cout << "[TB] Hardware completed." << std::endl;
        sc_stop();
    }

    SC_CTOR(SimulationControl) {
        SC_THREAD(run_test);
    }
};

template<typename MEM>
static void load_matrix(const std::string& filename,
                        uint64_t           global_addr,
                        uint64_t           base,
                        MEM&               mem_obj)
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "[TB] ERROR: cannot open " << filename << std::endl;
        return;
    }
    uint64_t local = global_addr - base;
    float val;
    int count = 0;
    while (file >> val) {
        mem_obj.direct_write(local, reinterpret_cast<unsigned char*>(&val), 4);
        local += 4;
        ++count;
    }
    std::cout << "[TB] Loaded " << count << " floats from " << filename
              << "  @ global 0x" << std::hex << global_addr << std::dec << std::endl;
}

static void save_matrix(const std::string& filename,
                        uint64_t           global_addr,
                        uint64_t           base,
                        int rows, int cols,
                        RamTLM&            ram)
{
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "[TB] ERROR: cannot open " << filename << " for writing" << std::endl;
        return;
    }
    uint64_t local = global_addr - base;
    for (int i = 0; i < rows * cols; ++i) {
        float val;
        ram.direct_read(local + i * 4, reinterpret_cast<unsigned char*>(&val), 4);
        file << std::fixed << std::setprecision(6) << val << "\n";
    }
    std::cout << "[TB] Saved " << rows * cols << " floats to " << filename << std::endl;
}

int sc_main(int argc, char* argv[]) {

    tlm::tlm_global_quantum::instance().set(sc_time(10, SC_NS));

    RamTLM       ram  ("TLM_RAM");
    BramTLM      bram ("TLM_BRAM");  
    Interconnect bus  ("Interconnect");

    std::cout << "\n[TB] Loading matrices..." << std::endl;
    load_matrix("../data/matrice/multihead_ulaz_Q.txt",   ADDR_Q,     RAM_BASE,  ram);
    load_matrix("../data/matrice/multihead_ulaz_K.txt",   ADDR_K,     RAM_BASE,  ram);
    load_matrix("../data/matrice/multihead_ulaz_V.txt",   ADDR_V,     RAM_BASE,  ram);
    load_matrix("../data/matrice/multihead_W_out.txt",    ADDR_W_OUT, BRAM_BASE, bram);
    load_matrix("../data/matrice/multihead_b_out.txt",    ADDR_B_OUT, BRAM_BASE, bram);

    MultiHeadModule uut("MultiHead");

    uut.reg_q_addr = ADDR_Q;
    uut.reg_k_addr = ADDR_K;
    uut.reg_v_addr = ADDR_V;
    uut.reg_w_addr = ADDR_W_OUT;
    uut.reg_b_addr = ADDR_B_OUT;
    uut.reg_y_addr = ADDR_Y;

    uut.socket.bind(bus.target_socket);

    for (int i = 0; i < NUM_HEADS; i++) {
        uut.heads[i].socket.bind(bus.target_socket);
        uut.heads[i].mat_mul.socket.bind(bus.target_socket);
    }

    bus.ram_socket.bind(ram.socket);
    bus.bram_socket.bind(bram.socket);

    SimulationControl ctrl("SimCtrl");
    ctrl.uut_ptr = &uut;

    std::cout << "\n[SystemC] Starting LT simulation...\n" << std::endl;
    sc_start();    

    std::cout << "\nTotal simulation time: " << sc_time_stamp() << std::endl;

    save_matrix("../data/vp_output.txt", ADDR_Y, RAM_BASE, 1500, 512, ram);

    return 0;
}