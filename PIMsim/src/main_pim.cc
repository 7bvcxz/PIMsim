#include <iostream>
#include "./../ext/headers/args.hxx"
#include "transaction_generator.h"
#include "half.hpp"

using namespace dramsim3;
using half_float::half;

int main(int argc, const char **argv) {
	srand(time(NULL));
    args::ArgumentParser parser(
        "PIM-DRAM Simulator.",
        "Examples: \n."
        "./build/pimdramsim3main configs/DDR4_8Gb_x8_3200.ini -c 100 -t "
        "sample_trace.txt\n"
        "./build/pimdramsim3main configs/DDR4_8Gb_x8_3200.ini -s random -c 100");
    args::HelpFlag help(parser, "help", "Display the help menu", {'h', "help"});
    args::ValueFlag<uint64_t> num_cycles_arg(parser, "num_cycles",
                                             "Number of cycles to simulate",
                                             {'c', "cycles"}, 100000);
    args::ValueFlag<std::string> output_dir_arg(
        parser, "output_dir", "Output directory for stats files",
        {'o', "output-dir"}, ".");
    args::Positional<std::string> config_arg(
        parser, "config", "The config file name (mandatory)");
    args::ValueFlag<std::string> pim_api_arg(
        parser, "pim_api", "PIM API - add, gemv",
        {"pim-api"}, "add");
    args::ValueFlag<uint64_t> add_n_arg(
        parser, "add_n", "[ADD] Number of elements in vector x, y and z",
        {"add-n"}, 1024*1024);
    args::ValueFlag<uint64_t> gemv_m_arg(
        parser, "gemv_m", "[GEMV] Number of rows of the matrix A",
        {"gemv-m"}, 4096);
    args::ValueFlag<uint64_t> gemv_n_arg(
        parser, "gemv_n", "[GEMV] Number of columns of the matrix A",
        {"gemv-n"}, 4096);

    try {
        parser.ParseCLI(argc, argv);
    } catch (args::Help) {
        std::cout << parser;
        return 0;
    } catch (args::ParseError e) {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    }

    std::string config_file = args::get(config_arg);
    if (config_file.empty()) {
        std::cerr << parser;
        return 1;
    }

    uint64_t cycles = args::get(num_cycles_arg);
    std::string output_dir = args::get(output_dir_arg);
    std::string pim_api = args::get(pim_api_arg);


    std::cout << C_GREEN << "Initializing..." << C_NORMAL << std::endl;
    
    TransactionGenerator * tx_generator;
    if (pim_api == "add") {
        uint64_t n = args::get(add_n_arg);

        uint8_t *x = (uint8_t *) malloc(sizeof(uint16_t) * n);
        uint8_t *y = (uint8_t *) malloc(sizeof(uint16_t) * n);
        uint8_t *z = (uint8_t *) malloc(sizeof(uint16_t) * n);

		for(int i=0; i<n; i++){
            half h_x = half(rand() / static_cast<float>(RAND_MAX));
            half h_y = half(rand() / static_cast<float>(RAND_MAX));
			((uint16_t*)x)[i] = *reinterpret_cast<uint16_t*>(&h_x);
			((uint16_t*)y)[i] = *reinterpret_cast<uint16_t*>(&h_y);
		}

        tx_generator = new AddTransactionGenerator(config_file, output_dir, n, x, y, z);
    }
    else if (pim_api == "gemv") {
        uint64_t m = args::get(gemv_m_arg);
        uint64_t n = args::get(gemv_n_arg);
        uint8_t *A = (uint8_t *) malloc(sizeof(uint16_t) * m * n);
        uint8_t *x = (uint8_t *) malloc(sizeof(uint16_t) * n);
        uint8_t *y = (uint8_t *) malloc(sizeof(uint16_t) * m);
        
		for(int i=0; i<n; i++){
            half h_x = half(rand() / static_cast<float>(RAND_MAX));
			((uint16_t*)x)[i] = *reinterpret_cast<uint16_t*>(&h_x);
            for (int j=0; j<m; j++) {
                half h_A = half(rand() / static_cast<float>(RAND_MAX));
                ((uint16_t*)A)[j*n+i] = *reinterpret_cast<uint16_t*>(&h_A);
            }
		}

        tx_generator = new GemvTransactionGenerator(config_file, output_dir, m, n, A, x, y);
    }

    tx_generator->Initialize();
    std::cout << C_GREEN << "Success Initialize" << C_NORMAL << std::endl << std::endl;
    
    std::cout << C_GREEN << "Setting Data..." << C_NORMAL << std::endl;
    tx_generator->SetData();
    std::cout << C_GREEN << "Success SetData" << C_NORMAL << std::endl << std::endl;

    std::cout << C_GREEN << "Executing..." << C_NORMAL << std::endl;
    tx_generator->Execute();
    std::cout << C_GREEN << "Success Execute" << C_NORMAL << std::endl << std::endl;

    std::cout << C_GREEN << "Getting Result..." << C_NORMAL << std::endl;
    tx_generator->GetResult();
    std::cout << C_GREEN << "Success GetResult" << C_NORMAL << std::endl << std::endl;
    tx_generator->CheckResult();

    tx_generator->PrintStats();

    delete tx_generator;

    return 0;
}
