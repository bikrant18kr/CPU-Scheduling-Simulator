#pragma once

#include "Scheduler.h"
#include <vector>
#include <algorithm>
#include <string>
#include <queue>

class SPNScheduler : public Scheduler {
public:
    std::string getName() const override {
        return "SPN";
    }

    ScheduleResult schedule(const std::vector<Process>& processes) const override {
        ScheduleResult result;
        result.algorithmName = getName();

        if (processes.empty()) return result;

        std::vector<Process> sorted_procs = processes;
        std::sort(sorted_procs.begin(), sorted_procs.end(), [](const Process& a, const Process& b) {
            if (a.arrivalTime != b.arrivalTime) return a.arrivalTime < b.arrivalTime;
            return a.id < b.id;
        });

        auto comp = [](const Process& a, const Process& b) {
            if (a.burstTime != b.burstTime) return a.burstTime > b.burstTime;
            if (a.arrivalTime != b.arrivalTime) return a.arrivalTime > b.arrivalTime;
            return a.id > b.id;
        };
        std::priority_queue<Process, std::vector<Process>, decltype(comp)> ready_queue(comp);

        int currentTime = 0;
        size_t completed = 0;
        size_t next_proc_idx = 0;
        int lastProcId = -2;

        std::vector<int> completionTimes(processes.size(), 0);
        std::vector<int> firstCpuTimes(processes.size(), 0);

        while (completed < processes.size()) {
            while (next_proc_idx < sorted_procs.size() && sorted_procs[next_proc_idx].arrivalTime <= currentTime) {
                ready_queue.push(sorted_procs[next_proc_idx]);
                next_proc_idx++;
            }

            if (ready_queue.empty()) {
                int nextTime = sorted_procs[next_proc_idx].arrivalTime;
                Scheduler::addTimelineEntry(result.timeline, -1, currentTime, nextTime);
                result.idleTime += (nextTime - currentTime);
                currentTime = nextTime;
                lastProcId = -1;
                continue;
            }

            Process p = ready_queue.top();
            ready_queue.pop();

            if (lastProcId != p.id && lastProcId != -2 && lastProcId != -1) {
                result.contextSwitches++;
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
            completed++;
        }

        result.totalTime = currentTime;
        result.results = Scheduler::computeResults(processes, completionTimes, firstCpuTimes);

        return result;
    }
};
