#pragma once

#include "Scheduler.h"
#include <vector>
#include <queue>
#include <algorithm>
#include <string>

class RoundRobinScheduler : public Scheduler {
private:
    int timeQuantum;

public:
    explicit RoundRobinScheduler(int quantum) : timeQuantum(quantum) {}

    std::string getName() const override {
        return "Round Robin (q=" + std::to_string(timeQuantum) + ")";
    }

    ScheduleResult schedule(const std::vector<Process>& processes) const override {
        ScheduleResult result;
        result.algorithmName = getName();
        
        if (processes.empty()) return result;

        int n = processes.size();
        std::vector<int> remainingTime(n);
        for (int i = 0; i < n; ++i) {
            remainingTime[i] = processes[i].burstTime;
        }

        std::vector<int> completionTimes(n, 0);
        std::vector<int> firstCpuTimes(n, -1);
        std::vector<bool> arrived(n, false);
        
        int currentTime = 0;
        int completed = 0;
        
        std::queue<int> readyQueue;
        
        auto getNewArrivals = [&](int limitTime) {
            std::vector<int> newArr;
            for (int i = 0; i < n; ++i) {
                if (!arrived[i] && processes[i].arrivalTime <= limitTime) {
                    newArr.push_back(i);
                }
            }
            std::sort(newArr.begin(), newArr.end(), [&](int a, int b) {
                if (processes[a].arrivalTime != processes[b].arrivalTime)
                    return processes[a].arrivalTime < processes[b].arrivalTime;
                return processes[a].id < processes[b].id;
            });
            for (int idx : newArr) {
                readyQueue.push(idx);
                arrived[idx] = true;
            }
        };

        int minArrival = processes[0].arrivalTime;
        for (const auto& p : processes) {
            if (p.arrivalTime < minArrival) minArrival = p.arrivalTime;
        }
        
        if (minArrival > 0) {
            currentTime = minArrival;
            addTimelineEntry(result.timeline, -1, 0, currentTime);
            result.idleTime += currentTime;
        }

        getNewArrivals(currentTime);

        int lastProcess = -1;

        while (completed < n) {
            if (readyQueue.empty()) {
                int nextArrival = -1;
                for (int i = 0; i < n; ++i) {
                    if (!arrived[i]) {
                        if (nextArrival == -1 || processes[i].arrivalTime < nextArrival) {
                            nextArrival = processes[i].arrivalTime;
                        }
                    }
                }
                
                if (nextArrival != -1) {
                    addTimelineEntry(result.timeline, -1, currentTime, nextArrival);
                    result.idleTime += (nextArrival - currentTime);
                    currentTime = nextArrival;
                    getNewArrivals(currentTime);
                }
                continue;
            }

            int pIdx = readyQueue.front();
            readyQueue.pop();

            if (firstCpuTimes[pIdx] == -1) {
                firstCpuTimes[pIdx] = currentTime;
            }

            if (lastProcess != -1 && lastProcess != processes[pIdx].id) {
                result.contextSwitches++;
            }
            lastProcess = processes[pIdx].id;

            int runTime = std::min(timeQuantum, remainingTime[pIdx]);
            addTimelineEntry(result.timeline, processes[pIdx].id, currentTime, currentTime + runTime);
            
            currentTime += runTime;
            remainingTime[pIdx] -= runTime;

            getNewArrivals(currentTime);

            if (remainingTime[pIdx] > 0) {
                readyQueue.push(pIdx);
            } else {
                completionTimes[pIdx] = currentTime;
                completed++;
            }
        }

        result.totalTime = currentTime;
        result.results = computeResults(processes, completionTimes, firstCpuTimes);
        return result;
    }
};
