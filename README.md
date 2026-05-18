## Server Study

본 저장소는 게임 서버의 기초를 학습하는 과정을 기록합니다.

<br>

### 1. Task Scheduler

주어진 태스크를 최적의 스레드에 배정하여 태스크를 최적화.

<br>

##### 성능 측정 데이터 (Benchmark)

##### 동일 조건의 태스크 실행 시간 비교 (단위: `ms`)

#### (1) `Central Queue`
```
Avg : 754.093ms
Min : 714.692ms
```
기준 모델. 단일 락 경합이 주요 병목.

<br>

#### (2) `Local Queue`
```
Avg : 821.981ms
Min : 763.293ms
```
락 경합은 줄었으나, 스레드 간 불균형으로 오히려 성능 하락.

<br>

#### (3) `Work Stealing`
```
Avg : 736.806ms
Min : 695.875ms
```
유휴 스레드 지원으로 개선되었으나, 훔쳐오는 비용이 존재함.

<br>

#### (4) `Batch Processing`

태스크 구성 변경으로 중앙큐 재측정
```
Batch Processing Avg 1727.130ms / Min 1670.483ms
Central Queue  Avg 1757.729ms / Min 1666.661ms
```
락 획득 횟수를 줄여 `lock contention`을 줄임.

<br>

##### 분석 회고

중앙큐를 압도하는 스케줄링 방법은 찾지 못함.

반복 측정시 중앙큐와 다른 방법의 우열이 바뀜.

<br>

##### 원인 분석

태스크 초기 배치 전략: 현재 모든 태스크를 중앙 큐에 주입한 뒤 스레드가 가져가는 구조를 취하고 있음.

가설: 초기 중앙 큐 접근 시의 락 경합이 전체 실행 시간에 큰 비중을 차지하여, 알고리즘 간의 변별력을 흐리는 요인으로 작용했을 가능성이 있음.

<br>

#### To-do List

(1) Profiler를 사용해 실제로 병목이 발생하는 지점을 확인.
(2) `Lock-free Queue`: `atomic`을 통한 큐 구현을 시도하여 락 자체의 오버헤드를 제거.

<br>

### 2. Network

게임 서버 기능을 구현하면서 학습하는 과정.

(1) `Single-threaded`: 1:1 통신의 기초. 클라이언트가 보낸 정보를 그대로 보냄. 블로킹 소켓의 사용으로 다중 접속 불가 확인.
(2) `Multi-threaded`: 클라이언트당 스레드 할당. 접속자 증가 시 컨텍스트 스위칭 오버헤드 발생.

(3) `IOCP Core`: `Overlapped I/O`와 `Completion Key`를 사용하여 비동기 입출력 구현.
(4) `AcceptEx`: 소켓 수락 과정도 비동기화하여 대량 접속 요청 시의 지연 시간 최적화.
(5) `DisconnectEx`: 세션 종료 시 소켓 자원 회수를 비동기화하여 안정성 강화.
(6) `Broadcast`: 패킷을 전체 세션에 전파하는 기능 구현.

<br>

#### 테스트 및 검증 (Test & Verification)

네트워크 레이어의 신뢰성 및 고부하 상황에서의 안정성을 검증함.

| 테스트 항목 | 목적 및 내용 | 결과 |
| :--- | :--- | :---: |
| `Fragmentation` | 클라이언트에서 분할 송신된 메시지가 서버 측에서 올바르게 조립되는지 확인 | **Pass** |
| `Sticky Packets` | 다수의 패킷이 하나의 수신 버퍼에 병합되어 전달될 때, 서버가 이를 정확히 분리하여 처리하는지 검증 | **Pass** |
| `Connection Stress` | 대량의 클라이언트가 재접속과 끊김을 반복하는 상황에서 서버의 안정성 및 자원 관리 능력 테스트 | **Pass** |
| `Broadcast` | 서버에서 다수의 클라이언트에게 패킷을 전파할 때, 모든 클라이언트가 정확히 수신하는지 확인 | **Pass** |
| `Invalid Packet Handling` | 클라이언트에서 의도적으로 잘못된 패킷을 전송하여 서버의 예외 처리 및 보안 대응 능력 검증 | **Pass** |

<br>

##### `Connection Stress` 상세 결과
고부하 상황에서의 안정적인 연결 및 재연결 처리를 검증하기 위해 테스트를 수행함.

*   **테스트 환경 및 조건**
    *   `Thread Count`: 100 Threads
    *   `Iteration`: 스레드당 1,000회 (총 100,000회 접속/해제 반복)
    *   `Retry Logic`: 연결 실패 시 지수 백오프(`Exponential Backoff`) 적용
    (최대 32회, `Sleep(1 << retryCount)` 방식의 대기 시간 증분 처리)

*   **수치 데이터**
    *   **총 연결 시도**: 100,000 회
    *   **성공 / 실패**: 100,000 회 / 0 회 (**Success Rate 100%**)
    *   **총 소요 시간**: **38,565 ms** (약 38.5초, 초당 약 2,600건 처리)

*   **분석 및 검토**
    *   100개의 스레드가 경합하는 환경에서도 교착 상태나 리소스 누수 없이 100% 연결 성공함.
    *   지수 백오프 기반의 재시도 전략이 고부하 상황에서 유효하게 작동하여 안정적인 복구력을 증명함.

<br>

#### To-do List

(1) `AOI(Area of Interest) Broadcast`: 격자 혹은 `Quad-tree` 자료구조를 도입하여 주변 클라이언트에게만 패킷을 전파하는 최적화.

(2) Engine Integration: 구현된 Task Scheduler와 IOCP 네트워크 레이어를 결합하여 완전한 비동기 서버 엔진으로 통합.

### 3. Major Troubleshooting

학습 과정 중 발생한 고난도 버그와 그 해결 과정을 기록합니다.

<br>

[트러블슈팅: 로그 지연으로 인한 핸들 파괴 분석](./Troubleshooting.md)

문제: `AcceptEx` 도입 후, 로그 포맷만 바꿨는데 `WSARecv`에서 커널 에러(`ERROR_PATH_NOT_FOUND`)가 발생하는 기현상 발생.

원인: `std::chrono::current_zone()`의 오버헤드가 네트워크 루프의 `Critical Path`를 방해하여 OS가 소켓 핸들 통로를 무효화함.

해결: `git bisect`를 통해 범인을 특정하고, 로깅 로직을 경량화하여 타이밍 이슈 해결.

<br>

### 4. Build System
| 명령어 | 기능 및 동작 |
| :--- | :--- |
| `make all` | 서버, 클라이언트 및 모든 테스트 모듈을 일괄 빌드 |
| `make server` | 서버 실행 파일 빌드 |
| `make run-server` | 서버 빌드 후 즉시 실행 |
| `make client` | 클라이언트 실행 파일 빌드 |
| `make run-client` | 클라이언트 빌드 후 즉시 실행 |
| `make test-NN` | `Tests/NN_...` 폴더를 자동 탐색하여 해당 모듈 빌드 후 실행 |
| `make clean` | 빌드 결과물(`bin/`) 폴더 강제 삭제 및 환경 초기화 |

<br>

#### Environment & Build Stack
```

CPU: 8-Core / 16-Logical Processors
RAM: 32GB
OS: Windows (MinGW-w64)
Compiler: GCC (MinGW-w64)
Optimization: -O3
C++ Standard: C++20
Build Tool: GNU Make 4.4.1
```
