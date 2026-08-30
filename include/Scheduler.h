#pragma once

#include "Process.h"
#include <vector>
#include <string>
#include <algorithm>
#include <queue>
#include <deque>
#include <functional>
#include <climits>
#include <cmath>
#include <numeric>

/**
 * @brief A single entry in the execution timeline (Gantt chart).
 *
 * Each entry records which process was running on the CPU during a time interval.
 * A processId of -1 indicates the CPU was idle.
 */
struct TimelineEntry {
    int processId;   ///< Process ID, or -1 for CPU idle
    int startTime;   ///< Start time of this execution segment
    int endTime;     ///< End time of this execution segment

    TimelineEntry(int pid, int start, int end)
        : processId(pid), startTime(start), endTime(end) {}
};

/**
 * @brief Per-process results after scheduling simulation.
 */
struct ProcessResult {
    int id;
    int arrivalTime;
    int burstTime;
    int completionTime;
    int turnaroundTime;   ///< completionTime - arrivalTime
    int waitingTime;      ///< turnaroundTime - burstTime
    int responseTime;     ///< firstCpuTime - arrivalTime
};

/**
 * @brief Complete result of a scheduling simulation.
 */
struct ScheduleResult {
    std::string algorithmName;
    std::vector<TimelineEntry> timeline;
    std::vector<ProcessResult> results;    ///< One per process, sorted by process ID
    int totalTime = 0;                     ///< Total simulation time
    int idleTime = 0;                      ///< Total CPU idle time
    int contextSwitches = 0;               ///< Number of context switches
};

/**
 * @brief Abstract base class for all scheduling algorithms.
 *
 * Each concrete scheduler implements the schedule() method with its own
 * algorithm logic. The interface is designed for polymorphic use:
 *
 *   Scheduler* s = new FCFSScheduler();
 *   ScheduleResult result = s->schedule(processes);
 *
 * This enables the comparison module to run all algorithms through
 * a uniform interface.
 */
class Scheduler {
public:
    virtual ~Scheduler() = default;

    /**
     * @brief Run the scheduling algorithm on the given set of processes.
     *
     * @param processes Input processes (not modified; algorithm works on internal copies).
     * @return ScheduleResult containing the execution timeline, per-process metrics,
     *         and aggregate statistics.
     */
    virtual ScheduleResult schedule(const std::vector<Process>& processes) const = 0;

    /**
     * @brief Get the human-readable name of this scheduling algorithm.
     */
    virtual std::string getName() const = 0;

protected:
    /**
     * @brief Helper: add a timeline entry, merging with the previous entry
     * if it's the same process (avoids fragmented Gantt chart entries).
     */
    static void addTimelineEntry(std::vector<TimelineEntry>& timeline,
                                  int processId, int startTime, int endTime) {
        if (!timeline.empty() && timeline.back().processId == processId
            && timeline.back().endTime == startTime) {
            timeline.back().endTime = endTime;
        } else {
            timeline.emplace_back(processId, startTime, endTime);
        }
    }

    /**
     * @brief Helper: compute per-process results from completion data.
     *
     * @param processes Original input processes.
     * @param completionTimes Completion time for each process (indexed by position in processes vector).
     * @param firstCpuTimes First time each process got the CPU (indexed same way).
     * @return Vector of ProcessResult, sorted by process ID.
     */
    static std::vector<ProcessResult> computeResults(
            const std::vector<Process>& processes,
            const std::vector<int>& completionTimes,
            const std::vector<int>& firstCpuTimes) {
        std::vector<ProcessResult> results;
        results.reserve(processes.size());
        for (size_t i = 0; i < processes.size(); ++i) {
            ProcessResult pr;
            pr.id = processes[i].id;
            pr.arrivalTime = processes[i].arrivalTime;
            pr.burstTime = processes[i].burstTime;
            pr.completionTime = completionTimes[i];
            pr.turnaroundTime = pr.completionTime - pr.arrivalTime;
            pr.waitingTime = pr.turnaroundTime - pr.burstTime;
            pr.responseTime = firstCpuTimes[i] - pr.arrivalTime;
            results.push_back(pr);
        }
        std::sort(results.begin(), results.end(),
                  [](const ProcessResult& a, const ProcessResult& b) {
                      return a.id < b.id;
                  });
        return results;
    }
};
