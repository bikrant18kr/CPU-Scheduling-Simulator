#pragma once

#include "Scheduler.h"
#include <vector>
#include <algorithm>
#include <string>

class FCFSScheduler : public Scheduler {
public:
    std::string getName() const override {
        return "FCFS";
    }

    ScheduleResult schedule(const std::vector<Process>& processes) const override {
        ScheduleResult result;
        result.algorithmName = getName();

        if (processes.empty()) return result;

        std::vector<Process> procs = processes;
        std::sort(procs.begin(), procs.end(), [](const Process& a, const Process& b) {
            if (a.arrivalTime != b.arrivalTime)
                return a.arrivalTime < b.arrivalTime;
            return a.id < b.id;
        });

        int currentTime = 0;
        int lastProcId = -2;

        std::vector<int> completionTimes(processes.size(), 0);
        std::vector<int> firstCpuTimes(processes.size(), 0);

        for (const auto& p : procs) {
            if (currentTime < p.arrivalTime) {
                Scheduler::addTimelineEntry(result.timeline, -1, currentTime, p.arrivalTime);
                result.idleTime += (p.arrivalTime - currentTime);
                currentTime = p.arrivalTime;
                lastProcId = -1;
            }

            if (lastProcId != p.id) {
                if (lastProcId != -2 && lastProcId != -1) {
                    result.contextSwitches++;
                }
            }

            int origIdx = -1;
            for (size_t i = 0; i < processes.size(); ++i) {
                if (processes[i].id == p.id) {
                    origIdx = static_cast<int>(i);
                    break;
                }
            }

            firstCpuTimes[origIdx] = currentTime;
            Scheduler::addTimelineEntry(result.timeline, p.id, currentTime, currentTime + p.burstTime);
            currentTime += p.burstTime;
            completionTimes[origIdx] = currentTime;
            
            lastProcId = p.id;
        }

        result.totalTime = currentTime;
        result.results = Scheduler::computeResults(processes, completionTimes, firstCpuTimes);
        
        return result;
    }
};
