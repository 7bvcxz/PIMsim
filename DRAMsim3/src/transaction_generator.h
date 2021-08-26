#ifndef __TRANSACTION_GENERATOR_H
#define __TRANSACTION_GENERATOR_H

#include <cstdint>
#include <sys/mman.h>
#include "memory_system.h"
#include "configuration.h"
#include "common.h"


#define EVEN_BANK 0
#define ODD_BANK  1

#define NUM_WORD_PER_ROW     32
#define NUM_CHANNEL          16
#define NUM_BANK_PER_CHANNEL 16
#define NUM_BANK             (NUM_BANK_PER_CHANNEL * NUM_CHANNEL)
#define SIZE_WORD            32
#define SIZE_ROW             (SIZE_WORD * NUM_WORD_PER_ROW)
#define UNIT_SIZE            2  // FP16

#define MAP_SBMR             0x3fff
#define MAP_ABMR             0x3ffe
#define MAP_PIM_OP_MODE      0x3ffd
#define MAP_CRF              0x3ffc
#define MAP_GRF              0x3ffb
#define MAP_SRF              0x3ffa


namespace dramsim3 {

class TransactionGenerator {
   public:
    TransactionGenerator(const std::string& config_file, const::string& output_dir)
        : memory_system_(
              config_file, output_dir,
              std::bind(&TransactionGenerator::ReadCallBack, this, std::placeholders::_1, std::placeholders::_2),
              std::bind(&TransactionGenerator::WriteCallBack, this, std::placeholders::_1)),
          config_(new Config(config_file, output_dir)),
          clk_(0) {
        pmemAddr_size_ = (uint64_t)4 * 1024 * 1024 * 1024;
        pmemAddr_ = (uint8_t *) mmap(NULL, pmemAddr_size_,
                                     PROT_READ | PROT_WRITE,
                                     MAP_ANON | MAP_PRIVATE,
                                     -1, 0);
        if (pmemAddr_ == (uint8_t*) MAP_FAILED)
            perror("mmap");
        burstSize_ = 32;

        data_temp_ = (uint8_t *) malloc(burstSize_);

        memory_system_.init(pmemAddr_, pmemAddr_size_, burstSize_);
    }
    ~TransactionGenerator() { delete(config_); }
    // virtual void ClockTick() = 0;
    virtual void Initialize() = 0;
    virtual void SetData() = 0;
    virtual void Execute() = 0;
    virtual void GetResult() = 0;
    void ReadCallBack(uint64_t addr, uint8_t *DataPtr);
    void WriteCallBack(uint64_t addr);
    void PrintStats() { memory_system_.PrintStats(); }
    uint64_t ReverseAddressMapping(Address& addr);
    uint64_t Ceiling(uint64_t num, uint64_t stride);
    void TryAddTransaction(uint64_t hex_addr, bool is_write, uint8_t *DataPtr);
    void Barrier();

   protected:
    MemorySystem memory_system_;
    const Config *config_;
    uint8_t *pmemAddr_;
    uint64_t pmemAddr_size_;
    unsigned int burstSize_;
    uint64_t clk_;

    uint8_t *data_temp_;
};

class AddTransactionGenerator : public TransactionGenerator {
   public:
    AddTransactionGenerator(const std::string& config_file,
                            const std::string& output_dir,
                            uint64_t n,
                            uint8_t *x,
                            uint8_t *y,
                            uint8_t *z)
        : TransactionGenerator(config_file, output_dir),
          n_(n), x_(x), y_(y), z_(z) {}
    void Initialize() override;
    void SetData() override;
    void Execute() override;
    void GetResult() override;

   private:
    uint8_t *x_, *y_, *z_;
    uint64_t n_;
    uint64_t addr_x_, addr_y_;
    uint64_t ukernel_access_size_;
    uint64_t ukernel_count_per_pim_;
    uint32_t *ukernel_;
};


class GemvTransactionGenerator : public TransactionGenerator {
   public:
    GemvTransactionGenerator(const std::string& config_file,
                             const std::string& output_dir,
                             uint64_t m,
                             uint64_t n,
                             uint8_t *A,
                             uint8_t *x,
                             uint8_t *y)
        : TransactionGenerator(config_file, output_dir),
          m_(m), n_(n), A_(A), x_(x), y_(y) {}
    void Initialize() override;
    void SetData() override;
    void Execute() override;
    void GetResult() override;

   private:
    void ExecuteBank(int bank);

    uint8_t *A_, *x_, *y_;
    uint8_t *A_T_;
    uint64_t m_, n_;
    uint64_t addr_A_, addr_y_;
    uint64_t ukernel_access_size_;
    uint64_t ukernel_count_per_pim_;
    uint32_t *ukernel_gemv_;
    uint32_t *ukernel_reduce_;
};

}  // namespace dramsim3
#endif