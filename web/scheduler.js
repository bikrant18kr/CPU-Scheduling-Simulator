(function() {
    // --------------------------------------------------------
    // Helper Functions
    // --------------------------------------------------------
    function addTimelineEntry(timeline, processId, startTime, endTime) {
        if (timeline.length > 0 && timeline[timeline.length - 1].processId === processId && timeline[timeline.length - 1].endTime === startTime) {
            timeline[timeline.length - 1].endTime = endTime;
        } else {
            timeline.push({ processId, startTime, endTime });
        }
    }

    function computeResults(processes, completionTimes, firstCpuTimes) {
        let results = [];
        for (let i = 0; i < processes.length; ++i) {
            let turnaroundTime = completionTimes[i] - processes[i].arrivalTime;
            let waitingTime = turnaroundTime - processes[i].burstTime;
            let responseTime = firstCpuTimes[i] - processes[i].arrivalTime;
            results.push({
                id: processes[i].id,
                arrivalTime: processes[i].arrivalTime,
                burstTime: processes[i].burstTime,
                completionTime: completionTimes[i],
                turnaroundTime: turnaroundTime,
                waitingTime: waitingTime,
                responseTime: responseTime
            });
        }
        results.sort((a, b) => a.id - b.id);
        return results;
    }

    function calculateMetrics(result) {
        if (result.results.length === 0) {
            return { avgWaitingTime: 0, avgTurnaroundTime: 0, avgResponseTime: 0, cpuUtilization: 0, throughput: 0, contextSwitches: 0, fairnessIndex: 0 };
        }
        
        let totalWait = 0, totalTurnaround = 0, totalResponse = 0;
        for (let p of result.results) {
            totalWait += p.waitingTime;
            totalTurnaround += p.turnaroundTime;
            totalResponse += p.responseTime;
        }
        
        let n = result.results.length;
        let avgWaitingTime = totalWait / n;
        let avgTurnaroundTime = totalTurnaround / n;
        let avgResponseTime = totalResponse / n;
        
        let varianceSum = 0;
        for (let p of result.results) {
            let diff = p.waitingTime - avgWaitingTime;
            varianceSum += diff * diff;
        }
        let fairnessIndex = Math.sqrt(varianceSum / n);
        
        let throughput = 0, cpuUtilization = 0;
        if (result.totalTime > 0) {
            throughput = n / result.totalTime;
            cpuUtilization = 100.0 * (result.totalTime - result.idleTime) / result.totalTime;
        }
        
        return {
            avgWaitingTime,
            avgTurnaroundTime,
            avgResponseTime,
            cpuUtilization,
            throughput,
            contextSwitches: result.contextSwitches,
            fairnessIndex
        };
    }

    // --------------------------------------------------------
    // Algorithms
    // --------------------------------------------------------
    function scheduleFCFS(processes) {
        let result = { algorithmName: "FCFS", timeline: [], results: [], totalTime: 0, idleTime: 0, contextSwitches: 0 };
        if (processes.length === 0) return result;
        
        let procs = [...processes].sort((a, b) => {
            if (a.arrivalTime !== b.arrivalTime) return a.arrivalTime - b.arrivalTime;
            return a.id - b.id;
        });
        
        let currentTime = 0;
        let lastProcId = -2;
        let completionTimes = new Array(processes.length).fill(0);
        let firstCpuTimes = new Array(processes.length).fill(0);
        
        for (let p of procs) {
            if (currentTime < p.arrivalTime) {
                addTimelineEntry(result.timeline, -1, currentTime, p.arrivalTime);
                result.idleTime += (p.arrivalTime - currentTime);
                currentTime = p.arrivalTime;
                lastProcId = -1;
            }
            
            if (lastProcId !== p.id) {
                if (lastProcId !== -2 && lastProcId !== -1) {
                    result.contextSwitches++;
                }
            }
            
            let origIdx = processes.findIndex(op => op.id === p.id);
            firstCpuTimes[origIdx] = currentTime;
            addTimelineEntry(result.timeline, p.id, currentTime, currentTime + p.burstTime);
            currentTime += p.burstTime;
            completionTimes[origIdx] = currentTime;
            
            lastProcId = p.id;
        }
        
        result.totalTime = currentTime;
        result.results = computeResults(processes, completionTimes, firstCpuTimes);
        return result;
    }

    function scheduleSPN(processes) {
        let result = { algorithmName: "SPN", timeline: [], results: [], totalTime: 0, idleTime: 0, contextSwitches: 0 };
        if (processes.length === 0) return result;
        
        let sorted_procs = [...processes].sort((a, b) => {
            if (a.arrivalTime !== b.arrivalTime) return a.arrivalTime - b.arrivalTime;
            return a.id - b.id;
        });
        
        let ready_queue = [];
        let currentTime = 0;
        let completed = 0;
        let next_proc_idx = 0;
        let lastProcId = -2;
        
        let completionTimes = new Array(processes.length).fill(0);
        let firstCpuTimes = new Array(processes.length).fill(0);
        
        while (completed < processes.length) {
            while (next_proc_idx < sorted_procs.length && sorted_procs[next_proc_idx].arrivalTime <= currentTime) {
                ready_queue.push(sorted_procs[next_proc_idx]);
                next_proc_idx++;
            }
            
            if (ready_queue.length === 0) {
                let nextTime = sorted_procs[next_proc_idx].arrivalTime;
                addTimelineEntry(result.timeline, -1, currentTime, nextTime);
                result.idleTime += (nextTime - currentTime);
                currentTime = nextTime;
                lastProcId = -1;
                continue;
            }
            
            let best_idx = 0;
            for (let i = 1; i < ready_queue.length; i++) {
                let a = ready_queue[best_idx];
                let b = ready_queue[i];
                let better = false;
                if (b.burstTime < a.burstTime) better = true;
                else if (b.burstTime === a.burstTime) {
                    if (b.arrivalTime < a.arrivalTime) better = true;
                    else if (b.arrivalTime === a.arrivalTime) {
                        if (b.id < a.id) better = true;
                    }
                }
                if (better) best_idx = i;
            }
            
            let p = ready_queue[best_idx];
            ready_queue.splice(best_idx, 1);
            
            if (lastProcId !== p.id && lastProcId !== -2 && lastProcId !== -1) {
                result.contextSwitches++;
            }
            
            let origIdx = processes.findIndex(op => op.id === p.id);
            firstCpuTimes[origIdx] = currentTime;
            addTimelineEntry(result.timeline, p.id, currentTime, currentTime + p.burstTime);
            currentTime += p.burstTime;
            completionTimes[origIdx] = currentTime;
            
            lastProcId = p.id;
            completed++;
        }
        
        result.totalTime = currentTime;
        result.results = computeResults(processes, completionTimes, firstCpuTimes);
        return result;
    }

    function scheduleSRT(processes) {
        let result = { algorithmName: "SRT", timeline: [], results: [], totalTime: 0, idleTime: 0, contextSwitches: 0 };
        if (processes.length === 0) return result;
        
        let procs = processes.map((p, i) => ({ p, remainingTime: p.burstTime, origIdx: i }));
        procs.sort((a, b) => {
            if (a.p.arrivalTime !== b.p.arrivalTime) return a.p.arrivalTime - b.p.arrivalTime;
            return a.p.id - b.p.id;
        });
        
        let currentTime = 0;
        let completed = 0;
        let next_arrival_idx = 0;
        let current_running_idx = -1;
        let last_running_id = -2;
        
        let completionTimes = new Array(processes.length).fill(0);
        let firstCpuTimes = new Array(processes.length).fill(-1);
        let is_completed = new Array(procs.length).fill(false);
        
        let segment_start_time = 0;
        
        while (completed < procs.length) {
            let next_event_time = -1;
            
            if (current_running_idx !== -1) {
                next_event_time = currentTime + procs[current_running_idx].remainingTime;
            }
            
            if (next_arrival_idx < procs.length) {
                if (next_event_time === -1 || procs[next_arrival_idx].p.arrivalTime < next_event_time) {
                    next_event_time = procs[next_arrival_idx].p.arrivalTime;
                }
            }
            
            if (next_event_time === -1) break;
            
            if (current_running_idx !== -1) {
                let elapsed = next_event_time - currentTime;
                procs[current_running_idx].remainingTime -= elapsed;
                if (procs[current_running_idx].remainingTime === 0) {
                    is_completed[current_running_idx] = true;
                    completionTimes[procs[current_running_idx].origIdx] = next_event_time;
                    completed++;
                }
            } else {
                result.idleTime += (next_event_time - currentTime);
            }
            
            currentTime = next_event_time;
            
            let best_idx = -1;
            for (let i = 0; i < procs.length; ++i) {
                if (procs[i].p.arrivalTime <= currentTime && !is_completed[i]) {
                    if (best_idx === -1) {
                        best_idx = i;
                    } else {
                        if (procs[i].remainingTime < procs[best_idx].remainingTime) {
                            best_idx = i;
                        } else if (procs[i].remainingTime === procs[best_idx].remainingTime) {
                            if (best_idx === current_running_idx) {
                                // keep current
                            } else if (i === current_running_idx) {
                                best_idx = i;
                            } else {
                                if (procs[i].p.arrivalTime < procs[best_idx].p.arrivalTime) {
                                    best_idx = i;
                                } else if (procs[i].p.arrivalTime === procs[best_idx].p.arrivalTime) {
                                    if (procs[i].p.id < procs[best_idx].p.id) {
                                        best_idx = i;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            
            while (next_arrival_idx < procs.length && procs[next_arrival_idx].p.arrivalTime <= currentTime) {
                next_arrival_idx++;
            }
            
            if (best_idx !== current_running_idx) {
                if (segment_start_time < currentTime) {
                    if (current_running_idx !== -1) {
                        addTimelineEntry(result.timeline, procs[current_running_idx].p.id, segment_start_time, currentTime);
                    } else {
                        addTimelineEntry(result.timeline, -1, segment_start_time, currentTime);
                    }
                }
                segment_start_time = currentTime;
                
                if (best_idx !== -1) {
                    if (firstCpuTimes[procs[best_idx].origIdx] === -1) {
                        firstCpuTimes[procs[best_idx].origIdx] = currentTime;
                    }
                    if (last_running_id !== procs[best_idx].p.id && last_running_id !== -2 && last_running_id !== -1) {
                        result.contextSwitches++;
                    }
                    last_running_id = procs[best_idx].p.id;
                } else {
                    last_running_id = -1;
                }
                
                current_running_idx = best_idx;
            } else if (best_idx === -1 && current_running_idx === -1) {
                last_running_id = -1;
            }
        }
        
        if (segment_start_time < currentTime) {
            if (current_running_idx !== -1) {
                addTimelineEntry(result.timeline, procs[current_running_idx].p.id, segment_start_time, currentTime);
            } else {
                addTimelineEntry(result.timeline, -1, segment_start_time, currentTime);
            }
        }
        
        result.totalTime = currentTime;
        result.idleTime = 0;
        for (let te of result.timeline) {
            if (te.processId === -1) result.idleTime += (te.endTime - te.startTime);
        }
        result.results = computeResults(processes, completionTimes, firstCpuTimes);
        return result;
    }

    function scheduleRR(processes, timeQuantum) {
        let result = { algorithmName: "Round Robin (q=" + timeQuantum + ")", timeline: [], results: [], totalTime: 0, idleTime: 0, contextSwitches: 0 };
        if (processes.length === 0) return result;
        
        let n = processes.length;
        let remainingTime = processes.map(p => p.burstTime);
        let completionTimes = new Array(n).fill(0);
        let firstCpuTimes = new Array(n).fill(-1);
        let arrived = new Array(n).fill(false);
        
        let currentTime = 0;
        let completed = 0;
        let readyQueue = [];
        
        let getNewArrivals = (limitTime) => {
            let newArr = [];
            for (let i = 0; i < n; ++i) {
                if (!arrived[i] && processes[i].arrivalTime <= limitTime) {
                    newArr.push(i);
                }
            }
            newArr.sort((a, b) => {
                if (processes[a].arrivalTime !== processes[b].arrivalTime)
                    return processes[a].arrivalTime - processes[b].arrivalTime;
                return processes[a].id - processes[b].id;
            });
            for (let idx of newArr) {
                readyQueue.push(idx);
                arrived[idx] = true;
            }
        };
        
        let minArrival = Math.min(...processes.map(p => p.arrivalTime));
        if (minArrival > 0) {
            currentTime = minArrival;
            addTimelineEntry(result.timeline, -1, 0, currentTime);
            result.idleTime += currentTime;
        }
        
        getNewArrivals(currentTime);
        let lastProcess = -1;
        
        while (completed < n) {
            if (readyQueue.length === 0) {
                let nextArrival = -1;
                for (let i = 0; i < n; ++i) {
                    if (!arrived[i]) {
                        if (nextArrival === -1 || processes[i].arrivalTime < nextArrival) {
                            nextArrival = processes[i].arrivalTime;
                        }
                    }
                }
                if (nextArrival !== -1) {
                    addTimelineEntry(result.timeline, -1, currentTime, nextArrival);
                    result.idleTime += (nextArrival - currentTime);
                    currentTime = nextArrival;
                    getNewArrivals(currentTime);
                }
                continue;
            }
            
            let pIdx = readyQueue.shift();
            
            if (firstCpuTimes[pIdx] === -1) {
                firstCpuTimes[pIdx] = currentTime;
            }
            
            if (lastProcess !== -1 && lastProcess !== processes[pIdx].id) {
                result.contextSwitches++;
            }
            lastProcess = processes[pIdx].id;
            
            let runTime = Math.min(timeQuantum, remainingTime[pIdx]);
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

    function scheduleHRRN(processes) {
        let result = { algorithmName: "HRRN", timeline: [], results: [], totalTime: 0, idleTime: 0, contextSwitches: 0 };
        if (processes.length === 0) return result;
        
        let n = processes.length;
        let completionTimes = new Array(n).fill(0);
        let firstCpuTimes = new Array(n).fill(-1);
        let done = new Array(n).fill(false);
        
        let currentTime = 0;
        let completed = 0;
        
        let minArrival = Math.min(...processes.map(p => p.arrivalTime));
        if (minArrival > 0) {
            currentTime = minArrival;
            addTimelineEntry(result.timeline, -1, 0, currentTime);
            result.idleTime += currentTime;
        }
        
        let lastProcessId = -1;
        
        while (completed < n) {
            let selectedIdx = -1;
            let maxRR = -1.0;
            
            for (let i = 0; i < n; ++i) {
                if (!done[i] && processes[i].arrivalTime <= currentTime) {
                    let waitTime = currentTime - processes[i].arrivalTime;
                    let rr = (waitTime + processes[i].burstTime) / processes[i].burstTime;
                    
                    let better = false;
                    if (selectedIdx === -1) {
                        better = true;
                    } else if (rr > maxRR) {
                        better = true;
                    } else if (rr === maxRR) {
                        if (processes[i].arrivalTime < processes[selectedIdx].arrivalTime) {
                            better = true;
                        } else if (processes[i].arrivalTime === processes[selectedIdx].arrivalTime) {
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
            
            if (selectedIdx === -1) {
                let nextArrival = -1;
                for (let i = 0; i < n; ++i) {
                    if (!done[i]) {
                        if (nextArrival === -1 || processes[i].arrivalTime < nextArrival) {
                            nextArrival = processes[i].arrivalTime;
                        }
                    }
                }
                if (nextArrival !== -1) {
                    addTimelineEntry(result.timeline, -1, currentTime, nextArrival);
                    result.idleTime += (nextArrival - currentTime);
                    currentTime = nextArrival;
                }
                continue;
            }
            
            firstCpuTimes[selectedIdx] = currentTime;
            
            if (lastProcessId !== -1 && lastProcessId !== processes[selectedIdx].id) {
                result.contextSwitches++;
            }
            lastProcessId = processes[selectedIdx].id;
            
            let runTime = processes[selectedIdx].burstTime;
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

    function scheduleMLFQ(processes, quanta) {
        if (!quanta || quanta.length === 0) quanta = [1];
        let result = { algorithmName: "Feedback Queue (q=" + quanta.join(",") + ")", timeline: [], results: [], totalTime: 0, idleTime: 0, contextSwitches: 0 };
        if (processes.length === 0) return result;
        
        let n = processes.length;
        let remainingTime = processes.map(p => p.burstTime);
        let completionTimes = new Array(n).fill(0);
        let firstCpuTimes = new Array(n).fill(-1);
        let arrived = new Array(n).fill(false);
        
        let queues = Array.from({length: quanta.length}, () => []);
        
        let currentTime = 0;
        let completed = 0;
        
        let getNewArrivals = (limitTime) => {
            let newArr = [];
            for (let i = 0; i < n; ++i) {
                if (!arrived[i] && processes[i].arrivalTime <= limitTime) {
                    newArr.push(i);
                }
            }
            newArr.sort((a, b) => {
                if (processes[a].arrivalTime !== processes[b].arrivalTime)
                    return processes[a].arrivalTime - processes[b].arrivalTime;
                return processes[a].id - processes[b].id;
            });
            for (let idx of newArr) {
                queues[0].push(idx);
                arrived[idx] = true;
            }
        };
        
        let minArrival = Math.min(...processes.map(p => p.arrivalTime));
        if (minArrival > 0) {
            currentTime = minArrival;
            addTimelineEntry(result.timeline, -1, 0, currentTime);
            result.idleTime += currentTime;
        }
        
        getNewArrivals(currentTime);
        
        let lastProcess = -1;
        
        while (completed < n) {
            let selectedQueue = -1;
            for (let i = 0; i < queues.length; ++i) {
                if (queues[i].length > 0) {
                    selectedQueue = i;
                    break;
                }
            }
            
            if (selectedQueue === -1) {
                let nextArrival = -1;
                for (let i = 0; i < n; ++i) {
                    if (!arrived[i]) {
                        if (nextArrival === -1 || processes[i].arrivalTime < nextArrival) {
                            nextArrival = processes[i].arrivalTime;
                        }
                    }
                }
                if (nextArrival !== -1) {
                    addTimelineEntry(result.timeline, -1, currentTime, nextArrival);
                    result.idleTime += (nextArrival - currentTime);
                    currentTime = nextArrival;
                    getNewArrivals(currentTime);
                }
                continue;
            }
            
            let pIdx = queues[selectedQueue].shift();
            
            if (firstCpuTimes[pIdx] === -1) {
                firstCpuTimes[pIdx] = currentTime;
            }
            
            if (lastProcess !== -1 && lastProcess !== processes[pIdx].id) {
                result.contextSwitches++;
            }
            lastProcess = processes[pIdx].id;
            
            let quantum = quanta[selectedQueue];
            let runTime = Math.min(quantum, remainingTime[pIdx]);
            
            addTimelineEntry(result.timeline, processes[pIdx].id, currentTime, currentTime + runTime);
            
            currentTime += runTime;
            remainingTime[pIdx] -= runTime;
            
            getNewArrivals(currentTime);
            
            if (remainingTime[pIdx] > 0) {
                let nextQueue = selectedQueue + 1;
                if (nextQueue >= queues.length) {
                    nextQueue = queues.length - 1;
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

    // --------------------------------------------------------
    // Public API
    // --------------------------------------------------------
    function compareAll(processes, rrQuantum = 2, mlfqQuanta = [1, 2, 4]) {
        let entries = [];
        
        let schedulers = [
            { name: "FCFS", run: () => scheduleFCFS(processes) },
            { name: "SPN", run: () => scheduleSPN(processes) },
            { name: "SRT", run: () => scheduleSRT(processes) },
            { name: "Round Robin (q=" + rrQuantum + ")", run: () => scheduleRR(processes, rrQuantum) },
            { name: "HRRN", run: () => scheduleHRRN(processes) },
            { name: "Feedback Queue (q=" + mlfqQuanta.join(",") + ")", run: () => scheduleMLFQ(processes, mlfqQuanta) }
        ];
        
        for (let s of schedulers) {
            let res = s.run();
            let metrics = calculateMetrics(res);
            entries.push({ algorithmName: res.algorithmName, metrics, result: res });
        }
        
        return entries;
    }

    function getRecommendation(entries) {
        if (!entries || entries.length === 0) return null;
        
        let m0 = entries[0].metrics;
        let minWT = m0.avgWaitingTime, maxWT = minWT;
        let minTAT = m0.avgTurnaroundTime, maxTAT = minTAT;
        let minRT = m0.avgResponseTime, maxRT = minRT;
        let minCPU = m0.cpuUtilization, maxCPU = minCPU;
        let minCS = m0.contextSwitches, maxCS = minCS;
        let minFair = m0.fairnessIndex, maxFair = minFair;
        
        let rec = {
            bestOverall: entries[0].algorithmName,
            bestWaitingTime: entries[0].algorithmName,
            bestTurnaroundTime: entries[0].algorithmName,
            bestResponseTime: entries[0].algorithmName,
            leastContextSwitches: entries[0].algorithmName,
            bestFairness: entries[0].algorithmName,
            scores: {},
            explanation: ""
        };
        
        for (let e of entries) {
            let m = e.metrics;
            if (m.avgWaitingTime < minWT) { minWT = m.avgWaitingTime; rec.bestWaitingTime = e.algorithmName; }
            if (m.avgWaitingTime > maxWT) { maxWT = m.avgWaitingTime; }
            
            if (m.avgTurnaroundTime < minTAT) { minTAT = m.avgTurnaroundTime; rec.bestTurnaroundTime = e.algorithmName; }
            if (m.avgTurnaroundTime > maxTAT) { maxTAT = m.avgTurnaroundTime; }
            
            if (m.avgResponseTime < minRT) { minRT = m.avgResponseTime; rec.bestResponseTime = e.algorithmName; }
            if (m.avgResponseTime > maxRT) { maxRT = m.avgResponseTime; }
            
            if (m.cpuUtilization < minCPU) { minCPU = m.cpuUtilization; }
            if (m.cpuUtilization > maxCPU) { maxCPU = m.cpuUtilization; }
            
            if (m.contextSwitches < minCS) { minCS = m.contextSwitches; rec.leastContextSwitches = e.algorithmName; }
            if (m.contextSwitches > maxCS) { maxCS = m.contextSwitches; }
            
            if (m.fairnessIndex < minFair) { minFair = m.fairnessIndex; rec.bestFairness = e.algorithmName; }
            if (m.fairnessIndex > maxFair) { maxFair = m.fairnessIndex; }
        }
        
        let bestOverallScore = Infinity;
        
        for (let e of entries) {
            let m = e.metrics;
            let norm = (val, minV, maxV) => {
                if (maxV === minV) return 0.0;
                return (val - minV) / (maxV - minV);
            };
            
            let nWT = norm(m.avgWaitingTime, minWT, maxWT);
            let nTAT = norm(m.avgTurnaroundTime, minTAT, maxTAT);
            let nRT = norm(m.avgResponseTime, minRT, maxRT);
            let nCPU = 1.0 - norm(m.cpuUtilization, minCPU, maxCPU);
            let nCS = norm(m.contextSwitches, minCS, maxCS);
            let nFair = norm(m.fairnessIndex, minFair, maxFair);
            
            let score = 0.25 * nWT + 0.2 * nTAT + 0.2 * nRT + 0.1 * nCPU + 0.1 * nCS + 0.15 * nFair;
            rec.scores[e.algorithmName] = score;
            
            if (score < bestOverallScore) {
                bestOverallScore = score;
                rec.bestOverall = e.algorithmName;
            }
        }
        
        rec.explanation = "The recommendation engine uses a weighted scoring heuristic across multiple metrics.\n" +
                          "Lower scores represent better overall performance across standard evaluation criteria.\n" +
                          "Weights used: Wait Time (25%), Turnaround (20%), Response (20%), Fairness (15%),\n" +
                          "CPU Utilization (10%), Context Switches (10%).\n" +
                          "Algorithm '" + rec.bestOverall + "' achieved the lowest (best) heuristic score.";
                          
        return rec;
    }

    window.SchedulerAPI = {
        runAlgorithm: function(algoName, processes, options = {}) {
            let res;
            let name = algoName.toLowerCase().replace(/[^a-z]/g, '');
            switch (name) {
                case 'fcfs':
                    res = scheduleFCFS(processes); break;
                case 'spn':
                    res = scheduleSPN(processes); break;
                case 'srt':
                    res = scheduleSRT(processes); break;
                case 'roundrobin':
                case 'rr':
                    res = scheduleRR(processes, options.rrQuantum || 2); break;
                case 'hrrn':
                    res = scheduleHRRN(processes); break;
                case 'feedbackqueue':
                case 'mlfq':
                    res = scheduleMLFQ(processes, options.mlfqQuanta || [1, 2, 4]); break;
                default:
                    throw new Error("Unknown algorithm: " + algoName);
            }
            let metrics = calculateMetrics(res);
            return { algorithmName: res.algorithmName, metrics, result: res };
        },
        compareAll: compareAll,
        getRecommendation: getRecommendation
    };
})();
