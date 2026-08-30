#pragma once

#include "Scheduler.h"
#include <vector>
#include <algorithm>
#include <string>

class HRRNScheduler : public Scheduler {
public:
    HRRNScheduler() = default;

    std::string getName() const override {
        return "HRRN";
    }

    ScheduleResult schedule(const std::vector<Process>& processes) const override {
        ScheduleResult result;
        result.algorithmName = getName();
        
        if (processes.empty()) return result;

        int n = processes.size();
        std::vector<int> completionTimes(n, 0);
        std::vector<int> firstCpuTimes(n, -1);
        std::vector<bool> done(n, false);
        
        int currentTime = 0;
        int completed = 0;
        
        int minArrival = processes[0].arrivalTime;
        for (const auto& p : processes) {
            if (p.arrivalTime < minArrival) minArrival = p.arrivalTime;
        }
        
        if (minArrival > 0) {
            currentTime = minArrival;
            addTimelineEntry(result.timeline, -1, 0, currentTime);
            result.idleTime += currentTime;
        }

        int lastProcessId = -1;

        while (completed < n) {
            int selectedIdx = -1;
            double maxRR = -1.0;
            
            for (int i = 0; i < n; ++i) {
                if (!done[i] && processes[i].arrivalTime <= currentTime) {
                    double waitTime = currentTime - processes[i].arrivalTime;
                    double rr = (waitTime + processes[i].burstTime) / (double)processes[i].burstTime;
                    
                    bool better = false;
                    if (selectedIdx == -1) {
                        better = true;
                    } else if (rr > maxRR) {
                        better = true;
                    } else if (rr == maxRR) {
                        if (processes[i].arrivalTime < processes[selectedIdx].arrivalTime) {
                            better = true;
                        } else if (processes[i].arrivalTime == processes[selectedIdx].arrivalTime) {
                            if (processes[i].id < processes[selectedIdx].id) {
                                better = true;
                            }
                        }
                    }
                    
                    if (better) {
                        maxRR = rr;
                        selectedIdx = i;
                    }
                }
            }
            
            if (selectedIdx == -1) {
                int nextArrival = -1;
                for (int i = 0; i < n; ++i) {
                    if (!done[i]) {
                        if (nextArrival == -1 || processes[i].arrivalTime < nextArrival) {
                            nextArrival = processes[i].arrivalTime;
                        }
                    }
                }
                
                if (nextArrival != -1) {
                    addTimelineEntry(result.timeline, -1, currentTime, nextArrival);
                    result.idleTime += (nextArrival - currentTime);
                    currentTime = nextArrival;
                }
                continue;
            }
            
            firstCpuTimes[selectedIdx] = currentTime;
            
            if (lastProcessId != -1 && lastProcessId != processes[selectedIdx].id) {
                result.contextSwitches++;
            }
            lastProcessId = processes[selectedIdx].id;

            int runTime = processes[selectedIdx].burstTime;
            addTimelineEntry(result.timeline, processes[selectedIdx].id, currentTime, currentTime + runTime);
            
            currentTime += runTime;
            
            completionTimes[selectedIdx] = currentTime;
            done[selectedIdx] = true;
            completed++;
        }

        result.totalTime = currentTime;
        result.results = computeResults(processes, completionTimes, firstCpuTimes);
        return result;
    }
};
