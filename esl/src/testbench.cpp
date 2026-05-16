#include <systemc.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/tlm_quantumkeeper.h>
#include "header/vp.h" 

SC_MODULE(Testbench) {
    tlm_utils::simple_initiator_socket<Testbench> isoc;

    SC_CTOR(Testbench) : isoc("isoc") {
        SC_THREAD(run_test);
    }

    void run_test() {
        tlm_utils::tlm_quantumkeeper qk;
        qk.reset();
        sc_time loct = SC_ZERO_TIME;

        std::cout << "\nLoading matrices via TLM..." << std::endl;
        load_matrix_tlm("../data/matrice/multihead_ulaz_Q.txt", ADDR_Q,     loct, qk);
        load_matrix_tlm("../data/matrice/multihead_ulaz_K.txt", ADDR_K,     loct, qk);
        load_matrix_tlm("../data/matrice/multihead_ulaz_V.txt", ADDR_V,     loct, qk);
        load_matrix_tlm("../data/matrice/multihead_W_out.txt",  ADDR_W_OUT, loct, qk);
        load_matrix_tlm("../data/matrice/multihead_b_out.txt",  ADDR_B_OUT, loct, qk);

        std::cout << "Writing CTRL=1 to start hardware..." << std::endl;
        {
            tlm::tlm_generic_payload pl;
            unsigned char cmd = 1;
            pl.set_command(tlm::TLM_WRITE_COMMAND);
            pl.set_address(ADDR_CTRL);
            pl.set_data_ptr(&cmd);
            pl.set_data_length(1);
            pl.set_streaming_width(1);  
            pl.set_byte_enable_ptr(0);
            pl.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
            isoc->b_transport(pl, loct);
            qk.set_and_sync(loct);
        }

        std::cout << "Polling status..." << std::endl;
        {
            tlm::tlm_generic_payload pl;
            unsigned char status = 0;
            pl.set_command(tlm::TLM_READ_COMMAND);
            pl.set_address(ADDR_STATUS);
            pl.set_data_ptr(&status);
            pl.set_data_length(1);
            pl.set_streaming_width(1);  
            pl.set_byte_enable_ptr(0);

            while (true) {
                pl.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
                isoc->b_transport(pl, loct);
                qk.set_and_sync(loct);

                if (status == 1) break;

                loct += sc_time(100, SC_NS);
                qk.set_and_sync(loct);
            }
        }

        std::cout << "Hardware done. Saving output..." << std::endl;
        save_matrix_tlm("../data/vp_output.txt", ADDR_Y, 1500, 512, loct, qk);

        std::cout << "Complete @ " << sc_time_stamp() << std::endl;
        sc_stop();
    }

private:
    void load_matrix_tlm(const std::string& filename, uint64_t addr,
                         sc_time& loct, tlm_utils::tlm_quantumkeeper& qk)
    {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "ERROR: cannot open " << filename << std::endl;
            return;
        }
        std::vector<float> buf;
        float v;
        while (file >> v) buf.push_back(v);

        unsigned int len = buf.size() * sizeof(float);
        tlm::tlm_generic_payload pl;
        pl.set_command(tlm::TLM_WRITE_COMMAND);
        pl.set_address(addr);
        pl.set_data_ptr(reinterpret_cast<unsigned char*>(buf.data()));
        pl.set_data_length(len);
        pl.set_streaming_width(len);
        pl.set_byte_enable_ptr(0);
        pl.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

        isoc->b_transport(pl, loct);
        qk.set_and_sync(loct);

        std::cout << "[TB]   " << buf.size() << " floats → 0x"
                  << std::hex << addr << std::dec << std::endl;
    }

    void save_matrix_tlm(const std::string& filename, uint64_t addr,
                         int rows, int cols,
                         sc_time& loct, tlm_utils::tlm_quantumkeeper& qk)
    {
        std::vector<float> buf(rows * cols);
        unsigned int len = buf.size() * sizeof(float);

        tlm::tlm_generic_payload pl;
        pl.set_command(tlm::TLM_READ_COMMAND);
        pl.set_address(addr);
        pl.set_data_ptr(reinterpret_cast<unsigned char*>(buf.data()));
        pl.set_data_length(len);
        pl.set_streaming_width(len); 
        pl.set_byte_enable_ptr(0);
        pl.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

        isoc->b_transport(pl, loct);
        qk.set_and_sync(loct);

        std::ofstream file(filename);
        for (float val : buf)
            file << std::fixed << std::setprecision(6) << val << "\n";

        std::cout << "Saved " << buf.size() << " floats to " << filename << std::endl;
    }
};

int sc_main(int argc, char* argv[]) {
    tlm::tlm_global_quantum::instance().set(sc_time(10, SC_NS));

    vp platform("Virtual_Platform");
    Testbench tb("Testbench");
    tb.isoc.bind(platform.s_cpu);

    std::cout << "\n[SystemC] Starting TLM 2.0 LT simulation...\n" << std::endl;
    sc_start();

    std::cout << "\nTotal simulation time: " << sc_time_stamp() << std::endl;
    return 0;
}