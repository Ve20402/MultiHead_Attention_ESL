
OUTPUT_DIR = ../data

SYSTEMC_HOME ?= /usr/local/systemc
SYSTEMC_ARCH = linux64
SYSTEMC_LINK = -lsystemc

CXX = g++
CXXFLAGS = -O3 -DSC_INCLUDE_FX -I. -Iheader -I.

SRCS = src/testbench.cpp \
       src/ram_tlm.cpp \
       src/bram_tlm.cpp \
       src/interconnect.cpp \
       src/matrix_multiplier.cpp \
       src/single_head.cpp \
       src/multi_head.cpp

SC_INCLUDE = -I$(SYSTEMC_HOME)/include
SC_LIB = -L$(SYSTEMC_HOME)/lib-$(SYSTEMC_ARCH) -Wl,-rpath=$(SYSTEMC_HOME)/lib-$(SYSTEMC_ARCH) $(SYSTEMC_LINK)

PYTHON = python3
PYBIND_INCLUDES = $(shell $(PYTHON) -m pybind11 --includes)
PYBIND_SUFFIX = $(shell python3-config --extension-suffix)
PYBIND_FLAGS = -shared -fPIC

SC_INCLUDE = -I$(SYSTEMC_HOME)/include
SC_LIB = -L$(SYSTEMC_HOME)/lib-$(SYSTEMC_ARCH) -Wl,-rpath=$(SYSTEMC_HOME)/lib-$(SYSTEMC_ARCH) -lsystemc

LIB_NAME = multihead_attention_algorithm$(PYBIND_SUFFIX)
VP_EXE = simulation_tlm

.PHONY: all check_venv check_deps help run_app vp install_deps clean

all: help

check_venv:
ifndef VIRTUAL_ENV
	$(error ERROR: Niste u Python virtuelnom okruzenju. Molimo Vas da ga aktivirate koristeci instrukcije na README-u.)
endif
	@echo "Trenutno Python virtuelno okruzenje: $(VIRTUAL_ENV)"

check_deps: check_venv
	@# 1. Provera SystemC biblioteke...
	@echo "int main() { return 0; }" > .test_sysc.cpp
	@$(CXX) .test_sysc.cpp $(SC_LIB) -o .test_sysc_out > /dev/null 2>&1 && \
		(echo "SystemC pronadjen."; rm .test_sysc.cpp .test_sysc_out) || \
		(echo "GRESKA: SystemC nije pronadjen ili se ne može linkovati."; \
		 echo "Molim Vas da pratite README za instrukcije."; \
		 rm .test_sysc.cpp; exit 1)

	@# 2. Provera PortAudio biblioteke...
	@# Trenutno ne moguce bez uvodjenja build sistema, a zadatak trazi da bude Makefile

	@# 3. Provera da li su python biblioteke instalirane
	@if [ -z "$(PYBIND_INCLUDES)" ]; then \
		echo "GRESKA: pybind11 nije pronadjen. Instaliraj sa 'make install_deps'"; \
		exit 1; \
	fi

help:
	@echo "------------------------------------------------------------------"
	@echo "Dostupne komande:"
	@echo "  make run_app     -> Glavni program"
	@echo "  make vp          -> Kompajlira i pokrece Virtuelnu Platformu (TLM)"
	@echo "                      "
	@echo "  make install_deps-> Instalira potrebne Python biblioteke"
	@echo "  make clean       -> Brise sve generisane fajlove"
	@echo "------------------------------------------------------------------"


run_app: check_deps
	@echo ">>> Kompajliranje C++ wrapper-a za Python..."
	@mkdir -p ../data/matrice
	$(CXX) $(CXXFLAGS) $(PYBIND_FLAGS) $(PYBIND_INCLUDES) src/pybind_wrapper.cpp src/attention_module_multiple_heads.cpp -o $(OUTPUT_DIR)/$(LIB_NAME)
	@echo ">>> Pokretanje Python aplikacije..."
	@# Menjamo folder u kome Python trazi biblioteke jer PyBind biblioteka koju smo napravili je tehnicki build artifact, po tom moramo da gledamo folder 'izlaz'
	@PYTHONPATH=$(OUTPUT_DIR):$(PYTHONPATH) $(PYTHON) final_app.py

vp: check_deps
	@echo ">>> Kompajliranje Virtuelne Platforme (TLM)..."
	$(CXX) $(CXXFLAGS) $(SC_INCLUDE) $(SRCS) $(SC_LIB) -o $(VP_EXE)
	@echo ">>> Pokretanje VP simulacije..."
	./$(VP_EXE)

install_deps: check_venv
	@echo ">>> Instaliranje Python biblioteka..."
	@# Instaliramo PyTorch bez GPU datoteka jer nam stvarno ne trebaju, a ogrmoni su fajlovi
	$(PYTHON) -m pip install torch==2.9.1 --index-url https://download.pytorch.org/whl/cpu
	@# Instaliramo sve ostalo
	$(PYTHON) -m pip install numpy transformers==4.35.0 sounddevice soundfile pybind11

clean:
	@echo ">>> Ciscenje..."
	rm -f $(OUTPUT_DIR)/$(LIB_NAME) $(VP_EXE)
	rm -f *.o *.so
	rm -f vp_output.txt
	@echo ">>> Zavrseno."

