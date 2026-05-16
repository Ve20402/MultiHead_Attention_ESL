#ifndef DATATYPES_H
#define DATATYPES_H

#ifndef SC_INCLUDE_FX
#define SC_INCLUDE_FX
#endif

#include <systemc.h>
#include <tlm.h>
#include <vector>

#define MAX_COLS 1024

using DATA_T = sc_fixed<32, 10>;
using PROB_T = sc_fixed<16, 1>;
using ACC_T  = sc_fixed<32, 12>;

typedef float mem_data_t;

using Matrix = std::vector<std::vector<DATA_T>>;
using Vector = std::vector<DATA_T>;

#endif