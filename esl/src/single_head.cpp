#include "header/single_head.h"
#include <iostream>

using namespace std;

SingleHeadModule::SingleHeadModule(sc_module_name name) 
    : sc_module(name), mat_mul("internal_mm"), socket("socket") 
{
    qk.reset();  
    SC_THREAD(single_head_process);
}

float SingleHeadModule::mem_read(uint64_t addr) {
    float val;
    tlm_generic_payload trans;
    sc_time delay = qk.get_local_time();

    trans.set_command(TLM_READ_COMMAND);
    trans.set_address(addr);
    trans.set_data_ptr((unsigned char*)&val);
    trans.set_data_length(4);
    trans.set_streaming_width(4);
    trans.set_byte_enable_ptr(0);
    trans.set_dmi_allowed(false);
    trans.set_response_status(TLM_INCOMPLETE_RESPONSE);

    socket->b_transport(trans, delay);
    qk.set_and_sync(delay);
    return val;
}

void SingleHeadModule::mem_write(uint64_t addr, float val) {
    tlm_generic_payload trans;
    sc_time delay = qk.get_local_time();

    trans.set_command(TLM_WRITE_COMMAND);
    trans.set_address(addr);
    trans.set_data_ptr((unsigned char*)&val);
    trans.set_data_length(4);
    trans.set_streaming_width(4);
    trans.set_byte_enable_ptr(0);
    trans.set_dmi_allowed(false);
    trans.set_response_status(TLM_INCOMPLETE_RESPONSE);

    socket->b_transport(trans, delay);
    qk.set_and_sync(delay);
}

void SingleHeadModule::mem_read_block(uint64_t addr, float* data_ptr, uint32_t num_elements) {
    bool in_range = dmi_ptr_valid && 
                   (addr >= dmi_cache.get_start_address()) && 
                   ((addr + num_elements * 4) <= dmi_cache.get_end_address());

    if (!in_range) {
        tlm_generic_payload tmp_trans;
        tmp_trans.set_address(addr);
        dmi_ptr_valid = socket->get_direct_mem_ptr(tmp_trans, dmi_cache);
    }

    if (dmi_ptr_valid) {
        unsigned char* dmi_mem = dmi_cache.get_dmi_ptr();
        uint64_t offset = addr - dmi_cache.get_start_address();
        memcpy(data_ptr, &dmi_mem[offset], num_elements * 4);
        
        qk.inc(sc_time(10, SC_NS)); 
        qk.set_and_sync(qk.get_local_time());
    } else {
        tlm_generic_payload trans;
        sc_time delay = qk.get_local_time();
        trans.set_command(TLM_READ_COMMAND);
        trans.set_address(addr);
        trans.set_data_ptr((unsigned char*)data_ptr);
        trans.set_data_length(num_elements * 4);
        trans.set_streaming_width(num_elements * 4);
        trans.set_byte_enable_ptr(0);
        trans.set_dmi_allowed(false);
        trans.set_response_status(TLM_INCOMPLETE_RESPONSE);

        socket->b_transport(trans, delay);
        qk.set_and_sync(delay);
    }
}

void SingleHeadModule::mem_write_block(uint64_t addr, float* data_ptr, uint32_t num_elements) {
    bool in_range = dmi_ptr_valid && 
                   (addr >= dmi_cache.get_start_address()) && 
                   ((addr + num_elements * 4) <= dmi_cache.get_end_address());

    if (!in_range) {
        tlm_generic_payload tmp_trans;
        tmp_trans.set_address(addr);
        dmi_ptr_valid = socket->get_direct_mem_ptr(tmp_trans, dmi_cache);
    }

    if (dmi_ptr_valid) {
        unsigned char* dmi_mem = dmi_cache.get_dmi_ptr();
        uint64_t offset = addr - dmi_cache.get_start_address();
        memcpy(&dmi_mem[offset], data_ptr, num_elements * 4);
        unsigned int cycles = num_elements; // pretpostavimo 1 element po ciklusu
        qk.inc(sc_time(cycles * 10, SC_NS)); 
        qk.set_and_sync(qk.get_local_time());
    } else {
        tlm_generic_payload trans;
        sc_time delay = qk.get_local_time();
        trans.set_command(TLM_WRITE_COMMAND);
        trans.set_address(addr);
        trans.set_data_ptr((unsigned char*)data_ptr);
        trans.set_data_length(num_elements * 4);
        trans.set_streaming_width(num_elements * 4);
        trans.set_byte_enable_ptr(0);
        trans.set_dmi_allowed(false);
        trans.set_response_status(TLM_INCOMPLETE_RESPONSE);

        socket->b_transport(trans, delay);
        qk.set_and_sync(delay);
    }
}

void SingleHeadModule::single_head_process() {
    while (true) {
        wait(ev_start);
        
        cout << "   [Head " << (q_addr & 0xF00) / 0x100 << "] 1/3: Calculating Q x K^T..." << endl;
        mat_mul.X_addr = q_addr;
        mat_mul.W_addr = k_addr; 
        mat_mul.Y_addr = out_addr;
        
        mat_mul.rows = rows;    
        mat_mul.cols = cols;    
        mat_mul.w_rows = rows; 
        
        mat_mul.stride_X = 512; 
        mat_mul.stride_W = 512; 
        mat_mul.stride_Y = rows;
        
        mat_mul.transpose_W = true; 

        mat_mul.trigger_start();
        wait(mat_mul.ev_done);
        qk.set_and_sync(mat_mul.qk.get_local_time());

        cout << "   [Head " << (q_addr & 0xF00) / 0x100 << "] 2/3: Calculating Softmax..." << endl;
        for (uint32_t i = 0; i < rows; i++) {
            std::vector<float> row(rows);
            
            mem_read_block(out_addr + (i * rows) * 4, row.data(), rows);

            float max_val = -1e9;
            for (uint32_t j = 0; j < rows; j++) {
                if (row[j] > max_val) max_val = row[j];
            }

            float sum = 0;
            for (uint32_t j = 0; j < rows; j++) {
                row[j] = exp(row[j] - max_val);
                sum += row[j];
            }

            qk.inc(sc_time(rows * 50, SC_NS)); 
            
            for (uint32_t j = 0; j < rows; j++) {
                row[j] = row[j] / sum;
            }

            mem_write_block(out_addr + (i * rows) * 4, row.data(), rows);
        }
        qk.set_and_sync(qk.get_local_time());

        cout << "   [Head " << (q_addr & 0xF00) / 0x100 << "] 3/3: Calculating Probs x V..." << endl;
        
        mat_mul.X_addr = out_addr;      
        mat_mul.W_addr = v_addr;        
        mat_mul.Y_addr = out_addr + 0x200000; 
        
        mat_mul.rows = rows;       
        mat_mul.cols = rows;       
        mat_mul.w_rows = cols;     
        
        mat_mul.stride_X = rows;   
        mat_mul.stride_W = 512;    
        mat_mul.stride_Y = 0;      
        
        mat_mul.transpose_W = false; 

        mat_mul.trigger_start();
        wait(mat_mul.ev_done);
        qk.set_and_sync(mat_mul.qk.get_local_time());
        cout << "   [Head " << (q_addr & 0xF00) / 0x100 << "] Completed!" << endl;

        ev_done.notify();
    }
}

