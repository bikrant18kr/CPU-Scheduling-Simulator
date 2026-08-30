#pragma once

#include "Scheduler.h"
#include <vector>
#include <algorithm>
#include <string>

class SRTScheduler : public Scheduler {
public:
    std::string getName() const override {
        return "SRT";
    }

    ScheduleResult schedule(const std::vector<Process>& processes) const override {
        ScheduleResult result;
        result.algorithmName = getName();

        if (processes.empty()) return result;

        struct SRTProcess {
            Process p;
            int remainingTime;
            int origIdx;
        };

        std::vector<SRTProcess> procs;
        for (size_t i = 0; i < processes.size(); ++i) {
            procs.push_back({processes[i], processes[i].burstTime, static_cast<int>(i)});
        }

        std::sort(procs.begin(), procs.end(), [](const SRTProcess& a, const SRTProcess& b) {
            if (a.p.arrivalTime != b.p.arrivalTime) return a.p.arrivalTime < b.p.arrivalTime;
            return a.p.id < b.p.id;
        });

        int currentTime = 0;
        size_t completed = 0;
        size_t next_arrival_idx = 0;
        int current_running_idx = -1;
        int last_running_id = -2;

        std::vector<int> completionTimes(processes.size(), 0);
        std::vector<int> firstCpuTimes(processes.size(), -1);
        std::vector<bool> is_completed(procs.size(), false);

        int segment_start_time = 0;

        while (completed < procs.size()) {
            int next_event_time = -1;
            
            if (current_running_idx != -1) {
                next_event_time = currentTime + procs[current_running_idx].remainingTime;
            }

            if (next_arrival_idx < procs.size()) {
                if (next_event_time == -1 || procs[next_arrival_idx].p.arrivalTime < next_event_time) {
                    next_event_time = procs[next_arrival_idx].p.arrivalTime;
                }
            }
            
            if (next_event_time == -1) {
                break;
            }

            if (current_running_idx != -1) {
                int elapsed = next_event_time - currentTime;
                procs[current_running_idx].remainingTime -= elapsed;
                if (procs[current_running_idx].remainingTime == 0) {
                    is_completed[current_running_idx] = true;
                    completionTimes[procs[current_running_idx].origIdx] = next_event_time;
                    completed++;
                }
            } else {
                result.idleTime += (next_event_time - currentTime);
            }

            currentTime = next_event_time;

            int best_idx = -1;
            for (size_t i = 0; i < procs.size(); ++i) {
                if (procs[i].p.arrivalTime <= currentTime && !is_completed[i]) {
                    if (best_idx == -1) {
                        best_idx = static_cast<int>(i);
                    } else {
                        if (procs[i].remainingTime < procs[best_idx].remainingTime) {
                            best_idx = static_cast<int>(i);
                        } else if (procs[i].remainingTime == procs[best_idx].remainingTime) {
                            if (best_idx == current_running_idx) {
                                // Keep best_idx
                            } else if (static_cast<int>(i) == current_running_idx) {
                                best_idx = static_cast<int>(i);
                            } else {
                                if (procs[i].p.arrivalTime < procs[best_idx].p.arrivalTime) {
                                    best_idx = static_cast<int>(i);
                                } else if (procs[i].p.arrivalTime == procs[best_idx].p.arrivalTime) {
                                    if (procs[i].p.id < procs[best_idx].p.id) {
                                        best_idx = static_cast<int>(i);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            
            while (next_arrival_idx < procs.size() && procs[next_arrival_idx].p.arrivalTime <= currentTime) {
                next_arrival_idx++;
            }

            if (best_idx != current_running_idx) {
                if (segment_start_time < currentTime) {
                    if (current_running_idx != -1) {
                        Scheduler::addTimelineEntry(result.timeline, procs[current_running_idx].p.id, segment_start_time, currentTime);
                    } else {
                        Scheduler::addTimelineEntry(result.timeline, -1, segment_start_time, currentTime);
                    }
                }
                segment_start_time = currentTime;

                if (best_idx != -1) {
                    if (firstCpuTimes[procs[best_idx].origIdx] == -1) {
                        firstCpuTimes[procs[best_idx].origIdx] = currentTime;
                    }
                    if (last_running_id != procs[best_idx].p.id && last_running_id != -2 && last_running_id != -1) {
                        result.contextSwitches++;
                    }
                    last_running_id = procs[best_idx].p.id;
                } else {
                    last_running_id = -1;
                }
                
                current_running_idx = best_idx;
            } else if (best_idx == -1 && current_running_idx == -1) {
                last_running_id = -1;
            }
        }

        // Flush the final timeline segment
        if (segment_start_time < currentTime) {
            if (current_running_idx != -1) {
                Scheduler::addTimelineEntry(result.timeline, procs[current_running_idx].p.id, segment_start_time, currentTime);
            } else {
                Scheduler::addTimelineEntry(result.timeline, -1, segment_start_time, currentTime);
            }
        }

        result.totalTime = currentTime;
        // Compute idle time from timeline
        result.idleTime = 0;
        for (const auto& te : result.timeline) {
            if (te.processId == -1) result.idleTime += (te.endTime - te.startTime);
        }
        result.results = Scheduler::computeResults(processes, completionTimes, firstCpuTimes);

        return result;
    }
};
