#include <iostream>
#include <random>
#include "./../ext/headers/args.hxx"
#include "./transaction_generator.h"
#include "half.hpp"

using namespace dramsim3;
using half_float::half;

// main code to simulate PIM simulator
int main(int argc, const char **argv) {
    srand(time(NULL));
    // parse simulation settings
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
    args::ValueFlag<uint64_t> mul_n_arg(
        parser, "mul_n", "[MUL] Number of elements in vector x, y and z",
        {"mul-n"}, 1024*1024);
    args::ValueFlag<uint64_t> gemv_m_arg(
        parser, "gemv_m", "[GEMV] Number of rows of the matrix A",
        {"gemv-m"}, 4096);
    args::ValueFlag<uint64_t> gemv_n_arg(
        parser, "gemv_n", "[GEMV] Number of columns of the matrix A",
        {"gemv-n"}, 4096);
    args::ValueFlag<uint64_t> bn_l_arg(
        parser, "bn_l", "[BatchNorm] Number of channels of the matrix A",
        {"bn-l"}, 64);
    args::ValueFlag<uint64_t> bn_f_arg(
        parser, "bn_f", "[BatchNorm] Height of the matrix A",
        {"bn-f"}, 512);
    args::ValueFlag<uint64_t> lstm_if_arg(
        parser, "lstm-if", "[LSTM] Number of input features",
        {"lstm-if"}, 1024);
    args::ValueFlag<uint64_t> lstm_of_arg(
        parser, "lstm-of", "[LSTM] Number of output features",
        {"lstm-of"}, 1024);

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

    // Initialize modules of PIM-Simulator
    //  Transaction Generator + DRAMsim3 + PIM Functional Simulator
    std::cout << C_GREEN << "Initializing modules..." << C_NORMAL << std::endl;
    TransactionGenerator * tx_generator;

    // Define operands and Transaction generator for simulating computation
	std::random_device random_device;
	auto rng = std::mt19937(random_device());
	auto f32rng = std::bind(std::normal_distribution<float>(0, 1), std::ref(rng));
    if (pim_api == "add") {
        uint64_t n = args::get(add_n_arg);

        // Define input vector x, y
        uint8_t *x = (uint8_t *) malloc(sizeof(uint16_t) * n);
        uint8_t *y = (uint8_t *) malloc(sizeof(uint16_t) * n);
        // Define output vector z
        uint8_t *z = (uint8_t *) malloc(sizeof(uint16_t) * n);

        // Fill input operands with random value
        for (int i=0; i< n; i++) {
            half h_x = half(f32rng());
            half h_y = half(f32rng());
            ((uint16_t*)x)[i] = *reinterpret_cast<uint16_t*>(&h_x);
            ((uint16_t*)y)[i] = *reinterpret_cast<uint16_t*>(&h_y);
        }

        // Define Transaction generator for ADD computation
        tx_generator = new AddTransactionGenerator(config_file, output_dir,
                                                   n, x, y, z);
    } else if (pim_api == "mul") {
        uint64_t n = args::get(mul_n_arg);

        // Define input vector x, y
        uint8_t *x = (uint8_t *) malloc(sizeof(uint16_t) * n);
        uint8_t *y = (uint8_t *) malloc(sizeof(uint16_t) * n);
        // Define output vector z
        uint8_t *z = (uint8_t *) malloc(sizeof(uint16_t) * n);

        // Fill input operands with random value
        for (int i=0; i< n; i++) {
            half h_x = half(f32rng());
            half h_y = half(f32rng());
            ((uint16_t*)x)[i] = *reinterpret_cast<uint16_t*>(&h_x);
            ((uint16_t*)y)[i] = *reinterpret_cast<uint16_t*>(&h_y);
        }

        // Define Transaction generator for ADD computation
        tx_generator = new MulTransactionGenerator(config_file, output_dir,
                                                   n, x, y, z);
    } else if (pim_api == "gemv") {
        uint64_t m = args::get(gemv_m_arg);
        uint64_t n = args::get(gemv_n_arg);

        // Define input matrix A, vector x
        uint8_t *A = (uint8_t *) malloc(sizeof(uint16_t) * m * n);
        uint8_t *x = (uint8_t *) malloc(sizeof(uint16_t) * n);
        // Define output vector y
        uint8_t *y = (uint8_t *) malloc(sizeof(uint16_t) * m);

        // Fill input operands with random value
        for (int i=0; i< n; i++) {
            half h_x = half(f32rng());
            ((uint16_t*)x)[i] = *reinterpret_cast<uint16_t*>(&h_x);
            for (int j=0; j< m; j++) {
                half h_A = half(f32rng());
                ((uint16_t*)A)[j*n+i] = *reinterpret_cast<uint16_t*>(&h_A);
            }
        }

        // Define Transaction generator for GEMV computation
        tx_generator = new GemvTransactionGenerator(config_file, output_dir,
                                                    m, n, A, x, y);
    } else if (pim_api == "bn") {
        uint64_t l = args::get(bn_l_arg);
        uint64_t f = args::get(bn_f_arg);

		uint64_t num_duplicate = 4096 / f;

        // Define input x, weight y, z
        uint8_t *x = (uint8_t *) malloc(sizeof(uint16_t) * l * f);
        uint8_t *y = (uint8_t *) malloc(sizeof(uint16_t) * 4096 * 8);
        uint8_t *z = (uint8_t *) malloc(sizeof(uint16_t) * 4096 * 8);
        // Define output vector w
        uint8_t *w = (uint8_t *) malloc(sizeof(uint16_t) * l * f);

        // Fill input operands with random value
        for (int fi=0; fi<f; fi++) {
            half h_y = half(f32rng());
            half h_z = half(f32rng());
			for (int coi=0; coi<num_duplicate*8; coi++) {
				((uint16_t*)y)[fi + coi*f] = *reinterpret_cast<uint16_t*>(&h_y);
				((uint16_t*)z)[fi + coi*f] = *reinterpret_cast<uint16_t*>(&h_z);
			}
            for (int li=0; li<l; li++) {
                half h_x = half(f32rng());
                ((uint16_t*)x)[li*f + fi] = *reinterpret_cast<uint16_t*>(&h_x);
            }
        }
        // Define Transaction generator for GEMV computation
        tx_generator = new BatchNormTransactionGenerator(config_file, output_dir,
                                                         l, f, x, y, z, w);
    } else if (pim_api == "lstm") {
        uint64_t i_f = args::get(lstm_if_arg);
        uint64_t o_f = args::get(lstm_of_arg);

        // Define input matrix W, vector x, h, b
        uint8_t *x = (uint8_t *) malloc(sizeof(uint16_t) * i_f);
        uint8_t *h = (uint8_t *) malloc(sizeof(uint16_t) * i_f);
        uint8_t *b = (uint8_t *) malloc(sizeof(uint16_t) * o_f * 4);
        uint8_t *Wx = (uint8_t *) malloc(sizeof(uint16_t) * i_f * o_f * 4);
        uint8_t *Wh = (uint8_t *) malloc(sizeof(uint16_t) * i_f * o_f * 4);
        // Define output vector y
        uint8_t *y = (uint8_t *) malloc(sizeof(uint16_t) * o_f * 4);

        // Fill input operands with random value
        for (int i_fi=0; i_fi<i_f; i_fi++) {
            half h_x = half(f32rng());
            half h_h = half(f32rng());
            ((uint16_t*)x)[i_fi] = *reinterpret_cast<uint16_t*>(&h_x);
            ((uint16_t*)h)[i_fi] = *reinterpret_cast<uint16_t*>(&h_h);
        }
        for (int o_fi=0; o_fi<o_f*4; o_fi++) {
            half h_b = half(0);
            //half h_b = half(f32rng());
            ((uint16_t*)b)[o_fi] = *reinterpret_cast<uint16_t*>(&h_b);
        }
        
        for (int o_fi=0; o_fi<o_f*4; o_fi++) {
            for (int i_fi=0; i_fi<i_f; i_fi++) {
                half h_wx = half(f32rng());
                half h_wh = half(f32rng());
                ((uint16_t*)Wx)[i_f*o_fi + i_fi] = *reinterpret_cast<uint16_t*>(&h_wx);
                ((uint16_t*)Wh)[i_f*o_fi + i_fi] = *reinterpret_cast<uint16_t*>(&h_wh);
            }
        }

        // Define Transaction generator for GEMV computation
        tx_generator = new LstmTransactionGenerator(config_file, output_dir,
                                                    i_f, o_f, x, y, h, b, Wx, Wh);
                           
    }
    std::cout << C_GREEN << "Success Module Initialize" << C_NORMAL << "\n\n";

	uint64_t clk;

    // Initialize variables and ukernel
    std::cout << C_GREEN << "Initializing severals..." << C_NORMAL << std::endl;
	clk = tx_generator->GetClk();
    tx_generator->Initialize();
	clk = tx_generator->GetClk() - clk;
    std::cout << C_GREEN << "Success Initialize (" << clk << " cycles)" << C_NORMAL << "\n\n";

    // Write operand data and μkernel to physical memory and PIM registers
    std::cout << C_GREEN << "Setting Data..." << C_NORMAL << "\n";
	clk = tx_generator->GetClk();
    tx_generator->SetData();
	clk = tx_generator->GetClk() - clk;
    std::cout << C_GREEN << "Success SetData (" << clk << " cycles)" << C_NORMAL << "\n\n";

    // Execute PIM computation
    std::cout << C_GREEN << "Executing..." << C_NORMAL << "\n";
    tx_generator->is_print_ = true;
	clk = tx_generator->GetClk();
    tx_generator->start_clk_ = clk;
    tx_generator->Execute();
	clk = tx_generator->GetClk() - clk;
    tx_generator->is_print_ = false;
    std::cout << C_GREEN << "Success Execute (" << clk << " cycles)" << C_NORMAL << "\n\n";

    // Read PIM computation result from physical memory
    std::cout << C_GREEN << "Getting Result..." << C_NORMAL << "\n";
	clk = tx_generator->GetClk();
    tx_generator->GetResult();
	clk = tx_generator->GetClk() - clk;
    std::cout << C_GREEN << "Success GetResult (" << clk << " cycles)" << C_NORMAL << "\n\n";

    // Calculate error between the result of PIM computation and actual answer
    tx_generator->CheckResult();

    tx_generator->PrintStats();

    delete tx_generator;

    return 0;
}
