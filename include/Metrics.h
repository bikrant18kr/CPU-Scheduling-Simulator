#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <cmath>
#include "Scheduler.h" 

struct AggregateMetrics {
    double avgWaitingTime;
    double avgTurnaroundTime;
    double avgResponseTime;
    double cpuUtilization;   // percentage 0-100
    double throughput;       // processes per unit time
    int contextSwitches;
    double fairnessIndex;    // lower = fairer. Use std deviation of waiting times.
};

class Metrics {
public:
    static inline AggregateMetrics calculate(const ScheduleResult& result) {
        AggregateMetrics metrics = {0, 0, 0, 0, 0, 0, 0};
        if (result.results.empty()) return metrics;

        double totalWait = 0, totalTurnaround = 0, totalResponse = 0;
        for (const auto& p : result.results) {
            totalWait += p.waitingTime;
            totalTurnaround += p.turnaroundTime;
            totalResponse += p.responseTime;
        }

        size_t n = result.results.size();
        metrics.avgWaitingTime = totalWait / n;
        metrics.avgTurnaroundTime = totalTurnaround / n;
        metrics.avgResponseTime = totalResponse / n;

        double varianceSum = 0;
        for (const auto& p : result.results) {
            double diff = p.waitingTime - metrics.avgWaitingTime;
            varianceSum += diff * diff;
        }
        metrics.fairnessIndex = std::sqrt(varianceSum / n);

        metrics.contextSwitches = result.contextSwitches;

        if (result.totalTime > 0) {
            metrics.throughput = static_cast<double>(n) / result.totalTime;
            metrics.cpuUtilization = 100.0 * (result.totalTime - result.idleTime) / result.totalTime;
        }

        return metrics;
    }

    static inline void printProcessTable(const ScheduleResult& result) {
        std::cout << "\n--- Process Table (" << result.algorithmName << ") ---\n";
        std::cout << std::left 
                  << std::setw(5) << "PID" 
                  << std::setw(10) << "Arrival" 
                  << std::setw(10) << "Burst" 
                  << std::setw(12) << "Completion" 
                  << std::setw(12) << "Turnaround" 
                  << std::setw(10) << "Waiting" 
                  << std::setw(10) << "Response" << "\n";
        std::cout << std::string(69, '-') << "\n";

        for (const auto& p : result.results) {
            std::cout << std::left
                      << std::setw(5) << p.id
                      << std::setw(10) << p.arrivalTime
                      << std::setw(10) << p.burstTime
                      << std::setw(12) << p.completionTime
                      << std::setw(12) << p.turnaroundTime
                      << std::setw(10) << p.waitingTime
                      << std::setw(10) << p.responseTime << "\n";
        }
        std::cout << "\n";
    }

    static inline void printAggregateMetrics(const AggregateMetrics& metrics) {
        std::cout << "--- Aggregate Metrics ---\n";
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Avg Waiting Time    : " << metrics.avgWaitingTime << "\n";
        std::cout << "Avg Turnaround Time : " << metrics.avgTurnaroundTime << "\n";
        std::cout << "Avg Response Time   : " << metrics.avgResponseTime << "\n";
        std::cout << "CPU Utilization     : " << metrics.cpuUtilization << "%\n";
        std::cout << "Throughput          : " << metrics.throughput << " proc/unit\n";
        std::cout << "Context Switches    : " << metrics.contextSwitches << "\n";
        std::cout << "Fairness (StdDev WT): " << metrics.fairnessIndex << "\n\n";
    }

    static inline void printGanttChart(const ScheduleResult& result) {
        std::cout << "--- Gantt Chart (" << result.algorithmName << ") ---\n";
        if (result.timeline.empty()) {
            std::cout << "Empty timeline.\n\n";
            return;
        }

        std::string bars = "|";
        std::string times = std::to_string(result.timeline.front().startTime);
        
        for (const auto& entry : result.timeline) {
            std::string pStr;
            if (entry.processId == -1) {
                pStr = "IDLE";
            } else {
                pStr = "P" + std::to_string(entry.processId);
            }
            
            int padding = std::max(1, 4 - (int)pStr.length() / 2);
            bars += std::string(padding, '-') + pStr + std::string(padding, '-') + "|";
            
            std::string tStr = std::to_string(entry.endTime);
            int spacesNeeded = bars.length() - times.length() - tStr.length() + 1;
            if (spacesNeeded < 1) spacesNeeded = 1;
            
            times += std::string(spacesNeeded, ' ') + tStr;
        }

        std::cout << bars << "\n" << times << "\n\n";
    }
};
