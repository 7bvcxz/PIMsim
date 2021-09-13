# PIMSim
PIMSim은 삼성의 PIM-DRAM[1]의 동작을 테스트할 수 있는 DRAMSim3 기반의 시뮬레이션 환경을 제공한다. PIMSim은 PIM 동작 수행 및 테스트를 하기 위해 아래 두 개의 추가적인 모듈을 제공한다.
 - **Transaction Generator**: PIM에서 특정 연산(e.g., ADD, GEMV)을 수행하기 위해 host-side transaction을 생성
 - **PIM Function Simulator**: Host에서 넘어오는 transaction에 따라 PIM 동작을 시뮬레이션 한다.

## Overall Architecture

## Modified DRAMSim3
- DRAMSim 자체에서 physical memory 관리
  - Transaction에 data pointer 포함
  - Read, Write 시 DRAMSim 내부에서 physcial memory에 read/write 수행

## PIM Function Simulator
- 현재 DRAM의 모드 변경 (SB ↔ AB ↔ AB-PIM)
  - SB(Single bank mode): 단일 bank에서 수행
  - AB()

## Transaction Generator

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
└── ext/half                 # Additional library to support FP16 calculation
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

기존 DRAMsim3는 Simulation을 수행하기 위해 cpu.cc에서 하위 부분에게 특정 Tracefile에 기록된 transaction들이나 특정 횟수만큼의 Random transaction을 전달하여 Memory 성능측정을 하였다. 하지만 PIM Simulator의 경우, PIM을 수행하기 위한 특별한 transaction을 사용하며, 다양한 크기에 대한 연산을 지원하기 위하여 (e.g., vector length 100에 대한 ADD 연산 <-> vector length 150에 대한 ADD 연산) 개별적인 transaction generator를 개발하였다.

또한, DRAMsim3는 각 transaction의 접근 address를 기반으로 complete/execution cycle을 측정하고 이를 이용해 Memory의 성능을 평가하기 때문에 실제 Data를 Read/Write하지 않고, 접근 address에 대한 timing만을 고려한다. 하지만, PIM Simulator는 실제 Data를 Read/Write 하고, 저장된 Data는 PIM을 수행할 때 사용한다. 따라서, PIM Simulator는 추가적으로 Data를 Read/Write할 수 있는 Physical memory를 정의하였고, Transaction에 추가적으로 Read 한 data를 저장하거나 Write 할 data를 저장하는 Data Pointer를 구현하였다.

결론적으로, transaction generator는 PIM을 수행하기 위하여
1. Physical memory, Test할 Operand data 정의
2. Operand data를 Physical memory에 저장하는 Transaction 생성 및 전달
3. PIM을 수행하기 위한 Transaction 생성 및 전달
의 역할을 수행한다.


전달된 transaction은
Transaction generator --> Dram system --> 해당 Controller(DRAMsim3 부분)
                                       └-> pim_func_sim 
으로 전달되며, pim_func_sim에서는 bank mode관리, physical memory에 Read/Write, 해당 pim_unit의 register 설정 및 transaction 전달을 수행한다.

pim_unit은 전달받은 transaction을 통해 PIM을 수행하며, PIM을 하기위한 연산동작은 모두 pim_unit에서 수행된다.
전달된 transaction을 통해 pim_unit은 

<아직 설명할 것 남은것 목록>
ㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡ
1. pim_unit 설명 및 그림 추가(순서도)
   - 내부 동작 순서
   - JUMP, NOP 구현 방법
   - SetOperandAddr 설명
2. pim_func_sim 설명 및 그림 추가(순서도)
3. add의 μKernel, data 저장 방식 정리
4. gemv의 μKernel, data 저장 방식 정리
5. mode 별 동작 설명
6. register의 값 바꾸는 방법 설명(CRF, SRF, GRF별 row/column address) 



Summary of PIM Simulator
1. Run PIM Simulator with a type of computation(e.g., GEMV, ADD, ...) and test data size(filled with random value)
2. Generate transactions by transaction_generator
3. Send transactions to dram_system
4. Dram_system sends transactions to proper controller & pim_func_sim
   1. proper controller (Same as DRAMsim3) (except for AB, AB-PIM mode --> TODO will be added about 0913, 0914) 
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


## Reference
[1] Lee, Sukhan, et al. "Hardware Architecture and Software Stack for PIM Based on Commercial DRAM Technology: Industrial Product." 2021 ACM/IEEE 48th Annual International Symposium on Computer Architecture (ISCA). IEEE, 2021.