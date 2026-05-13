#include "header/matrix_multiplier.h"
#include <iostream>

using namespace std;

MatrixMultiplier::MatrixMultiplier(sc_module_name name) : sc_module(name), socket("socket") {
    stride_X = 0; stride_W = 0; stride_Y = 0;
    transpose_W = false; 
    qk.reset();
    SC_THREAD(multiply);
}

float MatrixMultiplier::mem_read(uint64_t addr) {
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

    if (trans.is_response_error()) {
        SC_REPORT_ERROR("TLM", "Error trying to read from memory!");
    }

    return val;
}

void MatrixMultiplier::mem_write(uint64_t addr, float val) {
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

    if (trans.is_response_error()) {
        SC_REPORT_ERROR("TLM", "Error trying to write to memory!");
    }
}

void MatrixMultiplier::mem_read_block(uint64_t addr, float* data_ptr, uint32_t num_elements) {
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
    } 
    else {
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
        if (trans.is_response_error()) {
            SC_REPORT_ERROR("TLM", "Error trying to read from memory!");
        }
    }
}

void MatrixMultiplier::mem_write_block(uint64_t addr, float* data_ptr, uint32_t num_elements) {
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

        qk.inc(sc_time(10, SC_NS)); 
        qk.set_and_sync(qk.get_local_time());
    } 
    else {
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

        if (trans.is_response_error()) {
            SC_REPORT_ERROR("TLM", "Error trying to write to memory!");
        }
    }
}

void MatrixMultiplier::multiply() {
    while (true) {
        wait(ev_start);
        
        uint32_t sX = (stride_X == 0) ? cols : stride_X;
        uint32_t sW = (stride_W == 0) ? cols : stride_W; 
        uint32_t sY = (stride_Y == 0) ? w_rows : stride_Y;

        uint32_t w_max_rows = transpose_W ? w_rows : cols;
        uint32_t w_max_cols = transpose_W ? cols : w_rows;
        
        std::vector<float> W_buffer(w_max_rows * sW, 0.0f);
        for (uint32_t r = 0; r < w_max_rows; r++) {
            mem_read_block(W_addr + r * sW * 4, &W_buffer[r * sW], w_max_cols);
        }

        std::vector<float> X_buffer(cols);
        std::vector<float> Y_buffer(w_rows);

        for (uint32_t i = 0; i < rows; i++) {
            
            mem_read_block(X_addr + (i * sX) * 4, X_buffer.data(), cols);
            
            for (uint32_t j = 0; j < w_rows; j++) {
                ACC_T suma = 0;                
                for (uint32_t k = 0; k < cols; k++) {                        
                    float valW_raw;
                    if (transpose_W) {
                        valW_raw = W_buffer[j * sW + k];
                    } else {
                        valW_raw = W_buffer[k * sW + j];
                    }
                    suma += (DATA_T)X_buffer[k] * (DATA_T)valW_raw;
                }
                Y_buffer[j] = (float)suma;
            }

            qk.inc(sc_time((cols / 8) * w_rows * 10, SC_NS)); 
            qk.set_and_sync(qk.get_local_time());
            mem_write_block(Y_addr + (i * sY) * 4, Y_buffer.data(), w_rows);

            if ((i + 1) % 500 == 0) {
                cout << "      [MM-PROGRESS] Head 0x" << hex << X_addr << dec 
                     << " sucessfuly completed " << (i + 1) << " rows." << endl;
            }
        }
        
        cout << "Multiplication completed!" << endl;
        ev_done.notify();
    }
}