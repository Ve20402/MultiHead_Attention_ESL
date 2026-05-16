#include "header/multi_head.h"
#include <iostream>

using namespace std;

MultiHeadModule::MultiHeadModule(sc_module_name name) 
    : sc_module(name), heads("heads", NUM_HEADS), socket("socket") 
{
    qk.reset();
    SC_THREAD(multi_head_process);
}

float MultiHeadModule::mem_read(uint64_t addr) {
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

void MultiHeadModule::mem_write(uint64_t addr, float val) {
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

void MultiHeadModule::mem_read_block(uint64_t addr, float* data_ptr, uint32_t num_elements) {
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
        
        unsigned int cycles = num_elements; // pretpostavimo 1 element po ciklusu
        qk.inc(sc_time(cycles * 10, SC_NS));
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

void MultiHeadModule::mem_write_block(uint64_t addr, float* data_ptr, uint32_t num_elements) {
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

void MultiHeadModule::multi_head_process() {
    
    cout << "Waiting for start signal..." << endl;

    while (true) {
         wait(ev_start);
         done_flag = false;
        for (int i = 0; i < NUM_HEADS; i++) {
            uint32_t head_cols = COLS_W / NUM_HEADS; 
            uint32_t input_offset = i * head_cols * 4; 
            
            heads[i].q_addr = reg_q_addr + input_offset;
            heads[i].k_addr = reg_k_addr + input_offset;
            heads[i].v_addr = reg_v_addr + input_offset;
            heads[i].out_addr = reg_y_addr + (i * HEAD_MEM_SPACE); 
            heads[i].rows = ROWS_W;
            heads[i].cols = head_cols;
            heads[i].trigger_start();
        }

        sc_core::sc_event_and_list all_heads_done;
        for (int i = 0; i < NUM_HEADS; i++) {
            all_heads_done &= heads[i].ev_done;
        }
        wait(all_heads_done);

        uint64_t final_res_offset = 0x200000; 
        for(int h=0; h<NUM_HEADS; h++) {
            uint64_t check_addr = reg_y_addr + (h * HEAD_MEM_SPACE) + final_res_offset;
            float first_val = mem_read(check_addr);
            cout << "   > Head " << h << " on 0x" << hex << check_addr << " : " << dec << first_val << endl;
        }

		cout << "\nFinal projection..." << endl;
        
        std::vector<float> W_proj_buffer(COLS_W * COLS_W); // 512 x 512
        std::vector<float> B_proj_buffer(COLS_W);   // 512

        for (uint32_t k = 0; k < COLS_W; k++) {
            mem_read_block(reg_w_addr + (k * COLS_W) * 4, &W_proj_buffer[k * COLS_W], COLS_W);
        }
        mem_read_block(reg_b_addr, B_proj_buffer.data(), COLS_W);

        std::vector<float> Y_row_buffer(COLS_W);
        std::vector<float> OUT_row_buffer(COLS_W);

        for (uint32_t i = 0; i < ROWS_W; i++) {
            
            for(int h = 0; h < NUM_HEADS; h++) {
                uint64_t head_y_addr = reg_y_addr + (h * HEAD_MEM_SPACE) + final_res_offset + (i * 64) * 4;
                mem_read_block(head_y_addr, &Y_row_buffer[h * 64], 64);
            }

            for (uint32_t j = 0; j < COLS_W; j++) {
                float sum = 0.0f;
                for (uint32_t k = 0; k < COLS_W; k++) {

                    sum += Y_row_buffer[k] * W_proj_buffer[k * COLS_W + j];
                }
                OUT_row_buffer[j] = sum + B_proj_buffer[j];
            }

            qk.inc(sc_time(((COLS_W / 8) * COLS_W) * 10, SC_NS));   //simuliramo unroll = 8, pa zbog toga delimo sa 8, ali moze i bez toga
            qk.set_and_sync(qk.get_local_time()); 

            mem_write_block(reg_y_addr + (i * COLS_W) * 4, OUT_row_buffer.data(), COLS_W);
        }

        cout << "Final results returned to 0x" << hex << reg_y_addr << dec << endl;
        done_flag = true;
        ev_done.notify();
    }
}