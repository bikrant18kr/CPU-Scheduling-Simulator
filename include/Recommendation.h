#pragma once

#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <iostream>
#include "Comparison.h"

struct Recommendation {
    std::string bestOverall;
    std::string bestWaitingTime;
    std::string bestTurnaroundTime;
    std::string bestResponseTime;
    std::string bestFairness;
    std::string leastContextSwitches;
    std::map<std::string, double> scores;  // algorithm name -> normalized score
    std::string explanation;
};

class RecommendationEngine {
public:
    static inline Recommendation recommend(const std::vector<ComparisonEntry>& entries) {
        Recommendation rec;
        if (entries.empty()) return rec;

        double minWT = entries[0].metrics.avgWaitingTime; double maxWT = minWT;
        double minTAT = entries[0].metrics.avgTurnaroundTime; double maxTAT = minTAT;
        double minRT = entries[0].metrics.avgResponseTime; double maxRT = minRT;
        double minCPU = entries[0].metrics.cpuUtilization; double maxCPU = minCPU;
        double minCS = entries[0].metrics.contextSwitches; double maxCS = minCS;
        double minFair = entries[0].metrics.fairnessIndex; double maxFair = minFair;

        rec.bestWaitingTime = entries[0].algorithmName;
        rec.bestTurnaroundTime = entries[0].algorithmName;
        rec.bestResponseTime = entries[0].algorithmName;
        rec.leastContextSwitches = entries[0].algorithmName;
        rec.bestFairness = entries[0].algorithmName;

        for (const auto& e : entries) {
            const auto& m = e.metrics;
            if (m.avgWaitingTime < minWT) { minWT = m.avgWaitingTime; rec.bestWaitingTime = e.algorithmName; }
            if (m.avgWaitingTime > maxWT) { maxWT = m.avgWaitingTime; }
            if (m.avgTurnaroundTime < minTAT) { minTAT = m.avgTurnaroundTime; rec.bestTurnaroundTime = e.algorithmName; }
            if (m.avgTurnaroundTime > maxTAT) { maxTAT = m.avgTurnaroundTime; }
            if (m.avgResponseTime < minRT) { minRT = m.avgResponseTime; rec.bestResponseTime = e.algorithmName; }
            if (m.avgResponseTime > maxRT) { maxRT = m.avgResponseTime; }
            if (m.cpuUtilization < minCPU) { minCPU = m.cpuUtilization; }
            if (m.cpuUtilization > maxCPU) { maxCPU = m.cpuUtilization; }
            if (m.contextSwitches < minCS) { minCS = m.contextSwitches; rec.leastContextSwitches = e.algorithmName; }
            if (m.contextSwitches > maxCS) { maxCS = m.contextSwitches; }
            if (m.fairnessIndex < minFair) { minFair = m.fairnessIndex; rec.bestFairness = e.algorithmName; }
            if (m.fairnessIndex > maxFair) { maxFair = m.fairnessIndex; }
        }

        double bestOverallScore = 1e9;
        
        for (const auto& e : entries) {
            const auto& m = e.metrics;
            auto norm = [](double val, double minV, double maxV) {
                if (maxV == minV) return 0.0;
                return (val - minV) / (maxV - minV);
            };

            double nWT = norm(m.avgWaitingTime, minWT, maxWT);
            double nTAT = norm(m.avgTurnaroundTime, minTAT, maxTAT);
            double nRT = norm(m.avgResponseTime, minRT, maxRT);
            double nCPU = 1.0 - norm(m.cpuUtilization, minCPU, maxCPU); // inverted
            double nCS = norm(m.contextSwitches, minCS, maxCS);
            double nFair = norm(m.fairnessIndex, minFair, maxFair);

            double score = 0.25 * nWT + 0.2 * nTAT + 0.2 * nRT + 0.1 * nCPU + 0.1 * nCS + 0.15 * nFair;
            rec.scores[e.algorithmName] = score;

            if (score < bestOverallScore) {
                bestOverallScore = score;
                rec.bestOverall = e.algorithmName;
            }
        }

        rec.explanation = "The recommendation engine uses a weighted scoring heuristic across multiple metrics.\n"
                          "Lower scores represent better overall performance across standard evaluation criteria.\n"
                          "Weights used: Wait Time (25%), Turnaround (20%), Response (20%), Fairness (15%),\n"
                          "CPU Utilization (10%), Context Switches (10%).\n"
                          "Algorithm '" + rec.bestOverall + "' achieved the lowest (best) heuristic score.";
                          
        return rec;
    }

    static inline void printRecommendation(const Recommendation& rec) {
        std::cout << "\n=========================== RECOMMENDATIONS ===========================\n";
        std::cout << "Category Winners:\n";
        std::cout << "  Best Waiting Time    : " << rec.bestWaitingTime << "\n";
        std::cout << "  Best Turnaround Time : " << rec.bestTurnaroundTime << "\n";
        std::cout << "  Best Response Time   : " << rec.bestResponseTime << "\n";
        std::cout << "  Least Context Switch : " << rec.leastContextSwitches << "\n";
        std::cout << "  Best Fairness        : " << rec.bestFairness << "\n\n";

        std::cout << "Overall Scores (lower is better):\n";
        for (const auto& pair : rec.scores) {
            std::cout << "  " << std::left << std::setw(25) << pair.first << ": " 
                      << std::fixed << std::setprecision(4) << pair.second << "\n";
        }
        std::cout << "\n";
        
        std::cout << "*** BEST OVERALL ALGORITHM: " << rec.bestOverall << " ***\n\n";
        std::cout << "Explanation:\n" << rec.explanation << "\n\n";
        std::cout << "Disclaimer: This is a heuristic recommendation and may not fit specific real-time constraints.\n";
        std::cout << "========================================================================\n\n";
    }
};
