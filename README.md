# PIMsim
## What is it?
```
PIM Simulator
- Transaction generator + DRAMsim3 + PIM Functional Simulator
- 요약 (자세한 사항은 해당 폴더의 README.md에 기재됨)
  . PIM Functional Simulator를 기존 DRAMsim3와 연결하여, Memory에 대한 Transaction을
    기반으로 동작하는 Simulator입니다.
  . Transaction generator가 PIM 연산을 수행하기 위한 Transaction을 생성하여 (DRAMsim3
	+ PIM Functional Simulator)에 전달하여 수행합니다.
  . 결국, Memory Transaction을 이용하여 PIM 연산을 수행할 수 있으며, Memory Timing 
    정보를 이용한 각 Memory Transaction의 수행 cycle수, 완료 cycle을 얻어낼 수 있다.

- Transaction generator 
  . PIM Functional Simulator의 동작을 확인하기 위하여, PIM 연산을 수행하기 위한 Memory
    Transaction을 생성하여 (DRAMsim3 + PIM Functional Simulator)에 전달한다.
  . 간략하게 Memory에 Transaction을 전달하는 Host 역할을 수행한다.
- DRAMsim3
  . 전달받은 Memory Transaction을 Memory Timing 정보를 이용하여 각 Memory Transaction의
    수행 cycle수, 완료 cycle을 얻어낸다.
  . https://github.com/umd-memsys/DRAMsim3 의 코드를 사용하였다.
- PIM Functional Simulator
  . Memory Transaction을 통해 PIM 연산을 수행한다.

```

# PIMFuncSim
## What is it?
```
PIM Functional Simulator
- for checking PIM operation
- 요약 (자세한 사항은 해당 폴더의 README.md에 기재됨)
  . PIM 연산을 수행하기 위하여, μKernel과 그에 맞는 Transaction을 계획할 필요가 있다.
  . 연산에 필요한 μKernal과 Transaction을 설정하고 이를 수행하여, 올바른 PIM 연산을 
    수행하는지 확인할 수 있는 Simulator이다.
```
