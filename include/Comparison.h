#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include "Metrics.h"
#include "Process.h"
#include "Scheduler.h"
#include "FCFSScheduler.h"
#include "SPNScheduler.h"
#include "SRTScheduler.h"
#include "RoundRobinScheduler.h"
#include "HRRNScheduler.h"
#include "FeedbackQueueScheduler.h"

struct ComparisonEntry {
    std::string algorithmName;
    AggregateMetrics metrics;
    ScheduleResult result;
};

class Comparison {
public:
    static inline std::vector<ComparisonEntry> compareAll(
        const std::vector<Process>& processes,
        int rrQuantum = 2,
        std::vector<int> mlfqQuanta = {1, 2, 4}) 
    {
        std::vector<ComparisonEntry> entries;

        FCFSScheduler fcfs;
        SPNScheduler spn;
        SRTScheduler srt;
        RoundRobinScheduler rr(rrQuantum);
        HRRNScheduler hrrn;
        FeedbackQueueScheduler mlfq(mlfqQuanta);

        const Scheduler* schedulers[] = {&fcfs, &spn, &srt, &rr, &hrrn, &mlfq};

        for (const auto* sched : schedulers) {
            ScheduleResult res = sched->schedule(processes);
            AggregateMetrics metrics = Metrics::calculate(res);
            entries.push_back({sched->getName(), metrics, res});
        }

        return entries;
    }

    static inline void printComparisonTable(const std::vector<ComparisonEntry>& entries) {
        std::cout << "\n========================================= COMPARISON TABLE =========================================\n";
        std::cout << std::left 
                  << std::setw(25) << "Algorithm" 
                  << std::setw(10) << "Avg WT" 
                  << std::setw(10) << "Avg TAT" 
                  << std::setw(10) << "Avg RT" 
                  << std::setw(12) << "CPU Util(%)" 
                  << std::setw(15) << "Ctx Switches" 
                  << std::setw(10) << "Fairness" << "\n";
        std::cout << std::string(92, '-') << "\n";

        for (const auto& e : entries) {
            std::cout << std::left
                      << std::setw(25) << e.algorithmName
                      << std::fixed << std::setprecision(2)
                      << std::setw(10) << e.metrics.avgWaitingTime
                      << std::setw(10) << e.metrics.avgTurnaroundTime
                      << std::setw(10) << e.metrics.avgResponseTime
                      << std::setw(12) << e.metrics.cpuUtilization
                      << std::setw(15) << e.metrics.contextSwitches
                      << std::setw(10) << e.metrics.fairnessIndex << "\n";
        }
        std::cout << "====================================================================================================\n\n";
    }
};
