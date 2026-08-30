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
        
        if (processes.length === 0) {
            document.getElementById('empty-process-msg').classList.remove('hidden');
        } else {
            document.getElementById('empty-process-msg').classList.add('hidden');
            processes.forEach(p => {
                tbodyProc.innerHTML += `
                    <tr>
                        <td class="px-4 py-3 font-semibold text-slate-700">P${p.id}</td>
                        <td class="px-4 py-3 text-slate-600">${p.arrivalTime}</td>
                        <td class="px-4 py-3 text-slate-600">${p.burstTime}</td>
                        <td class="px-4 py-3 text-right">
                            <button onclick="removeProcess(${p.id})" class="text-slate-400 hover:text-red-500 font-bold transition">&times;</button>
                        </td>
                    </tr>
                `;
            });
        }
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
            document.getElementById('panel-comparison').classList.remove('hidden');
            panelRec.classList.remove('hidden');
            
            // Run Comparison
            const entries = window.SchedulerAPI.compareAll(processes, rrQ, mlfqQ);
            
            // Render Comparison Table
            const tb = document.getElementById('comparison-tbody');
            tb.innerHTML = '';
            entries.forEach(e => {
                tb.innerHTML += `
                    <tr class="hover:bg-slate-50 transition">
                        <td class="px-5 py-3 font-medium text-slate-800">${e.algorithmName}</td>
                        <td class="px-5 py-3 text-slate-600">${e.metrics.avgWaitingTime.toFixed(2)}</td>
                        <td class="px-5 py-3 text-slate-600">${e.metrics.avgTurnaroundTime.toFixed(2)}</td>
                        <td class="px-5 py-3 text-slate-600">${e.metrics.avgResponseTime.toFixed(2)}</td>
                        <td class="px-5 py-3 text-slate-600">${e.metrics.contextSwitches}</td>
                        <td class="px-5 py-3 text-slate-600">${e.metrics.fairnessIndex.toFixed(2)}</td>
                    </tr>
                `;
            });

            // Render Recommendation
            const rec = window.SchedulerAPI.getRecommendation(entries);
            document.getElementById('rec-best-algo').innerText = rec.bestOverall;
            document.getElementById('rec-explanation').innerText = rec.explanation;
            document.getElementById('rec-wt').innerText = rec.bestWaitingTime;
            document.getElementById('rec-tat').innerText = rec.bestTurnaroundTime;
            document.getElementById('rec-rt').innerText = rec.bestResponseTime;
            document.getElementById('rec-cs').innerText = rec.leastContextSwitches;
            document.getElementById('rec-fair').innerText = rec.bestFairness;

        } else {
            document.getElementById('panel-comparison').classList.add('hidden');
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
                    <tr class="hover:bg-slate-50 transition">
                        <td class="px-5 py-3 font-semibold text-slate-700">P${p.id}</td>
                        <td class="px-5 py-3 text-slate-600">${p.arrivalTime}</td>
                        <td class="px-5 py-3 text-slate-600">${p.burstTime}</td>
                        <td class="px-5 py-3 font-bold text-accent">${p.completionTime}</td>
                        <td class="px-5 py-3 text-slate-600">${p.turnaroundTime}</td>
                        <td class="px-5 py-3 text-slate-600">${p.waitingTime}</td>
                        <td class="px-5 py-3 text-slate-600">${p.responseTime}</td>
                    </tr>
                `;
            });

            renderGanttChart(result.result.timeline, result.result.totalTime);
            panelSingle.classList.remove('hidden');
        }
    });

    // Helper to generate distinct colors for PIDs
    function getColorForPid(pid) {
        if (pid === -1) return '#cbd5e1'; // IDLE color (slate-300)
        const colors = [
            '#3b82f6', '#10b981', '#f59e0b', '#8b5cf6', '#ec4899', 
            '#06b6d4', '#f97316', '#84cc16', '#14b8a6', '#6366f1'
        ];
        return colors[(pid - 1) % colors.length];
    }

    function renderGanttChart(timeline, totalTime) {
        const container = document.getElementById('gantt-container');
        const axis = document.getElementById('gantt-axis');
        container.innerHTML = '';
        axis.innerHTML = '';
        
        if (timeline.length === 0 || totalTime === 0) return;

        // Generate Time Axis Ticks
        const tickCount = Math.min(totalTime, 20); // max 20 ticks
        const interval = Math.ceil(totalTime / tickCount);
        
        for (let i = 0; i <= totalTime; i += interval) {
            const leftPct = (i / totalTime) * 100;
            axis.innerHTML += `
                <div class="absolute top-0 flex flex-col items-center" style="left: ${leftPct}%; transform: translateX(-50%);">
                    <div class="h-1.5 w-px bg-slate-300"></div>
                    <span class="text-[10px] font-medium text-slate-500 mt-0.5">${i}</span>
                </div>
            `;
        }
        
        // Ensure final tick is there if it wasn't hit perfectly
        if (totalTime % interval !== 0) {
            axis.innerHTML += `
                <div class="absolute top-0 flex flex-col items-center" style="left: 100%; transform: translateX(-50%);">
                    <div class="h-1.5 w-px bg-slate-300"></div>
                    <span class="text-[10px] font-medium text-slate-500 mt-0.5">${totalTime}</span>
                </div>
            `;
        }

        timeline.forEach(entry => {
            const widthPct = ((entry.endTime - entry.startTime) / totalTime) * 100;
            const color = getColorForPid(entry.processId);
            const label = entry.processId === -1 ? 'IDLE' : 'P' + entry.processId;
            const textColor = entry.processId === -1 ? 'text-slate-600' : 'text-white';
            
            const tooltip = entry.processId === -1 
                ? \`CPU Idle (\${entry.startTime} - \${entry.endTime})\`
                : \`Process P\${entry.processId} executing (\${entry.startTime} - \${entry.endTime})\`;

            const block = document.createElement('div');
            block.title = tooltip;
            block.className = \`gantt-block h-full relative border-r border-white/20 flex flex-col justify-center items-center \${textColor} cursor-pointer\`;
            block.style.width = widthPct + '%';
            block.style.backgroundColor = color;

            if (widthPct > 2) { // only show text if block is wide enough
                block.innerHTML = \`<span class="font-bold text-xs tracking-wider z-10 drop-shadow-md">\${label}</span>\`;
            }
            
            container.appendChild(block);
        });
    }
});
