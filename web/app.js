// app.js - UI Controller

document.addEventListener('DOMContentLoaded', () => {
    
    let processes = [];
    let pidCounter = 1;

    // DOM Elements
    const tbodyProc = document.getElementById('process-tbody');
    const inputArr = document.getElementById('input-arrival');
    const inputBur = document.getElementById('input-burst');
    const btnAdd = document.getElementById('btn-add-process');
    const btnSample = document.getElementById('btn-load-sample');
    
    const selectAlgo = document.getElementById('select-algo');
    const paramRR = document.getElementById('param-rr');
    const paramMLFQ = document.getElementById('param-mlfq');
    const btnRun = document.getElementById('btn-run');
    
    // Result panels
    const panelEmpty = document.getElementById('panel-empty');
    const panelSingle = document.getElementById('panel-single-result');
    const panelComp = document.getElementById('panel-comparison');
    const panelRec = document.getElementById('panel-recommendation');

    // Toggling parameters based on algorithm selection
    selectAlgo.addEventListener('change', (e) => {
        paramRR.classList.add('hidden');
        paramMLFQ.classList.add('hidden');
        if (e.target.value === 'RR' || e.target.value === 'ALL') paramRR.classList.remove('hidden');
        if (e.target.value === 'MLFQ' || e.target.value === 'ALL') paramMLFQ.classList.remove('hidden');
    });

    // Add Process
    btnAdd.addEventListener('click', () => {
        const arrival = parseInt(inputArr.value);
        const burst = parseInt(inputBur.value);
        if (arrival >= 0 && burst > 0) {
            processes.push({ id: pidCounter++, arrivalTime: arrival, burstTime: burst });
            renderProcessTable();
            inputArr.value = '';
            inputBur.value = '';
            inputArr.focus();
        } else {
            alert('Invalid input. Arrival >= 0 and Burst > 0.');
        }
    });

    // Load Sample
    btnSample.addEventListener('click', () => {
        processes = [
            { id: 1, arrivalTime: 0, burstTime: 6 },
            { id: 2, arrivalTime: 1, burstTime: 4 },
            { id: 3, arrivalTime: 2, burstTime: 2 },
            { id: 4, arrivalTime: 3, burstTime: 3 },
            { id: 5, arrivalTime: 5, burstTime: 5 }
        ];
        pidCounter = 6;
        renderProcessTable();
    });

    // Remove Process
    window.removeProcess = (id) => {
        processes = processes.filter(p => p.id !== id);
        renderProcessTable();
    };

    function renderProcessTable() {
        tbodyProc.innerHTML = '';
        processes.forEach(p => {
            tbodyProc.innerHTML += `
                <tr>
                    <td class="px-3 py-2 font-medium">P${p.id}</td>
                    <td class="px-3 py-2">${p.arrivalTime}</td>
                    <td class="px-3 py-2">${p.burstTime}</td>
                    <td class="px-3 py-2 text-right">
                        <button onclick="removeProcess(${p.id})" class="text-red-500 hover:text-red-700 font-bold">&times;</button>
                    </td>
                </tr>
            `;
        });
    }

    // Run Simulation
    btnRun.addEventListener('click', () => {
        if (processes.length === 0) {
            alert("Please add at least one process.");
            return;
        }

        const algo = selectAlgo.value;
        const rrQ = parseInt(document.getElementById('input-rr-q').value) || 2;
        const mlfqQStr = document.getElementById('input-mlfq-q').value;
        const mlfqQ = mlfqQStr.split(',').map(n => parseInt(n.trim())).filter(n => !isNaN(n) && n > 0);
        
        if (mlfqQ.length === 0) mlfqQ.push(1, 2, 4);

        panelEmpty.classList.add('hidden');

        if (algo === 'ALL') {
            panelSingle.classList.add('hidden');
            
            // Run Comparison
            const entries = window.SchedulerAPI.compareAll(processes, rrQ, mlfqQ);
            
            // Render Comparison Table
            const tb = document.getElementById('comparison-tbody');
            tb.innerHTML = '';
            entries.forEach(e => {
                tb.innerHTML += `
                    <tr>
                        <td class="px-3 py-2 font-medium">${e.algorithmName}</td>
                        <td class="px-3 py-2">${e.metrics.avgWaitingTime.toFixed(2)}</td>
                        <td class="px-3 py-2">${e.metrics.avgTurnaroundTime.toFixed(2)}</td>
                        <td class="px-3 py-2">${e.metrics.avgResponseTime.toFixed(2)}</td>
                        <td class="px-3 py-2">${e.metrics.contextSwitches}</td>
                        <td class="px-3 py-2">${e.metrics.fairnessIndex.toFixed(2)}</td>
                    </tr>
                `;
            });
            panelComp.classList.remove('hidden');

            // Render Recommendation
            const rec = window.SchedulerAPI.getRecommendation(entries);
            document.getElementById('rec-best-algo').innerText = rec.bestOverall;
            document.getElementById('rec-explanation').innerText = rec.explanation;
            document.getElementById('rec-wt').innerText = rec.bestWaitingTime;
            document.getElementById('rec-tat').innerText = rec.bestTurnaroundTime;
            document.getElementById('rec-rt').innerText = rec.bestResponseTime;
            document.getElementById('rec-cs').innerText = rec.leastContextSwitches;
            document.getElementById('rec-fair').innerText = rec.bestFairness;
            
            panelRec.classList.remove('hidden');

        } else {
            panelComp.classList.add('hidden');
            panelRec.classList.add('hidden');
            
            // Run Single
            let result;
            if (algo === 'RR') {
                result = window.SchedulerAPI.runAlgorithm('RR', processes, { rrQuantum: rrQ });
            } else if (algo === 'MLFQ') {
                result = window.SchedulerAPI.runAlgorithm('MLFQ', processes, { mlfqQuanta: mlfqQ });
            } else {
                result = window.SchedulerAPI.runAlgorithm(algo, processes);
            }

            // Update UI
            document.getElementById('gantt-algo-name').innerText = result.algorithmName;
            document.getElementById('metric-awt').innerText = result.metrics.avgWaitingTime.toFixed(2);
            document.getElementById('metric-atat').innerText = result.metrics.avgTurnaroundTime.toFixed(2);
            document.getElementById('metric-cpu').innerText = result.metrics.cpuUtilization.toFixed(1) + '%';
            document.getElementById('metric-cs').innerText = result.metrics.contextSwitches;

            // Process Table
            const tb = document.getElementById('result-tbody');
            tb.innerHTML = '';
            result.result.results.forEach(p => {
                tb.innerHTML += `
                    <tr>
                        <td class="px-3 py-2 font-medium">P${p.id}</td>
                        <td class="px-3 py-2">${p.arrivalTime}</td>
                        <td class="px-3 py-2">${p.burstTime}</td>
                        <td class="px-3 py-2 font-semibold text-indigo-600">${p.completionTime}</td>
                        <td class="px-3 py-2">${p.turnaroundTime}</td>
                        <td class="px-3 py-2">${p.waitingTime}</td>
                        <td class="px-3 py-2">${p.responseTime}</td>
                    </tr>
                `;
            });

            renderGanttChart(result.result.timeline, result.result.totalTime);
            panelSingle.classList.remove('hidden');
        }
    });

    // Helper to generate distinct colors for PIDs
    function getColorForPid(pid) {
        if (pid === -1) return '#e2e8f0'; // IDLE color (slate-200)
        const colors = [
            '#3b82f6', '#ef4444', '#10b981', '#f59e0b', '#8b5cf6', 
            '#ec4899', '#06b6d4', '#f97316', '#84cc16', '#14b8a6'
        ];
        return colors[(pid - 1) % colors.length];
    }

    function renderGanttChart(timeline, totalTime) {
        const container = document.getElementById('gantt-container');
        container.innerHTML = '';
        
        if (timeline.length === 0 || totalTime === 0) return;

        timeline.forEach(entry => {
            const widthPct = ((entry.endTime - entry.startTime) / totalTime) * 100;
            const color = getColorForPid(entry.processId);
            const label = entry.processId === -1 ? 'IDLE' : 'P' + entry.processId;
            const textColor = entry.processId === -1 ? 'text-slate-500' : 'text-white';

            const block = document.createElement('div');
            block.className = `gantt-block h-full relative border-r border-white/20 flex flex-col justify-center items-center rounded-sm ${textColor}`;
            block.style.width = widthPct + '%';
            block.style.backgroundColor = color;
            block.style.minWidth = '20px';

            // Process Label
            block.innerHTML = `<span class="font-bold text-sm tracking-wide z-10">${label}</span>`;
            
            // Start Time Marker
            const startMarker = document.createElement('div');
            startMarker.className = 'absolute -bottom-6 left-0 text-xs text-slate-500 font-mono transform -translate-x-1/2';
            startMarker.innerText = entry.startTime;
            block.appendChild(startMarker);

            container.appendChild(block);
        });

        // Add the final end time marker on the very last block
        const lastEntry = timeline[timeline.length - 1];
        const endMarker = document.createElement('div');
        endMarker.className = 'absolute -bottom-6 right-0 text-xs text-slate-500 font-mono transform translate-x-1/2';
        endMarker.innerText = lastEntry.endTime;
        container.lastChild.appendChild(endMarker);
    }
});
