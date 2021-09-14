# PIMsim
### PIMsim은 삼성의 PIM-DRAM[1]의 동작을 테스트할 수 있는 DRAMsim3 기반의 시뮬레이션 환경을 제공한다. PIMsim은 PIM 동작 수행 및 테스트를 하기 위해 아래 두 개의 추가적인 모듈을 제공한다
- **Transaction Generator**: PIM에서 특정 연산(e.g., ADD, GEMV)을 수행하기 위해 host-side transaction을 생성
- **PIM Functional Simulator**: Host에서 넘어오는 transaction에 따라 PIM 동작을 시뮬레이션 한다
<br><br>

## Overall Architecture
![image](https://user-images.githubusercontent.com/80901560/133197841-d0045dd5-ba19-4b88-9fce-7e3ab3947908.png)

위의 그림은 PIMsim의 전체적인 수행과정을 나타낸다

**Transaction Generator** 는 PIM에서 특정 연산을 수행하기 위해 host-side transaction을 생성하고 전달한다

또한, 실제 Data를 Read/Write할 수 있도록 Physical Memory를 제공한다

이에 **DRAMsim3** 는 Memory timing 정보를 고려하여 complete/execution time을 반환하고,

**PIM Functional Simulator** 는 DRAMsim3 로부터 transaction을 전달받아 Physical Memory에 Data를 Read/Write하거나, pim unit을 통하여 PIM 연산을 수행하고 연산결과를 Physical memory에 저장할 수 있다

따라서, **Transaction Generator** DRAMsim3와 PIM Functional Simulator로 부터 테스트의 cycle단위 결과 및 PIM 연산결과를 얻을 수 있다

(Transaction Generator는 현재 PIMsim의 전체 동작을 테스트하기 위한 Transaction을 생성하여 전달하는 Host 역할을 수행하며, 훗날 Gem5와 같은 Host Simulator로 대체할 수 있다)
<br><br>

## Transaction Generator
기존 DRAMsim3는 다음과 같은 방법으로 테스트를 수행한다
- Trace file을 통해 정해진 clock에 transaction 전달
- Random transaction을 생성
<br><br>

그러나, PIMsim의 경우 위의 두 가지 방법에 대해 다음과 같은 문제가 발생한다
- 다양한 크기를 가진 연산자에 대한 PIM 연산을 테스트 할 수 없다 (모든 크기에 대한 Trace file을 만들지 않는 이상)
- PIM을 수행하기 위해선 특정 transaction을 알맞은 clock timing에 보내야 한다
- 기존 테스트 환경은 Physical memory에 저장된 데이터를 고려하지 않는다
<br><br>

따라서, transaction generator를 통해 테스트를 수행하며 다음과 같은 특징을 제공한다
- PIM을 활용하기 위하여 High-level API (e.g., ADD, GEMV) 제공
  - High-level API 에 대한 transaction을 자동적으로 생성
- Physical memory 제공
  - Physical memory에 Read/Write를 수행하기 위해 transaction에 추가적으로 data pointer를 제공
<br><br>

### Transaction Generator의 Transaction 생성 및 전달 순서
다음과 같은 순서로 Transaction을 생성한다
1. SetData(): Operand Data를 Physical Memory에 저장하며, PIM 연산을 수행하기 위한 pim unit의 register 값을 설정한다
2. Execute(): PIM 연산을 수행하기 위하여, 각 PIM Instruction에 맞는 적절한 transaction을 전달한다
3. GetResult(): Physical Memory에 저장된 Data를 읽기 위한 transaction을 전달한다
<br><br>

## DRAMsim3
Memory timing을 고려하여 transaction의 complete/execution cycle을 제공한다
- DRAMsim3[3]의 코드를 사용하며, bank_mode에 따른 SimpleStats 업데이트를 추가하였다. (TODO 0914 완료예정)
<br><br>

## PIM Functional Simulator
PIM 동작을 시뮬레이션하기 위해 아래와 같은 기능들을 제공한다
- Physical memory Read/Write
- 현재 bank_mode 변경 (SB ↔ AB ↔ AB-PIM)
- PIM Register 값 설정 (CRF, GRF, SRF)
- PIM 연산 수행 및 저장
<br><br>

Physical memory Read/Write
- Physical memory에 Data를 Read/Write 한다.
<br><br>

현재 bank_mode 변경 (SB ↔ AB ↔ AB-PIM)
- SB(Single bank mode): 단일 bank에서 수행
- AB(All bank mode): Channel의 모든 bank에서 수행
- AB-PIM(All bank PIM mode): Channel의 모든 bank에서 수행 및 PIM 연산 수행
<br><br>  

pim unit의 Register 값 설정
- PIM 연산을 수행하기 위해 pim unit의 Register 값을 설정한다 (CRF, GRF, SRF)
<br><br>

PIM 연산 수행 및 저장
- pim unit에서 PIM 연산을 수행하며, 결과값을 Physical Memory에 저장할 수 있다
<br><br>
  

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

## 2. Build PIM Simulator
```
$ mkdir build
$ cd build
$ cmake ..
$ make -j4
```

## 3. Run ADD
ADD 연산의 길이(add-n)의 값을 설정하면, 해당 길이의 vector인 input x, y와 output z를 생성한 후 input의 값을 random한 값으로 채운다

이후, transaction generator를 이용하여 input x, y에 대한 ADD 연산을 수행하고, output z에 연산결과를 저장한다

그리고, PIM Simulator는 실제 연산결과와 PIM 연산결과인 z 사이의 오차값을 출력한다

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

## 4. Run GEMV
GEMV 연산의 길이(gemv-m, gemv-n)의 값을 설정하면, input으로 크기가 (m x n)인 Matrix A, (n)인 Vector x, output으로 크기가 (m)인 Vector y를 생성한 후 input의 값을 random한 값으로 채운다 

이후, transaction generator를 이용하여 input A, x에 대한 GEMV 연산을 수행하고, output y에 연산결과를 저장한다

그리고, PIM Simulator는 실제 연산결과와 PIM 연산결과인 y 사이의 오차값을 출력한다

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

## 5. Debug Mode
```
Can set debug mode in pim_config.h
Upgrade in progress,,, (written at 0913) (maybe finished at 0914)
```

## 6. Clean
```
$ make clean
```

## Reference
[1] Lee, Sukhan, et al. "Hardware Architecture and Software Stack for PIM Based on Commercial DRAM Technology: Industrial Product." 2021 ACM/IEEE 48th Annual International Symposium on Computer Architecture (ISCA). IEEE, 2021.
[2] S. Li, Z. Yang, D. Reddy, A. Srivastava and B. Jacob, "DRAMsim3: a Cycle-accurate, Thermal-Capable DRAM Simulator," in IEEE Computer Architecture Letters.
[3] https://github.com/umd-memsys/DRAMsim3 DRAMsim3 opensource code