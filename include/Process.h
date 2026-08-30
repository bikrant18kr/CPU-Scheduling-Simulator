#pragma once

/**
 * @brief Represents a process (task) in the CPU scheduling simulation.
 *
 * This is a pure data object representing the input specification of a process.
 * Simulation state (remaining time, completion time, etc.) is managed internally
 * by each scheduling algorithm.
 */
struct Process {
    int id;            ///< Unique process identifier (1-based)
    int arrivalTime;   ///< Time at which the process enters the ready queue
    int burstTime;     ///< Total CPU time required by the process

    Process() : id(0), arrivalTime(0), burstTime(0) {}

    Process(int id, int arrivalTime, int burstTime)
        : id(id), arrivalTime(arrivalTime), burstTime(burstTime) {}
};
