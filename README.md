    Server Study
    본 저장소는 게임 서버의 기초를 학습하는 과정을 기록합니다.

    1. Task Scheduler
    주어진 태스크를 최적의 스레드에 배정하여 태스크를 최적화.

    성능 측정 데이터 (Benchmark)
    동일 조건의 태스크 실행 시간 비교 (단위: ms)

    (1) Central Queue
    Avg : 754.093ms
    Min : 714.692ms
    기준 모델. 단일 락 경합이 주요 병목.

    (2) Local Queue
    Avg : 821.981ms
    Min : 763.293ms
    락 경합은 줄었으나, 스레드 간 불균형으로 오히려 성능 하락.

    (3) Work Stealing
    Avg : 736.806ms
    Min : 695.875ms
    유휴 스레드 지원으로 개선되었으나, 훔쳐오는 비용이 존재함.

    (4) Batch Processing
    태스크 구성 변경으로 중앙큐 재측정
    Batch Processing Avg 1727.130ms / Min 1670.483ms
    중앙큐  Avg 1757.729ms / Min 1666.661ms
    락 획득 횟수를 줄여 lock contention을 줄임.

    분석 회고
    중앙큐를 압도하는 방법은 찾지 못함.
    반복 측정시 중앙큐와 다른 방법의 우열이 바뀜.

    원인 분석
    태스크 초기 배치 전략: 현재 모든 태스크를 중앙 큐에 주입한 뒤 스레드가 가져가는 구조를 취하고 있음.
    가설: 초기 중앙 큐 접근 시의 락 경합이 전체 실행 시간에 큰 비중을 차지하여, 알고리즘 간의 변별력을 흐리는 요인으로 작용했을 가능성이 있음.

    To-do List
    (1) Tracy 등의 Profiler를 사용해 실제로 병목이 발생하는 지점을 확인.
    (2) Lock-free Queue: atomic을 통한 큐 구현을 시도하여 락 자체의 오버헤드를 제거.

    2. Network
    게임 서버 기능을 구현하면서 학습하는 과정.

    (1) Single-threaded: 1:1 통신의 기초. 클라이언트가 보낸 정보를 그대로 보냄. 블로킹 소켓의 사용으로 다중 접속 불가 확인.
    (2) Multi-threaded: 클라이언트당 스레드 할당. 접속자 증가 시 컨텍스트 스위칭 오버헤드 발생.
    (3) IOCP Core: Overlapped I/O와 Completion Key를 사용하여 비동기 입출력 구현.
    (4) AcceptEx: 소켓 수락 과정도 비동기화하여 대량 접속 요청 시의 지연 시간 최적화.

    To-do List
    (1) 패킷 조립, 버퍼 오버플로우 처리, race condition 제거 등 현재까지의 구현이 정확한지 검증.
    (2) 에코 서버를 넘어 실제 서버다운 기능을 추가.
    Broadcast 시스템 구축
    Global Broadcast: 전체 세션 리스트를 순회하며 메시지를 전파하는 기본 기능 구현.
    AOI(Area of Interest) Broadcast: 격자 혹은 쿼드트리 자료구조를 도입하여 주변 클라이언트에게만 패킷을 전파하는 최적화.
    (3) Zero-byte Receive: 대량의 세션 대기 시 Non-paged pool 메모리 사용량을 최소화하는 최적화 기법 적용.
    (4) DisconnectEx: 세션 종료 및 소켓 자원 회수를 비동기화하여 안정성 강화.
    (5) Engine Integration: 구현된 Task Scheduler와 IOCP 네트워크 레이어를 결합하여 완전한 비동기 서버 엔진으로 통합.

    3. Major Troubleshooting
    학습 과정 중 발생한 고난도 버그와 그 해결 과정을 기록합니다.

    [트러블슈팅: 로그 지연으로 인한 핸들 파괴 분석](./Troubleshooting.md)
    문제: AcceptEx 도입 후, 로그 포맷만 바꿨는데 WSARecv에서 커널 에러(코드 3)가 발생하는 기현상 발생.
    원인: std::chrono::current_zone()의 오버헤드가 네트워크 루프의 Critical Path를 방해하여 OS가 소켓 핸들 통로를 무효화함.
    해결: git bisect를 통해 범인을 특정하고, 로깅 로직을 경량화하여 타이밍 이슈 해결.

    Environment & Build Stack
    CPU: 8-Core / 16-Logical Processors
    RAM: 32GB
    OS: Windows (MinGW-w64)
    Compiler: GCC (MinGW-w64)
    Optimization: -O3
    C++ Standard: C++20
