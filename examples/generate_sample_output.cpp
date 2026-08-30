#include <iostream>
#include <vector>
#include "Process.h"
#include "Metrics.h"
#include "Comparison.h"
#include "Recommendation.h"

int main() {
    std::vector<Process> processes = {
        {1, 0, 6}, {2, 1, 4}, {3, 2, 2}, {4, 3, 3}, {5, 5, 5}
    };

    std::cout << "========================================\n";
    std::cout << "  CPU Scheduling Simulator\n";
    std::cout << "========================================\n\n";
    std::cout << "Sample Processes:\n";
    std::cout << "  P1(Arrival=0, Burst=6)\n";
    std::cout << "  P2(Arrival=1, Burst=4)\n";
    std::cout << "  P3(Arrival=2, Burst=2)\n";
    std::cout << "  P4(Arrival=3, Burst=3)\n";
    std::cout << "  P5(Arrival=5, Burst=5)\n\n";

    // Run single algorithm example
    FCFSScheduler fcfs;
    auto r = fcfs.schedule(processes);
    Metrics::printGanttChart(r);
    Metrics::printProcessTable(r);
    Metrics::printAggregateMetrics(Metrics::calculate(r));

    // Comparison
    auto entries = Comparison::compareAll(processes, 2, {1, 2, 4});
    Comparison::printComparisonTable(entries);

    // Recommendation
    auto rec = RecommendationEngine::recommend(entries);
    RecommendationEngine::printRecommendation(rec);

    return 0;
}
