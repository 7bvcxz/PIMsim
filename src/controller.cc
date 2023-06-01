#include "controller.h"
#include <iomanip>
#include <iostream>
#include <limits>

namespace dramsim3 {

#ifdef THERMAL
Controller::Controller(int channel, const Config &config, const Timing &timing,
                       ThermalCalculator &thermal_calc)
#else
Controller::Controller(int channel, const Config &config, const Timing &timing)
#endif  // THERMAL
    : 
#ifdef THERMAL
      thermal_calc_(thermal_calc),
#endif  // THERMAL
      write_buffer_threshold_(8),
      channel_id_(channel),
      clk_(0),
      config_(config),
      simple_stats_(config_, channel_id_),
      channel_state_(config, timing),
      cmd_queue_(channel_id_, config, channel_state_, simple_stats_),
      refresh_(config, channel_state_),
      is_unified_queue_(config.unified_queue),
      row_buf_policy_(config.row_buf_policy == "CLOSE_PAGE"
                          ? RowBufPolicy::CLOSE_PAGE
                          : RowBufPolicy::OPEN_PAGE),
      last_trans_clk_(0),
      write_draining_(0) {
    if (is_unified_queue_) {
        unified_queue_.reserve(config_.trans_queue_size);
    } else {
        read_queue_.reserve(config_.trans_queue_size);
        write_buffer_.reserve(config_.trans_queue_size);
    }

#ifdef CMD_TRACE
    std::string trace_file_name = config_.output_prefix + "ch_" +
                                  std::to_string(channel_id_) + "cmd.trace";
    std::cout << "Command Trace write to " << trace_file_name << std::endl;
    cmd_trace_.open(trace_file_name, std::ofstream::out);
#endif  // CMD_TRACE
}

std::pair<uint64_t, std::pair<int, uint8_t*>> Controller::ReturnDoneTrans(uint64_t clk) {
    auto it = return_queue_.begin();
    while (it != return_queue_.end()) {
        if (clk >= it->complete_cycle) {
            if (it->is_write) {
                simple_stats_.Increment("num_writes_done");    // hmm point  number of write requests done --> controller가 몇개의 write transaction을 완료했는지 --> no touch
            } else {
                simple_stats_.Increment("num_reads_done");     // hmm point  number of read requests done --> controller가 몇개의 read transaction을 완료했는지 --> no touch
                simple_stats_.AddValue("read_latency", clk_ - it->added_cycle);   // hmm point    read request latency (cycles) --> 이것도 그대로일꺼고 --> no touch
            }
            auto pair = std::make_pair(it->addr, std::make_pair(it->is_write, it->DataPtr));
            it = return_queue_.erase(it);
            return pair;
        } else {
            ++it;
        }
    }
    return std::make_pair(-1, std::make_pair(-1, nullptr));
}

void Controller::ClockTick() {
    // update refresh counter
    refresh_.ClockTick();

    bool cmd_issued = false;
    Command cmd;
    if (channel_state_.IsRefreshWaiting()) {
        cmd = cmd_queue_.FinishRefresh();
    }

    // cannot find a refresh related command or there's no refresh
    if (!cmd.IsValid()) {
        cmd = cmd_queue_.GetCommandToIssue();
    }

    if (cmd.IsValid()) {
        IssueCommand(cmd);
        cmd_issued = true;

        if (config_.enable_hbm_dual_cmd) {
            auto second_cmd = cmd_queue_.GetCommandToIssue();
            if (second_cmd.IsValid()) {
                if (second_cmd.IsReadWrite() != cmd.IsReadWrite()) {
                    IssueCommand(second_cmd);
                    simple_stats_.Increment("hbm_dual_cmds");       // number of cycles dual cmds issued   --> 기능을 꺼서 안하는거로 --> no touch
                }
            }
        }
    }

    // power updates pt 1
    for (int i = 0; i < config_.ranks; i++) {
        if (channel_state_.IsRankSelfRefreshing(i)) {
            simple_stats_.IncrementVec("sref_cycles", i);  // no touch  --> 사용안함
        } else {
            bool all_idle = channel_state_.IsAllBankIdleInRank(i);
            if (all_idle) {
                simple_stats_.IncrementVec("all_bank_idle_cycles", i);       // 모든 bank 놀고있는지 --> no touch
                channel_state_.rank_idle_cycles[i] += 1;
            } else {
                simple_stats_.IncrementVec("rank_active_cycles", i);         // no touch
                // reset
                channel_state_.rank_idle_cycles[i] = 0;
            }
        }
    }

    // power updates pt 2: move idle ranks into self-refresh mode to save power
    if (config_.enable_self_refresh && !cmd_issued) {
        for (auto i = 0; i < config_.ranks; i++) {
            if (channel_state_.IsRankSelfRefreshing(i)) {
                // wake up!
                if (!cmd_queue_.rank_q_empty[i]) {
                    auto addr = Address();
                    addr.rank = i;
                    auto cmd = Command(CommandType::SREF_EXIT, addr, -1);
                    cmd = channel_state_.GetReadyCommand(cmd, clk_);
                    if (cmd.IsValid()) {
                        IssueCommand(cmd);
                        break;
                    }
                }
            } else {
                if (cmd_queue_.rank_q_empty[i] &&
                    channel_state_.rank_idle_cycles[i] >=
                        config_.sref_threshold) {
                    auto addr = Address();
                    addr.rank = i;
                    auto cmd = Command(CommandType::SREF_ENTER, addr, -1);
                    cmd = channel_state_.GetReadyCommand(cmd, clk_);
                    if (cmd.IsValid()) {
                        IssueCommand(cmd);
                        break;
                    }
                }
            }
        }
    }

    ScheduleTransaction();
    clk_++;
    cmd_queue_.ClockTick();
    simple_stats_.Increment("num_cycles");    // no touch
    return;
}

bool Controller::WillAcceptTransaction(uint64_t hex_addr, bool is_write) const {
    if (is_unified_queue_) {
        return unified_queue_.size() < unified_queue_.capacity();
    } else if (!is_write) {
        return read_queue_.size() < read_queue_.capacity();
    } else {
        return write_buffer_.size() < write_buffer_.capacity();
    }
}

bool Controller::AddTransaction(Transaction trans) {
    trans.added_cycle = clk_;
    simple_stats_.AddValue("interarrival_latency", clk_ - last_trans_clk_);    // no touch,  latency between requests (interarrival)
    last_trans_clk_ = clk_;

    if (trans.is_write) {
		//std::cout << std::hex << clk_ << "\twrite\t" << trans.addr << std::dec << std::endl;
        if (pending_wr_q_.count(trans.addr) == 0) {  // can not merge writes
            pending_wr_q_.insert(std::make_pair(trans.addr, trans));
            if (is_unified_queue_) {
                unified_queue_.push_back(trans);
            } else {
                write_buffer_.push_back(trans);
            }
        }
        trans.complete_cycle = clk_ + 1;
        return_queue_.push_back(trans);
        return true;
    } else {  // read
        //std::cout << std::hex << clk_ << "\tread\t" << trans.addr << std::dec << std::endl;
        // if in write buffer, use the write buffer value
		if (pending_wr_q_.count(trans.addr) > 0) {
            trans.complete_cycle = clk_ + 1;
            return_queue_.push_back(trans);
            return true;
        }
        pending_rd_q_.insert(std::make_pair(trans.addr, trans));
        if (pending_rd_q_.count(trans.addr) == 1) {
            if (is_unified_queue_) {
                unified_queue_.push_back(trans);
            } else {
                read_queue_.push_back(trans);
            }
        }
        return true;
    }
}

void Controller::ScheduleTransaction() {
    // determine whether to schedule read or write
    if (write_draining_ == 0 && !is_unified_queue_) {
        // we basically have a upper and lower threshold for write buffer
        if ((write_buffer_.size() >= write_buffer_.capacity()) ||
            ((int)write_buffer_.size() > write_buffer_threshold_ && cmd_queue_.QueueEmpty())) {
            write_draining_ = write_buffer_.size();
        }
    }

    std::vector<Transaction> &queue =
        is_unified_queue_ ? unified_queue_
                          : write_draining_ > 0 ? write_buffer_ : read_queue_;
    for (auto it = queue.begin(); it != queue.end(); it++) {
        auto cmd = TransToCommand(*it);
        //std::cout << (*it).executed_bankmode << std::endl;  ok
        if (cmd_queue_.WillAcceptCommand(cmd.Rank(), cmd.Bankgroup(),
                                         cmd.Bank())) {
            if (!is_unified_queue_ && cmd.IsWrite()) {
                // Enforce R->W dependency
                if (pending_rd_q_.count(it->addr) > 0) {
                    write_draining_ = 0;
                    break;
                }
                write_draining_ -= 1;
            }
            cmd_queue_.AddCommand(cmd);
            //std::cout << cmd.executed_bankmode << std::endl;  ok
            queue.erase(it);
            break;
        }
    }
}

void Controller::IssueCommand(const Command &cmd) {
//std::cout << cmd.executed_bankmode;
#ifdef CMD_TRACE
    cmd_trace_ << std::left << std::setw(18) << clk_ << " " << cmd << std::endl;
#endif  // CMD_TRACE
#ifdef THERMAL
    // add channel in, only needed by thermal module
    thermal_calc_.UpdateCMDPower(channel_id_, cmd, clk_);
#endif  // THERMAL
    // if read/write, update pending queue and return queue
    if (cmd.IsRead()) {
        auto num_reads = pending_rd_q_.count(cmd.hex_addr);
        if (num_reads == 0) {
            std::cerr << cmd.hex_addr << " not in read queue! " << std::endl;
            exit(1);
        }
        // if there are multiple reads pending return them all
        while (num_reads > 0) {
            auto it = pending_rd_q_.find(cmd.hex_addr);
		    //std::cout << std::hex << clk_ << "\tread\t" << cmd.hex_addr << std::dec << std::endl;
            it->second.complete_cycle = clk_ + config_.read_delay;
            return_queue_.push_back(it->second);
            pending_rd_q_.erase(it);
            num_reads -= 1;
        }
    } else if (cmd.IsWrite()) {
        // there should be only 1 write to the same location at a time
        auto it = pending_wr_q_.find(cmd.hex_addr);
        if (it == pending_wr_q_.end()) {
            std::cerr << cmd.hex_addr << " not in write queue!" << std::endl;
            exit(1);
        }
        auto wr_lat = clk_ - it->second.added_cycle + config_.write_delay;
        simple_stats_.AddValue("write_latency", wr_lat);     // write cmd latency(cycles) ,,, no touch
		//std::cout << std::hex << clk_ << "\twrite\t" << cmd.hex_addr << std::dec << std::endl;
        pending_wr_q_.erase(it);
    }
    // must update stats before states (for row hits)
    //std::cout << cmd.executed_bankmode;
    UpdateCommandStats(cmd);
    channel_state_.UpdateTimingAndStates(cmd, clk_);
}

Command Controller::TransToCommand(const Transaction &trans) {
    auto addr = config_.AddressMapping(trans.addr);
    CommandType cmd_type;
    if (row_buf_policy_ == RowBufPolicy::OPEN_PAGE) {
        cmd_type = trans.is_write ? CommandType::WRITE : CommandType::READ;
    } else {
        cmd_type = trans.is_write ? CommandType::WRITE_PRECHARGE
                                  : CommandType::READ_PRECHARGE;
    }
    return Command(cmd_type, addr, trans.addr, trans.executed_bankmode);    // >> mmm <<
}

int Controller::QueueUsage() const { return cmd_queue_.QueueUsage(); }

void Controller::PrintEpochStats() {
    simple_stats_.Increment("epoch_num");           // no touch
    simple_stats_.PrintEpochStats();                // no touch
#ifdef THERMAL
    for (int r = 0; r < config_.ranks; r++) {
        double bg_energy = simple_stats_.RankBackgroundEnergy(r);     // sum of act, pre, sref energy
        thermal_calc_.UpdateBackgroundEnergy(channel_id_, r, bg_energy);
    }
#endif  // THERMAL
    return;
}

void Controller::PrintFinalStats() {                
    simple_stats_.PrintFinalStats();                 // no touch

#ifdef THERMAL
    for (int r = 0; r < config_.ranks; r++) {
        double bg_energy = simple_stats_.RankBackgroundEnergy(r);    // sum of act, pre, sref energy
        thermal_calc_.UpdateBackgroundEnergy(channel_id_, r, bg_energy);
    }
#endif  // THERMAL
    return;
}

void Controller::UpdateCommandStats(const Command &cmd) {
    switch (cmd.cmd_type) {
        case CommandType::READ:
        case CommandType::READ_PRECHARGE:
            // >> mmm
            if(cmd.executed_bankmode == "SB") {
                simple_stats_.Increment("num_read_cmds");                   // number of read/readp commands
            } else {
                for(int i=0; i<config_.banks; i++)
                    simple_stats_.Increment("num_read_cmds");                   // number of read/readp commands
            }
            // mmm <<
            if (channel_state_.RowHitCount(cmd.Rank(), cmd.Bankgroup(),
                                           cmd.Bank()) != 0) {
                simple_stats_.Increment("num_read_row_hits");           // number of read row buffer hits     no touch I think
            }
            break;
        case CommandType::WRITE:
        case CommandType::WRITE_PRECHARGE:
            // >> mmm
            if(cmd.executed_bankmode == "SB") {
                simple_stats_.Increment("num_write_cmds");                   // number of write/writep commands
            } else {
                for(int i=0; i<config_.banks; i++)
                    simple_stats_.Increment("num_write_cmds");
            }

            // mmm <<
            if (channel_state_.RowHitCount(cmd.Rank(), cmd.Bankgroup(),
                                           cmd.Bank()) != 0) {
                simple_stats_.Increment("num_write_row_hits");           // number of write row buffer hits     no touch I think
            }
            break;
        case CommandType::ACTIVATE:
            // >> mmm
            if(cmd.executed_bankmode == "SB") {
                simple_stats_.Increment("num_act_cmds");                     // number of act commands      
            } else {
                for(int i=0; i<config_.banks; i++)
                    simple_stats_.Increment("num_act_cmds");
            }
            // mmm <<
            break;
        case CommandType::PRECHARGE:
            // >> mmm
            if(cmd.executed_bankmode == "SB") {
                simple_stats_.Increment("num_pre_cmds");                     // number of pre commands        
            } else {
                for(int i=0; i<config_.banks; i++)
                    simple_stats_.Increment("num_pre_cmds");
            }
            // mmm <<
            break;
        case CommandType::REFRESH:                                        // >> hmm point    I remember this is about rank refresh
            simple_stats_.Increment("num_ref_cmds");                     // number of refresh commands        
            break;
        case CommandType::REFRESH_BANK:
            // >> mmm
            if(cmd.executed_bankmode == "SB") {
                simple_stats_.Increment("num_refb_cmds");                     // number of pre commands        
            } else {
                for(int i=0; i<config_.banks; i++)
                    simple_stats_.Increment("num_refb_cmds");
            }
            // mmm <<
            break;
        case CommandType::SREF_ENTER:  // no touch
            simple_stats_.Increment("num_srefe_cmds");                   // number of self ref ~      no touch       
            break;
        case CommandType::SREF_EXIT:  // no touch
            simple_stats_.Increment("num_srefx_cmds");                   // number of self ref exit ~     no touch
            break;
        default:
            AbruptExit(__FILE__, __LINE__);
    }
}

bool Controller::IsPendingTransaction() {
    if (pending_rd_q_.size() == 0 && pending_wr_q_.size() == 0)
        return false;
    else
        return true;
}

}  // namespace dramsim3
