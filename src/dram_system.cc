#include "dram_system.h"

#include <assert.h>

namespace dramsim3 {

// alternative way is to assign the id in constructor but this is less
// destructive
int BaseDRAMSystem::total_channels_ = 0;

BaseDRAMSystem::BaseDRAMSystem(Config &config, const std::string &output_dir,
                               std::function<void(uint64_t, uint8_t*)> read_callback,
                               std::function<void(uint64_t)> write_callback)
    : read_callback_(read_callback),
      write_callback_(write_callback),
      last_req_clk_(0),
      config_(config),
      timing_(config_),
#ifdef THERMAL
      thermal_calc_(config_),
#endif  // THERMAL
      clk_(0) {
    total_channels_ += config_.channels;

    pim_func_sim_ = new PimFuncSim(config);

#ifdef ADDR_TRACE
    std::string addr_trace_name = config_.output_prefix + "addr.trace";
    address_trace_.open(addr_trace_name);
#endif
}

void BaseDRAMSystem::init(uint8_t* pmemAddr_, uint64_t pmemAddr_size_,
                          unsigned int burstSize_) {
    pmemAddr = pmemAddr_;
    pmemAddr_size = pmemAddr_size_;
    burstSize = burstSize_;
    std::cout << "DramSys initialized!\n";
    pim_func_sim_->init(pmemAddr_, pmemAddr_size_, burstSize_);
}

int BaseDRAMSystem::GetChannel(uint64_t hex_addr) const {
    hex_addr >>= config_.shift_bits;
    return (hex_addr >> config_.ch_pos) & config_.ch_mask;
}

void BaseDRAMSystem::PrintEpochStats() {
    // first epoch, print bracket
    if (clk_ - config_.epoch_period == 0) {
        std::ofstream epoch_out(config_.json_epoch_name, std::ofstream::out);
        epoch_out << "[";
    }
    for (size_t i = 0; i < ctrls_.size(); i++) {
        ctrls_[i]->PrintEpochStats();
        std::ofstream epoch_out(config_.json_epoch_name, std::ofstream::app);
        epoch_out << "," << std::endl;
    }
#ifdef THERMAL
    thermal_calc_.PrintTransPT(clk_);
#endif  // THERMAL
    return;
}

void BaseDRAMSystem::PrintStats() {
    // Finish epoch output, remove last comma and append ]
    std::ofstream epoch_out(config_.json_epoch_name, std::ios_base::in |
                                                         std::ios_base::out |
                                                         std::ios_base::ate);
    epoch_out.seekp(-2, std::ios_base::cur);
    epoch_out.write("]", 1);
    epoch_out.close();

    std::ofstream json_out(config_.json_stats_name, std::ofstream::out);
    json_out << "{";

    // close it now so that each channel can handle it
    json_out.close();
    for (size_t i = 0; i < ctrls_.size(); i++) {
        ctrls_[i]->PrintFinalStats();
        if (i != ctrls_.size() - 1) {
            std::ofstream chan_out(config_.json_stats_name, std::ofstream::app);
            chan_out << "," << std::endl;
        }
    }
    json_out.open(config_.json_stats_name, std::ofstream::app);
    json_out << "}";

#ifdef THERMAL
    thermal_calc_.PrintFinalPT(clk_);
#endif  // THERMAL
}

void BaseDRAMSystem::ResetStats() {
    for (size_t i = 0; i < ctrls_.size(); i++) {
        ctrls_[i]->ResetStats();
    }
}

//void BaseDRAMSystem::RegisterCallbacks(
//    std::function<void(uint64_t)> read_callback,
//    std::function<void(uint64_t)> write_callback) {
//    // TODO(a) this should be propagated to controllers
//    read_callback_ = read_callback;
//    write_callback_ = write_callback;
// }

bool BaseDRAMSystem::IsPendingTransaction() {
    for (size_t i = 0; i < ctrls_.size(); i++) {
        if (ctrls_[i]->IsPendingTransaction())
            return true;
    }
    return false;
}

void BaseDRAMSystem::SetWriteBufferThreshold(int threshold) {
    for (size_t i = 0; i < ctrls_.size(); i++) {
        ctrls_[i]->write_buffer_threshold_ = (threshold < 0) ? 8 : threshold;
    }
}

JedecDRAMSystem::JedecDRAMSystem(Config &config, const std::string &output_dir,
                                 std::function<void(uint64_t, uint8_t*)> read_callback,
                                 std::function<void(uint64_t)> write_callback)
    : BaseDRAMSystem(config, output_dir, read_callback, write_callback) {
    if (config_.IsHMC()) {
        std::cerr << "Initialized a memory system with an HMC config file!"
                  << std::endl;
        AbruptExit(__FILE__, __LINE__);
    }

    ctrls_.reserve(config_.channels);
    for (auto i = 0; i < config_.channels; i++) {
#ifdef THERMAL
        ctrls_.push_back(new Controller(i, config_, timing_, thermal_calc_));
#else
        ctrls_.push_back(new Controller(i, config_, timing_));
#endif  // THERMAL
    }
}

JedecDRAMSystem::~JedecDRAMSystem() {
    for (auto it = ctrls_.begin(); it != ctrls_.end(); it++) {
        delete (*it);
    }
}

bool JedecDRAMSystem::WillAcceptTransaction(uint64_t hex_addr,
                                            bool is_write) const {
    int channel = GetChannel(hex_addr);
    return ctrls_[channel]->WillAcceptTransaction(hex_addr, is_write);
}

bool JedecDRAMSystem::AddTransaction(uint64_t hex_addr, bool is_write,
                                     uint8_t *DataPtr) {
    #if 0
	if(is_write)
	    std::cout << std::hex << clk_ << "\twrite\t" << hex_addr << std::dec << std::endl;
	else
	    std::cout << std::hex << clk_ << "\tread\t" << hex_addr << std::dec << std::endl;
    #endif

// Record trace - Record address trace for debugging or other purposes
#ifdef ADDR_TRACE
    address_trace_ << std::hex << hex_addr << std::dec << " "
                   << (is_write ? "WRITE " : "READ ") << clk_ << std::endl;
#endif


    int channel = GetChannel(hex_addr);
    bool ok = ctrls_[channel]->WillAcceptTransaction(hex_addr, is_write);


    assert(ok);
    if (ok) {
        Transaction trans = Transaction(hex_addr, is_write, DataPtr);
		// Address addr = config_.AddressMapping(hex_addr);
        // Send transaction to PIM Functional Simulator
        //  Performs physical memory RD/WR, bank mode change, set PIM register,
        //  execute PIM computation and write result to physical memory
        //if(addr.channel == 0 && (addr.bank == 0 && addr.bank == 1))
        //    std::cout << clk_ << "\t" << std::hex << hex_addr + 0x5000 << std::dec << std::endl; // << " " << is_write << " to PFSim\tch: " << addr.channel << "\tba: " << (addr.bankgroup * 4 + addr.bank) << "\tco: " << addr.column << "\tro: " << addr.row << std::endl;
        pim_func_sim_->AddTransaction(&trans);

        #if 0
		if(is_write)
		    std::cout << std::hex << clk_ << "\twrite\t" << hex_addr << std::dec << std::endl;
		else
		    std::cout << std::hex << clk_ << "\tread\t" << hex_addr << std::dec << std::endl;
        #endif
        
        // Send transaction to proper channel's controller
        //  returns transaction's complete/execution cycle considering memory
        //  timing information
        ctrls_[channel]->AddTransaction(trans);
    }
    last_req_clk_ = clk_;
    return ok;
}

void JedecDRAMSystem::ClockTick() {
    for (size_t i = 0; i < ctrls_.size(); i++) {
        // look ahead and return earlier
        while (true) {
            auto pair = ctrls_[i]->ReturnDoneTrans(clk_);
            if (pair.second.first == 1) {
                write_callback_(pair.first);
            } else if (pair.second.first == 0) {
                read_callback_(pair.first, pair.second.second);
            } else {
                break;
            }
        }
    }
    for (size_t i = 0; i < ctrls_.size(); i++) {
        ctrls_[i]->ClockTick();
    }
    clk_++;

    if (clk_ % config_.epoch_period == 0) {
        PrintEpochStats();
    }
    return;
}

IdealDRAMSystem::IdealDRAMSystem(Config &config, const std::string &output_dir,
                                 std::function<void(uint64_t, uint8_t*)> read_callback,
                                 std::function<void(uint64_t)> write_callback)
    : BaseDRAMSystem(config, output_dir, read_callback, write_callback),
      latency_(config_.ideal_memory_latency) {}

IdealDRAMSystem::~IdealDRAMSystem() {}

bool IdealDRAMSystem::AddTransaction(uint64_t hex_addr, bool is_write,
                                     uint8_t *DataPtr) {
    auto trans = Transaction(hex_addr, is_write, DataPtr);
    trans.added_cycle = clk_;
    infinite_buffer_q_.push_back(trans);
    return true;
}

void IdealDRAMSystem::ClockTick() {
    for (auto trans_it = infinite_buffer_q_.begin();
         trans_it != infinite_buffer_q_.end();) {
        if (clk_ - trans_it->added_cycle >= static_cast<uint64_t>(latency_)) {
            if (trans_it->is_write) {
                write_callback_(trans_it->addr);
            } else {
                read_callback_(trans_it->addr, trans_it->DataPtr);
            }
            trans_it = infinite_buffer_q_.erase(trans_it++);
        }
        if (trans_it != infinite_buffer_q_.end()) {
            ++trans_it;
        }
    }

    clk_++;
    return;
}

}  // namespace dramsim3
