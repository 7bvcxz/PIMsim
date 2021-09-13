# 이 문서는 Transaction Generator, DRAMsim3, Pim Function Simulator 실험 환경을 만드는 방법과, PIMsim 실험 환경을 만드는 방법을 가이드 합니다.

## 1. Code Structure
```
From DRAMsim3
    An event driven simulator that simulate memory transactions considering memory timing constraints. 
	It returns complete cycle and execution cycle for each transaction
└── bankstate
└── channel_state
└── command_queue
└── common
└── configuration
└── controller
└── cpu
└── dram_system
└── hmc
└── main
└── memory_system
└── refresh
└── simple_stats
└── thermal_replay
└── thermal_solver
└── thermal
└── timing

Added for PIM Simulator
└── half                     # Additional library to support FP16 calculation
└── main_pim                 # Main code to control PIM Simulator
└── pim_func_sim             # Get transaction and select the action that matches the transaction's purpose
                             #  Actions
                             #  1. Change Bank mode
                             #  2. Program pim_unit's register
                             #  3. Send transaction to pim_unit
└── pim_unit                 # Get transaction and execute PIM. Has several registers that could be 
                             # used for PIM. Can access to physical memory
└── pim_utils                # Several codes to support pim_unit
└── transaction_generator    # Make transactions to execute a sort of computation. It supports GEMV and ADD for now.
                               Host Simulator will perform transaction_generator in future
```

## 2. How it works
```
Summary of PIM Simulator
1. Set the type of computation and test data(filled with random value)
2. Generate and send transactions by transaction_generator
3. Send transactions to dram_system
4. Dram_system sends transactions to proper controller & pim_func_sim
   1. proper controller (Same as DRAMsim3) (except for AB, AB-PIM mode --> will be added about 0913, 0914) 
      1. Calculates and return transaction's complete cycle and execution cycle with memory's timing value
   2. pim_func_sim
      1. Control the bank mode and compute PIM
      2. Change bank mode, Read/Write data to physical memory, Send transactions to pim_unit 

How PIM works (PIM transaction flow)
1. Initialize Physical Memory and test data(filled with random value)
2. Set data in Physical Memory
   1. Transactions to write test data on physical memory
3. Program μKernel in pim_unit
   1. Transactions to change the bank mode into AB(All bank) mode
   2. Transactions to program μKernel in pim_unit
   3. Transactions to write data in pim_unit's register (for computation)
4. Execute PIM with pim_units
   1. Send transaction to pim_unit to execute PIM
      for each pim_unit, 
      1. Read data of transaction's addr from physical memory 
	  2. Read PIM instruction from CRF register
	  3. Set operand's address
	  4. Compute with operand's addresses and PIM instruction's OP code
	  5. Save data to DST(destination) address
5. Read computed data
   1. Transactions to change the bank mode into SB(Single bank) mode
   2. Transactions to read data from physical memory
```
![PIMsim](https://user-images.githubusercontent.com/80901560/133036861-5e8a13aa-01b6-4d54-9d6b-9a2360cbe28d.png)

## 3. Build PIM Simulator
```
$ mkdir build
$ cd build
$ cmake ..
$ make -j4
```

## 4. Run ADD 
```
./pimdramsim3main
    - ../configs/( memory setting )
        - only support 'HBM2_4Gb_test.ini' yet
    - --pim-api=add
    - --add-n= ( vector length )
        - only support multiple of 4096 yet

e.g., 
$ ./pimdramsim3main ../configs/HBM2_4Gb_test.ini --pim-api=add --add-n=4096
$ ./pimdramsim3main ../configs/HBM2_4Gb_test.ini --pim-api=add --add-n=8192
```

## 5. Run GEMV
```
./pimdramsim3main
    - ../configs/( memory setting )
        - only support 'HBM2_4Gb_test.ini' yet
    - --pim-api=gemv
    - --gemv-m= ( output length )
        - only support multiple of 4096 yet
    - --gemv-n= ( input length )
        - only support multiple of 32 yet
        
e.g., 
$ ./pimdramsim3main ../configs/HBM2_4Gb_test.ini --pim-api=gemv --gemv-m=4096 --gemv-n=32
$ ./pimdramsim3main ../configs/HBM2_4Gb_test.ini --pim-api=gemv --gemv-m=8192 --gemv-n=32
$ ./pimdramsim3main ../configs/HBM2_4Gb_test.ini --pim-api=gemv --gemv-m=8192 --gemv-n=96
```

## 6. Debug Mode
```
Can set debug mode in pim_config.h
Upgrade in progress,,, (written at 0913) (maybe finished at 0913)
```

## 7. Clean
```
$ make clean
```
