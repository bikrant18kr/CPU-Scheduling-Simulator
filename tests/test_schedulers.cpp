#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <cassert>
#include <sstream>

#include "Process.h"
#include "Scheduler.h"
#include "FCFSScheduler.h"
#include "SPNScheduler.h"
#include "SRTScheduler.h"
#include "RoundRobinScheduler.h"
#include "HRRNScheduler.h"
#include "FeedbackQueueScheduler.h"
#include "Metrics.h"
#include "Comparison.h"
#include "Recommendation.h"

// ============================================================
// Test Framework (lightweight, no external dependencies)
// ============================================================

static int tests_passed = 0;
static int tests_failed = 0;
static int tests_total = 0;

#define TEST(name) \
    static void test_##name(); \
    struct TestRegister_##name { \
        TestRegister_##name() { \
            tests_total++; \
            std::cout << "  TEST " << #name << "... "; \
            try { \
                test_##name(); \
                std::cout << "PASSED" << std::endl; \
                tests_passed++; \
            } catch (const std::exception& e) { \
                std::cout << "FAILED: " << e.what() << std::endl; \
                tests_failed++; \
            } catch (...) { \
                std::cout << "FAILED: unknown exception" << std::endl; \
                tests_failed++; \
            } \
        } \
    } testInstance_##name; \
    static void test_##name()

void check(bool condition, const std::string& msg) {
    if (!condition) throw std::runtime_error(msg);
}

void checkEq(int actual, int expected, const std::string& label) {
    if (actual != expected) {
        throw std::runtime_error(label + ": expected " + std::to_string(expected)
                                 + ", got " + std::to_string(actual));
    }
}

void checkApprox(double actual, double expected, const std::string& label, double eps = 0.01) {
    if (std::abs(actual - expected) > eps) {
        std::ostringstream oss;
        oss << label << ": expected " << expected << ", got " << actual;
        throw std::runtime_error(oss.str());
    }
}

// Helper: find process result by ID
const ProcessResult& findResult(const ScheduleResult& sr, int id) {
    for (const auto& pr : sr.results) {
        if (pr.id == id) return pr;
    }
    throw std::runtime_error("Process ID " + std::to_string(id) + " not found in results");
}

// ============================================================
// Common test data
// ============================================================

// Standard test set from Phase 3 teaching materials:
// P1(AT=0,BT=5), P2(AT=1,BT=3), P3(AT=2,BT=6), P4(AT=4,BT=2)
std::vector<Process> standardProcesses() {
    return { {1,0,5}, {2,1,3}, {3,2,6}, {4,4,2} };
}

// ============================================================
// 1. EMPTY INPUT
// ============================================================
TEST(empty_input_all_algorithms) {
    std::vector<Process> empty;
    FCFSScheduler fcfs;
    SPNScheduler spn;
    SRTScheduler srt;
    RoundRobinScheduler rr(2);
    HRRNScheduler hrrn;
    FeedbackQueueScheduler mlfq({1,2,4});

    auto r1 = fcfs.schedule(empty);
    auto r2 = spn.schedule(empty);
    auto r3 = srt.schedule(empty);
    auto r4 = rr.schedule(empty);
    auto r5 = hrrn.schedule(empty);
    auto r6 = mlfq.schedule(empty);

    check(r1.results.empty(), "FCFS empty");
    check(r2.results.empty(), "SPN empty");
    check(r3.results.empty(), "SRT empty");
    check(r4.results.empty(), "RR empty");
    check(r5.results.empty(), "HRRN empty");
    check(r6.results.empty(), "MLFQ empty");
}

// ============================================================
// 2. SINGLE PROCESS
// ============================================================
TEST(single_process) {
    std::vector<Process> procs = { {1, 0, 5} };

    FCFSScheduler fcfs;
    auto r = fcfs.schedule(procs);

    checkEq((int)r.results.size(), 1, "result count");
    auto& p = r.results[0];
    checkEq(p.completionTime, 5, "CT");
    checkEq(p.turnaroundTime, 5, "TAT");
    checkEq(p.waitingTime, 0, "WT");
    checkEq(p.responseTime, 0, "RT");
    checkEq(r.totalTime, 5, "totalTime");
    checkEq(r.idleTime, 0, "idleTime");
}

TEST(single_process_late_arrival) {
    std::vector<Process> procs = { {1, 3, 4} };

    FCFSScheduler fcfs;
    auto r = fcfs.schedule(procs);

    auto& p = r.results[0];
    checkEq(p.completionTime, 7, "CT");
    checkEq(p.turnaroundTime, 4, "TAT");
    checkEq(p.waitingTime, 0, "WT");
    checkEq(p.responseTime, 0, "RT");
    checkEq(r.idleTime, 3, "idleTime");
    checkEq(r.totalTime, 7, "totalTime");
}

// ============================================================
// 3. FCFS — Manually verified
// ============================================================
// P1(0,5), P2(1,3), P3(2,6), P4(4,2)
// Gantt: |P1(0-5)|P2(5-8)|P3(8-14)|P4(14-16)|
// P1: CT=5,  TAT=5,  WT=0,  RT=0
// P2: CT=8,  TAT=7,  WT=4,  RT=4
// P3: CT=14, TAT=12, WT=6,  RT=6
// P4: CT=16, TAT=12, WT=10, RT=10
TEST(fcfs_standard) {
    auto procs = standardProcesses();
    FCFSScheduler fcfs;
    auto r = fcfs.schedule(procs);

    checkEq((int)r.results.size(), 4, "result count");
    auto p1 = findResult(r, 1);
    auto p2 = findResult(r, 2);
    auto p3 = findResult(r, 3);
    auto p4 = findResult(r, 4);

    checkEq(p1.completionTime, 5, "P1 CT");
    checkEq(p1.turnaroundTime, 5, "P1 TAT");
    checkEq(p1.waitingTime, 0, "P1 WT");
    checkEq(p1.responseTime, 0, "P1 RT");

    checkEq(p2.completionTime, 8, "P2 CT");
    checkEq(p2.turnaroundTime, 7, "P2 TAT");
    checkEq(p2.waitingTime, 4, "P2 WT");

    checkEq(p3.completionTime, 14, "P3 CT");
    checkEq(p3.turnaroundTime, 12, "P3 TAT");
    checkEq(p3.waitingTime, 6, "P3 WT");

    checkEq(p4.completionTime, 16, "P4 CT");
    checkEq(p4.turnaroundTime, 12, "P4 TAT");
    checkEq(p4.waitingTime, 10, "P4 WT");

    checkEq(r.totalTime, 16, "totalTime");
    checkEq(r.idleTime, 0, "idleTime");
}

// ============================================================
// 4. SPN — Manually verified
// ============================================================
// At t=0: only P1 available -> run P1(0-5)
// At t=5: P2(BT=3),P3(BT=6),P4(BT=2) available. Shortest=P4 -> P4(5-7)
// At t=7: P2(BT=3),P3(BT=6). Shortest=P2 -> P2(7-10)
// At t=10: P3 -> P3(10-16)
TEST(spn_standard) {
    auto procs = standardProcesses();
    SPNScheduler spn;
    auto r = spn.schedule(procs);

    auto p1 = findResult(r, 1);
    auto p2 = findResult(r, 2);
    auto p3 = findResult(r, 3);
    auto p4 = findResult(r, 4);

    checkEq(p1.completionTime, 5, "P1 CT");
    checkEq(p4.completionTime, 7, "P4 CT");
    checkEq(p2.completionTime, 10, "P2 CT");
    checkEq(p3.completionTime, 16, "P3 CT");

    checkEq(p4.waitingTime, 1, "P4 WT");
    checkEq(p2.waitingTime, 6, "P2 WT");
    checkEq(p3.waitingTime, 8, "P3 WT");

    checkEq(r.totalTime, 16, "totalTime");
}

// ============================================================
// 5. SRT — Manually verified
// ============================================================
// t=0: P1(rem=5) runs
// t=1: P2(rem=3) arrives. P1 rem=4. 3<4 -> preempt P1, run P2
// t=2: P3(rem=6) arrives. P2 rem=2. 6>2 -> P2 continues
// t=4: P2 finishes(CT=4). P4(rem=2) arrives. Ready: P1(rem=4),P3(rem=6),P4(rem=2). Shortest=P4
// t=6: P4 finishes(CT=6). Ready: P1(rem=4),P3(rem=6). Run P1
// t=10: P1 finishes(CT=10). Run P3
// t=16: P3 finishes(CT=16)
TEST(srt_standard) {
    auto procs = standardProcesses();
    SRTScheduler srt;
    auto r = srt.schedule(procs);

    auto p1 = findResult(r, 1);
    auto p2 = findResult(r, 2);
    auto p3 = findResult(r, 3);
    auto p4 = findResult(r, 4);

    checkEq(p2.completionTime, 4, "P2 CT");
    checkEq(p4.completionTime, 6, "P4 CT");
    checkEq(p1.completionTime, 10, "P1 CT");
    checkEq(p3.completionTime, 16, "P3 CT");

    checkEq(p1.responseTime, 0, "P1 RT");
    checkEq(p2.responseTime, 0, "P2 RT");
    checkEq(p4.responseTime, 0, "P4 RT");

    checkEq(p1.waitingTime, 5, "P1 WT");
    checkEq(p2.waitingTime, 0, "P2 WT");
    checkEq(p4.waitingTime, 0, "P4 WT");

    checkEq(r.totalTime, 16, "totalTime");
}

// ============================================================
// 6. ROUND ROBIN — Manually verified (quantum=2)
// ============================================================
// t=0: RQ=[P1]. Run P1 (0-2, rem=3)
// t=2: P2,P3 arrived. RQ=[P2,P3,P1]. Run P2 (2-4, rem=1)
// t=4: P4 arrived. RQ=[P3,P1,P4,P2]. Run P3 (4-6, rem=4)
// t=6: RQ=[P1,P4,P2,P3]. Run P1 (6-8, rem=1)
// t=8: RQ=[P4,P2,P3,P1]. Run P4 (8-10, rem=0, done CT=10)
// t=10: RQ=[P2,P3,P1]. Run P2 (10-11, rem=0, done CT=11)
// t=11: RQ=[P3,P1]. Run P3 (11-13, rem=2)
// t=13: RQ=[P1,P3]. Run P1 (13-14, rem=0, done CT=14)
// t=14: RQ=[P3]. Run P3 (14-16, rem=0, done CT=16)
TEST(rr_standard_q2) {
    auto procs = standardProcesses();
    RoundRobinScheduler rr(2);
    auto r = rr.schedule(procs);

    auto p1 = findResult(r, 1);
    auto p2 = findResult(r, 2);
    auto p3 = findResult(r, 3);
    auto p4 = findResult(r, 4);

    checkEq(p4.completionTime, 10, "P4 CT");
    checkEq(p2.completionTime, 11, "P2 CT");
    checkEq(p1.completionTime, 14, "P1 CT");
    checkEq(p3.completionTime, 16, "P3 CT");

    // Response times
    checkEq(p1.responseTime, 0, "P1 RT");  // first CPU at t=0
    checkEq(p2.responseTime, 1, "P2 RT");  // first CPU at t=2, arrived t=1
    checkEq(p3.responseTime, 2, "P3 RT");  // first CPU at t=4, arrived t=2
    checkEq(p4.responseTime, 4, "P4 RT");  // first CPU at t=8, arrived t=4

    checkEq(r.totalTime, 16, "totalTime");
}

// ============================================================
// 7. HRRN — Manually verified
// ============================================================
// t=0: Only P1 -> run P1(0-5)
// t=5: P2(waited 4,BT=3), P3(waited 3,BT=6), P4(waited 1,BT=2)
//   RR_P2 = (4+3)/3 = 2.33
//   RR_P3 = (3+6)/6 = 1.5
//   RR_P4 = (1+2)/2 = 1.5
//   Highest = P2 -> run P2(5-8)
// t=8: P3(waited 6,BT=6), P4(waited 4,BT=2)
//   RR_P3 = (6+6)/6 = 2.0
//   RR_P4 = (4+2)/2 = 3.0
//   Highest = P4 -> run P4(8-10)
// t=10: P3 -> run P3(10-16)
TEST(hrrn_standard) {
    auto procs = standardProcesses();
    HRRNScheduler hrrn;
    auto r = hrrn.schedule(procs);

    auto p1 = findResult(r, 1);
    auto p2 = findResult(r, 2);
    auto p3 = findResult(r, 3);
    auto p4 = findResult(r, 4);

    checkEq(p1.completionTime, 5, "P1 CT");
    checkEq(p2.completionTime, 8, "P2 CT");
    checkEq(p4.completionTime, 10, "P4 CT");
    checkEq(p3.completionTime, 16, "P3 CT");

    checkEq(p2.waitingTime, 4, "P2 WT");
    checkEq(p4.waitingTime, 4, "P4 WT");
    checkEq(p3.waitingTime, 8, "P3 WT");
}

// ============================================================
// 8. FEEDBACK QUEUE — Manually verified (quanta = {1, 2, 4})
// ============================================================
// t=0: P1->Q0. Run P1(Q0,q=1) 0-1, rem=4 -> demote to Q1
// t=1: P2->Q0. Q0=[P2]. Run P2(Q0,q=1) 1-2, rem=2 -> Q1
// t=2: P3->Q0. Q0=[P3]. Q1=[P1,P2]. Run P3(Q0,q=1) 2-3, rem=5 -> Q1
// t=3: Q0 empty. Q1=[P1,P2,P3]. Run P1(Q1,q=2) 3-5, rem=2 -> Q2
// t=4 during P1's run: P4->Q0... but no preemption mid-quantum
// t=5: P4->Q0 (arrived at t=4). Q0=[P4]. Run P4(Q0,q=1) 5-6, rem=1 -> Q1
// t=6: Q0 empty. Q1=[P2,P3,P4]. Run P2(Q1,q=2) 6-8, rem=0 CT=8
// t=8: Q1=[P3,P4]. Run P3(Q1,q=2) 8-10, rem=3 -> Q2
// t=10: Q1=[P4]. Run P4(Q1,q=2) -> only needs 1, 10-11, rem=0 CT=11
// t=11: Q2=[P1,P3]. Run P1(Q2,q=4) -> needs 2, 11-13, rem=0 CT=13
// t=13: Q2=[P3]. Run P3(Q2,q=4) -> needs 3, 13-16, rem=0 CT=16
TEST(mlfq_standard) {
    auto procs = standardProcesses();
    FeedbackQueueScheduler mlfq({1, 2, 4});
    auto r = mlfq.schedule(procs);

    auto p1 = findResult(r, 1);
    auto p2 = findResult(r, 2);
    auto p3 = findResult(r, 3);
    auto p4 = findResult(r, 4);

    checkEq(p2.completionTime, 8, "P2 CT");
    checkEq(p4.completionTime, 11, "P4 CT");
    checkEq(p1.completionTime, 13, "P1 CT");
    checkEq(p3.completionTime, 16, "P3 CT");

    // All processes got CPU quickly in Q0
    checkEq(p1.responseTime, 0, "P1 RT");
    checkEq(p2.responseTime, 0, "P2 RT");
    checkEq(p3.responseTime, 0, "P3 RT");

    checkEq(r.totalTime, 16, "totalTime");
}

// ============================================================
// 9. SAME ARRIVAL TIMES
// ============================================================
TEST(same_arrival_times) {
    // All arrive at t=0. FCFS should use ID ordering.
    std::vector<Process> procs = { {1,0,4}, {2,0,3}, {3,0,1} };
    FCFSScheduler fcfs;
    auto r = fcfs.schedule(procs);

    // FCFS: sorted by arrival (all 0), then by ID
    auto p1 = findResult(r, 1);
    auto p2 = findResult(r, 2);
    auto p3 = findResult(r, 3);

    checkEq(p1.completionTime, 4, "P1 CT");
    checkEq(p2.completionTime, 7, "P2 CT");
    checkEq(p3.completionTime, 8, "P3 CT");

    // SPN should pick shortest first
    SPNScheduler spn;
    auto r2 = spn.schedule(procs);
    auto sp3 = findResult(r2, 3);
    auto sp2 = findResult(r2, 2);
    auto sp1 = findResult(r2, 1);

    checkEq(sp3.completionTime, 1, "SPN P3 CT");  // burst=1, runs first
    checkEq(sp2.completionTime, 4, "SPN P2 CT");  // burst=3
    checkEq(sp1.completionTime, 8, "SPN P1 CT");  // burst=4
}

// ============================================================
// 10. CPU IDLE PERIODS
// ============================================================
TEST(cpu_idle_gap) {
    // Gap between P1 finishing and P2 arriving
    std::vector<Process> procs = { {1,0,2}, {2,5,3} };
    FCFSScheduler fcfs;
    auto r = fcfs.schedule(procs);

    checkEq(r.idleTime, 3, "idle time");
    checkEq(r.totalTime, 8, "total time");

    auto p2 = findResult(r, 2);
    checkEq(p2.completionTime, 8, "P2 CT");
    checkEq(p2.waitingTime, 0, "P2 WT");

    // Check timeline has idle entry
    bool foundIdle = false;
    for (const auto& te : r.timeline) {
        if (te.processId == -1 && te.startTime == 2 && te.endTime == 5)
            foundIdle = true;
    }
    check(foundIdle, "Timeline should have idle period 2-5");
}

// ============================================================
// 11. SAME BURST TIMES
// ============================================================
TEST(same_burst_times) {
    std::vector<Process> procs = { {1,0,3}, {2,1,3}, {3,2,3} };

    // SPN: all same burst, should use arrival time tie-break (= FCFS order)
    SPNScheduler spn;
    auto r = spn.schedule(procs);
    auto p1 = findResult(r, 1);
    auto p2 = findResult(r, 2);
    auto p3 = findResult(r, 3);

    checkEq(p1.completionTime, 3, "P1 CT");
    checkEq(p2.completionTime, 6, "P2 CT");
    checkEq(p3.completionTime, 9, "P3 CT");
}

// ============================================================
// 12. ROUND ROBIN — QUANTUM=1 (maximum context switches)
// ============================================================
TEST(rr_quantum_1) {
    std::vector<Process> procs = { {1,0,3}, {2,0,2} };
    RoundRobinScheduler rr(1);
    auto r = rr.schedule(procs);

    // t=0: P1(1), t=1: P2(1), t=2: P1(1), t=3: P2(1), t=4: P1(1)
    auto p1 = findResult(r, 1);
    auto p2 = findResult(r, 2);

    checkEq(p2.completionTime, 4, "P2 CT");
    checkEq(p1.completionTime, 5, "P1 CT");
    checkEq(p1.responseTime, 0, "P1 RT");
    checkEq(p2.responseTime, 1, "P2 RT");
}

// ============================================================
// 13. ROUND ROBIN — LARGE QUANTUM (degenerates to FCFS)
// ============================================================
TEST(rr_large_quantum) {
    auto procs = standardProcesses();
    RoundRobinScheduler rr(100);
    auto r = rr.schedule(procs);

    FCFSScheduler fcfs;
    auto r2 = fcfs.schedule(procs);

    // With quantum larger than all bursts, RR = FCFS
    for (size_t i = 0; i < r.results.size(); i++) {
        checkEq(r.results[i].completionTime, r2.results[i].completionTime,
                "RR(100) vs FCFS CT for P" + std::to_string(r.results[i].id));
    }
}

// ============================================================
// 14. SRT — NO PREEMPTION NEEDED
// ============================================================
TEST(srt_no_preemption) {
    // Processes arrive in order of increasing burst time — no preemption occurs
    std::vector<Process> procs = { {1,0,1}, {2,1,2}, {3,3,3} };
    SRTScheduler srt;
    auto r = srt.schedule(procs);

    auto p1 = findResult(r, 1);
    auto p2 = findResult(r, 2);
    auto p3 = findResult(r, 3);

    checkEq(p1.completionTime, 1, "P1 CT");
    checkEq(p2.completionTime, 3, "P2 CT");
    checkEq(p3.completionTime, 6, "P3 CT");
}

// ============================================================
// 15. METRICS CALCULATION
// ============================================================
TEST(metrics_calculation) {
    auto procs = standardProcesses();
    FCFSScheduler fcfs;
    auto r = fcfs.schedule(procs);
    auto m = Metrics::calculate(r);

    // Avg WT = (0+4+6+10)/4 = 5.0
    checkApprox(m.avgWaitingTime, 5.0, "Avg WT");
    // Avg TAT = (5+7+12+12)/4 = 9.0
    checkApprox(m.avgTurnaroundTime, 9.0, "Avg TAT");
    // CPU Utilization: no idle time, so 100%
    checkApprox(m.cpuUtilization, 100.0, "CPU Util");
    // Throughput: 4 processes / 16 units
    checkApprox(m.throughput, 0.25, "Throughput");
}

// ============================================================
// 16. COMPARISON MODULE
// ============================================================
TEST(comparison_runs_all_six) {
    auto procs = standardProcesses();
    auto entries = Comparison::compareAll(procs, 2, {1, 2, 4});

    checkEq((int)entries.size(), 6, "Should have 6 algorithm results");

    // Verify each has results
    for (const auto& e : entries) {
        check(!e.result.results.empty(), "Algorithm " + e.algorithmName + " has no results");
        check(e.result.totalTime > 0, "Algorithm " + e.algorithmName + " has zero totalTime");
    }
}

// ============================================================
// 17. RECOMMENDATION MODULE
// ============================================================
TEST(recommendation_produces_output) {
    auto procs = standardProcesses();
    auto entries = Comparison::compareAll(procs, 2, {1, 2, 4});
    auto rec = RecommendationEngine::recommend(entries);

    check(!rec.bestOverall.empty(), "bestOverall should not be empty");
    check(!rec.bestWaitingTime.empty(), "bestWaitingTime should not be empty");
    check(!rec.bestResponseTime.empty(), "bestResponseTime should not be empty");
    check(!rec.bestFairness.empty(), "bestFairness should not be empty");
    check(!rec.explanation.empty(), "explanation should not be empty");
    checkEq((int)rec.scores.size(), 6, "Should have scores for all 6 algorithms");
}

// ============================================================
// 18. STARVATION SCENARIO (SPN)
// ============================================================
TEST(starvation_scenario_spn) {
    // Long process P1, many short processes arriving later
    std::vector<Process> procs = { {1,0,20}, {2,1,1}, {3,2,1}, {4,3,1}, {5,4,1} };
    SPNScheduler spn;
    auto r = spn.schedule(procs);

    auto p1 = findResult(r, 1);
    // P1 runs first (only one at t=0), finishes at 20.
    // Then short ones: P2(20-21), P3(21-22), P4(22-23), P5(23-24)
    // Actually P1 runs to completion first since it started at t=0
    checkEq(p1.completionTime, 20, "P1 CT");
    // But if P1 arrived later, it would starve. Let's test that:

    std::vector<Process> procs2 = { {1,0,1}, {2,0,20}, {3,1,1}, {4,2,1}, {5,3,1} };
    auto r2 = spn.schedule(procs2);
    auto p2_r = findResult(r2, 2);
    // P2(BT=20) should run last because SPN always picks shorter ones
    checkEq(p2_r.completionTime, 24, "P2 starved - finishes last");
    checkEq(p2_r.waitingTime, 4, "P2 waited while short ones ran");
}

// ============================================================
// 19. PROCESSES ARRIVING DURING EXECUTION
// ============================================================
TEST(arrivals_during_execution) {
    std::vector<Process> procs = { {1,0,10}, {2,3,2}, {3,5,1} };

    SRTScheduler srt;
    auto r = srt.schedule(procs);

    // t=0: P1 starts (rem=10)
    // t=3: P2 arrives (rem=2). P1 rem=7. 2<7, preempt. Run P2.
    // t=5: P2 done (CT=5). P3 arrives (rem=1). P1 rem=7. 1<7, run P3.
    // t=6: P3 done (CT=6). Run P1.
    // t=13: P1 done (CT=13).
    auto p1 = findResult(r, 1);
    auto p2 = findResult(r, 2);
    auto p3 = findResult(r, 3);

    checkEq(p2.completionTime, 5, "P2 CT");
    checkEq(p3.completionTime, 6, "P3 CT");
    checkEq(p1.completionTime, 13, "P1 CT");
}

// ============================================================
// 20. LARGE WORKLOAD
// ============================================================
TEST(large_workload) {
    std::vector<Process> procs;
    for (int i = 1; i <= 50; i++) {
        procs.emplace_back(i, i - 1, (i % 5) + 1);
    }

    FCFSScheduler fcfs;
    auto r = fcfs.schedule(procs);
    checkEq((int)r.results.size(), 50, "Should have 50 results");

    // All processes should have positive turnaround time
    for (const auto& pr : r.results) {
        check(pr.turnaroundTime > 0, "TAT should be > 0 for P" + std::to_string(pr.id));
        check(pr.waitingTime >= 0, "WT should be >= 0 for P" + std::to_string(pr.id));
    }

    // Run all algorithms on large workload - shouldn't crash
    auto entries = Comparison::compareAll(procs, 3, {1, 2, 4});
    checkEq((int)entries.size(), 6, "All 6 algorithms should complete");
}

// ============================================================
// 21. GANTT CHART INTEGRITY
// ============================================================
TEST(gantt_chart_integrity) {
    auto procs = standardProcesses();

    FCFSScheduler fcfs;
    auto r = fcfs.schedule(procs);

    // Timeline should be contiguous — no gaps or overlaps
    for (size_t i = 1; i < r.timeline.size(); i++) {
        checkEq(r.timeline[i].startTime, r.timeline[i-1].endTime,
                "Timeline gap at entry " + std::to_string(i));
    }

    // First entry should start at 0
    check(!r.timeline.empty(), "Timeline should not be empty");
    checkEq(r.timeline.front().startTime, 0, "Timeline should start at 0");

    // Last entry should end at totalTime
    checkEq(r.timeline.back().endTime, r.totalTime, "Timeline should end at totalTime");
}

// ============================================================
// 22. ALGORITHM NAMES
// ============================================================
TEST(algorithm_names) {
    FCFSScheduler fcfs;
    SPNScheduler spn;
    SRTScheduler srt;
    RoundRobinScheduler rr(3);
    HRRNScheduler hrrn;
    FeedbackQueueScheduler mlfq({1, 2, 4});

    check(fcfs.getName() == "FCFS", "FCFS name");
    check(spn.getName() == "SPN", "SPN name");
    check(srt.getName() == "SRT", "SRT name");
    check(rr.getName() == "Round Robin (q=3)", "RR name");
    check(mlfq.getName() == "Feedback Queue (q=1,2,4)", "MLFQ name");
}

// ============================================================
// 23. TURNAROUND = WAITING + BURST (invariant)
// ============================================================
TEST(tat_equals_wt_plus_bt) {
    auto procs = standardProcesses();
    const Scheduler* schedulers[] = {
        new FCFSScheduler(),
        new SPNScheduler(),
        new SRTScheduler(),
        new RoundRobinScheduler(2),
        new HRRNScheduler(),
        new FeedbackQueueScheduler({1, 2, 4})
    };

    for (const auto* s : schedulers) {
        auto r = s->schedule(procs);
        for (const auto& pr : r.results) {
            checkEq(pr.turnaroundTime, pr.waitingTime + pr.burstTime,
                    s->getName() + " P" + std::to_string(pr.id) + " TAT != WT + BT");
        }
        delete s;
    }
}

// ============================================================
// MAIN
// ============================================================
int main() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "  CPU Scheduling Simulator - Test Suite" << std::endl;
    std::cout << "========================================\n" << std::endl;

    // Tests run automatically via static initialization

    std::cout << "\n========================================" << std::endl;
    std::cout << "  Results: " << tests_passed << "/" << tests_total << " passed, "
              << tests_failed << " failed" << std::endl;
    std::cout << "========================================\n" << std::endl;

    return tests_failed > 0 ? 1 : 0;
}
