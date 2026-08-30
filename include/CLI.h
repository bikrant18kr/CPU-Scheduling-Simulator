#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <limits>
#include "Process.h"
#include "Metrics.h"
#include "Comparison.h"
#include "Recommendation.h"
#include "FCFSScheduler.h"
#include "SPNScheduler.h"
#include "SRTScheduler.h"
#include "RoundRobinScheduler.h"
#include "HRRNScheduler.h"
#include "FeedbackQueueScheduler.h"

class CLI {
public:
    inline void run() {
        printHeader("CPU Scheduling Simulator");
        
        std::cout << "Select Input Method:\n"
                  << "1. Manual Entry\n"
                  << "2. Use Sample Dataset\n"
                  << "Choice: ";
        int choice;
        std::cin >> choice;

        std::vector<Process> processes;
        if (choice == 1) {
            processes = getProcessesFromUser();
        } else {
            processes = getSampleProcesses();
        }

        bool running = true;
        while (running) {
            displayMenu();
            int menuChoice;
            std::cin >> menuChoice;

            switch (menuChoice) {
                case 1:
                    runSingleAlgorithm(processes);
                    break;
                case 2:
                    runComparison(processes);
                    break;
                case 3:
                {
                    int rrQ = getQuantumFromUser();
                    std::vector<int> mlfqQ = getMlfqQuantaFromUser();
                    auto entries = Comparison::compareAll(processes, rrQ, mlfqQ);
                    auto rec = RecommendationEngine::recommend(entries);
                    RecommendationEngine::printRecommendation(rec);
                    break;
                }
                case 4:
                {
                    std::cout << "1. Manual Entry\n2. Use Sample Dataset\nChoice: ";
                    int c; std::cin >> c;
                    if (c == 1) processes = getProcessesFromUser();
                    else processes = getSampleProcesses();
                    break;
                }
                case 5:
                    running = false;
                    std::cout << "Exiting...\n";
                    break;
                default:
                    std::cout << "Invalid choice. Try again.\n";
            }
        }
    }

private:
    inline std::vector<Process> getProcessesFromUser() {
        printHeader("Process Entry");
        int count;
        std::cout << "Enter number of processes: ";
        while (!(std::cin >> count) || count <= 0) {
            std::cout << "Please enter a valid positive number: ";
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }

        std::vector<Process> procs;
        for (int i = 1; i <= count; ++i) {
            int arrival, burst;
            std::cout << "Process P" << i << " Arrival Time: ";
            while (!(std::cin >> arrival) || arrival < 0) {
                std::cout << "Invalid. Arrival time >= 0: ";
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }
            std::cout << "Process P" << i << " Burst Time: ";
            while (!(std::cin >> burst) || burst <= 0) {
                std::cout << "Invalid. Burst time > 0: ";
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }
            procs.emplace_back(i, arrival, burst);
        }
        return procs;
    }

    inline std::vector<Process> getSampleProcesses() {
        return {
            Process(1, 0, 6),
            Process(2, 1, 4),
            Process(3, 2, 2),
            Process(4, 3, 3),
            Process(5, 5, 5)
        };
    }

    inline int getQuantumFromUser() {
        int q;
        std::cout << "Enter Round Robin Quantum: ";
        while (!(std::cin >> q) || q <= 0) {
            std::cout << "Please enter a positive integer: ";
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
        return q;
    }

    inline std::vector<int> getMlfqQuantaFromUser() {
        std::vector<int> quanta;
        int count;
        std::cout << "Enter number of queues for MLFQ: ";
        while (!(std::cin >> count) || count <= 0) {
            std::cout << "Positive integer required: ";
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
        for (int i = 1; i <= count; ++i) {
            int q;
            std::cout << "Queue " << i << " Quantum: ";
            while (!(std::cin >> q) || q <= 0) {
                std::cout << "Positive integer required: ";
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }
            quanta.push_back(q);
        }
        return quanta;
    }

    inline void runSingleAlgorithm(const std::vector<Process>& processes) {
        printHeader("Run Algorithm");
        std::cout << "1. FCFS\n2. SPN\n3. SRT\n4. Round Robin\n5. HRRN\n6. Multilevel Feedback Queue\nChoice: ";
        int choice;
        std::cin >> choice;

        ScheduleResult res;
        if (choice == 1) {
            FCFSScheduler sched; res = sched.schedule(processes);
        } else if (choice == 2) {
            SPNScheduler sched; res = sched.schedule(processes);
        } else if (choice == 3) {
            SRTScheduler sched; res = sched.schedule(processes);
        } else if (choice == 4) {
            int q = getQuantumFromUser();
            RoundRobinScheduler sched(q); res = sched.schedule(processes);
        } else if (choice == 5) {
            HRRNScheduler sched; res = sched.schedule(processes);
        } else if (choice == 6) {
            auto quanta = getMlfqQuantaFromUser();
            FeedbackQueueScheduler sched(quanta); res = sched.schedule(processes);
        } else {
            std::cout << "Invalid choice.\n";
            return;
        }

        Metrics::printGanttChart(res);
        Metrics::printProcessTable(res);
        Metrics::printAggregateMetrics(Metrics::calculate(res));
    }

    inline void runComparison(const std::vector<Process>& processes) {
        printHeader("Compare Algorithms");
        int rrQ = getQuantumFromUser();
        std::vector<int> mlfqQ = getMlfqQuantaFromUser();
        auto entries = Comparison::compareAll(processes, rrQ, mlfqQ);
        Comparison::printComparisonTable(entries);
    }

    inline void displayMenu() {
        std::cout << "\n=== MAIN MENU ===\n"
                  << "1. Run a specific algorithm\n"
                  << "2. Compare all algorithms\n"
                  << "3. Get recommendation\n"
                  << "4. Change processes\n"
                  << "5. Exit\n"
                  << "Enter choice: ";
    }

    inline void printHeader(const std::string& title) {
        std::cout << "\n========================================\n"
                  << "  " << title << "\n"
                  << "========================================\n";
    }
};
