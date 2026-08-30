#pragma once

#include "Scheduler.h"
#include <vector>
#include <queue>
#include <algorithm>
#include <string>
#include <sstream>

class FeedbackQueueScheduler : public Scheduler {
private:
    std::vector<int> quanta;

public:
    explicit FeedbackQueueScheduler(const std::vector<int>& quanta) : quanta(quanta) {
        if (this->quanta.empty()) {
            this->quanta.push_back(1);
        }
    }

    std::string getName() const override {
        std::ostringstream oss;
        oss << "Feedback Queue (q=";
        for (size_t i = 0; i < quanta.size(); ++i) {
            oss << quanta[i];
            if (i < quanta.size() - 1) oss << ",";
        }
        oss << ")";
        return oss.str();
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
        
        std::vector<std::queue<int>> queues(quanta.size());
        
        int currentTime = 0;
        int completed = 0;
        
        auto getNewArrivals = [&](int limitTime) {
            std::vector<int> newArr;
            for (int i = 0; i < n; ++i) {
                if (!arrived[i] && processes[i].arrivalTime <= limitTime) {
                    newArr.push_back(i);
                }
            }
            std::sort(newArr.begin(), newArr.end(), [&](int a, int b) {
                if(processes[a].arrivalTime != processes[b].arrivalTime)
                    return processes[a].arrivalTime < processes[b].arrivalTime;
                return processes[a].id < processes[b].id;
            });
            for (int idx : newArr) {
                queues[0].push(idx);
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
            int selectedQueue = -1;
            for (size_t i = 0; i < queues.size(); ++i) {
                if (!queues[i].empty()) {
                    selectedQueue = i;
                    break;
                }
            }
            
            if (selectedQueue == -1) {
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
            
            int pIdx = queues[selectedQueue].front();
            queues[selectedQueue].pop();
            
            if (firstCpuTimes[pIdx] == -1) {
                firstCpuTimes[pIdx] = currentTime;
            }
            
            if (lastProcess != -1 && lastProcess != processes[pIdx].id) {
                result.contextSwitches++;
            }
            lastProcess = processes[pIdx].id;
            
            int quantum = quanta[selectedQueue];
            int runTime = std::min(quantum, remainingTime[pIdx]);
            
            addTimelineEntry(result.timeline, processes[pIdx].id, currentTime, currentTime + runTime);
            
            currentTime += runTime;
            remainingTime[pIdx] -= runTime;
            
            getNewArrivals(currentTime);
            
            if (remainingTime[pIdx] > 0) {
                int nextQueue = selectedQueue + 1;
                if (nextQueue >= (int)queues.size()) {
                    nextQueue = queues.size() - 1;
                }
                queues[nextQueue].push(pIdx);
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
