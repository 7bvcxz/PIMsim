#include <iostream>
#include "./../ext/headers/args.hxx"
#include "./transaction_generator.h"

using namespace dramsim3;

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
    args::ValueFlag<uint64_t> batch_arg(
        parser, "batch", "Batch size",
        {"batch"}, 1);
    args::ValueFlag<uint64_t> add_n_arg(
        parser, "add_n", "[ADD] Number of elements in vector x, y and z",
        {"add-n"}, 1024*1024);
    args::ValueFlag<uint64_t> gemv_m_arg(
        parser, "gemv_m", "[GEMV] Number of rows of the matrix A",
        {"gemv-m"}, 4096);
    args::ValueFlag<uint64_t> gemv_n_arg(
        parser, "gemv_n", "[GEMV] Number of columns of the matrix A",
        {"gemv-n"}, 4096);
    args::ValueFlag<double> miss_ratio_arg(
        parser, "miss_ratio", "Miss ratio",
        {"miss-ratio"}, 1);

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
    uint64_t b = args::get(batch_arg);

    // Initialize modules of PIM-Simulator
    //  Transaction Generator + DRAMsim3 + PIM Functional Simulator
    std::cout << C_GREEN << "Initializing modules..." << C_NORMAL << std::endl;
    TransactionGenerator * tx_generator;

    // Define operands and Transaction generator for simulating computation
    if (pim_api == "add") {
        uint64_t n = args::get(add_n_arg);

        // Define Transaction generator for ADD computation
        tx_generator = new CPUAddTransactionGenerator(config_file, output_dir,
                                                      b, n);
    } else if (pim_api == "gemv") {
        uint64_t m = args::get(gemv_m_arg);
        uint64_t n = args::get(gemv_n_arg);
        double miss_ratio = args::get(miss_ratio_arg);

        // Define Transaction generator for GEMV computation
        tx_generator = new CPUGemvTransactionGenerator(config_file, output_dir,
                                                       b, m, n, miss_ratio);
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
    clk = tx_generator->GetClk();
    tx_generator->Execute();
    clk = tx_generator->GetClk() - clk;
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
