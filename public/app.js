document.addEventListener('DOMContentLoaded', () => {
    // DOM Elements
    const dropZone = document.getElementById('drop-zone');
    const fileInput = document.getElementById('file-input');
    const browseBtn = document.getElementById('browse-btn');
    const loadingState = document.getElementById('loading-state');
    const viewerContainer = document.getElementById('viewer-container');
    
    const imgOriginal = document.getElementById('img-original');
    const imgProcessed = document.getElementById('img-processed');
    const methodSelect = document.getElementById('method-select');
    const processedBadge = document.getElementById('processed-badge');
    const imgResolution = document.getElementById('img-resolution');
    const verificationStatus = document.getElementById('verification-status');
    
    const slider = document.getElementById('comparison-slider');
    const handle = document.getElementById('slider-handle');
    
    const fastestMethod = document.getElementById('fastest-method');
    const fastestTime = document.getElementById('fastest-time');
    const maxSpeedup = document.getElementById('max-speedup');
    const maxSpeedupMethod = document.getElementById('max-speedup-method');
    
    const barChartWrapper = document.getElementById('bar-chart-wrapper');
    const metricsTbody = document.getElementById('metrics-tbody');
    
    const cudaStatusCard = document.getElementById('cuda-status-card');
    const cudaCardTitle = document.getElementById('cuda-card-title');
    const cudaStatusBody = document.getElementById('cuda-status-body');

    let isDragging = false;
    let runResults = null;

    // Trigger file input when clicking browse button
    browseBtn.addEventListener('click', (e) => {
        e.stopPropagation();
        fileInput.click();
    });

    // Make entire drop zone clickable to browse files
    dropZone.addEventListener('click', (e) => {
        if (e.target !== fileInput) {
            fileInput.click();
        }
    });

    fileInput.addEventListener('click', (e) => {
        e.stopPropagation();
    });

    // Drag over styling
    dropZone.addEventListener('dragover', (e) => {
        e.preventDefault();
        dropZone.classList.add('dragover');
    });

    dropZone.addEventListener('dragleave', () => {
        dropZone.classList.remove('dragover');
    });

    dropZone.addEventListener('drop', (e) => {
        e.preventDefault();
        dropZone.classList.remove('dragover');
        if (e.dataTransfer.files.length > 0) {
            handleFileUpload(e.dataTransfer.files[0]);
        }
    });

    fileInput.addEventListener('change', () => {
        if (fileInput.files.length > 0) {
            handleFileUpload(fileInput.files[0]);
        }
    });

    // Handle Upload and Process request
    async function handleFileUpload(file) {
        // Robust check: check extension if browser leaves file.type empty
        const ext = file.name.split('.').pop().toLowerCase();
        const validExtensions = ['png', 'jpg', 'jpeg', 'bmp', 'tga', 'gif'];
        const isImage = (file.type && file.type.startsWith('image/')) || validExtensions.includes(ext);

        if (!isImage) {
            alert('Please select a valid image file (PNG, JPG, JPEG).');
            return;
        }

        const formData = new FormData();
        formData.append('image', file);

        // UI state transitions
        dropZone.classList.add('hidden');
        viewerContainer.classList.add('hidden');
        loadingState.classList.remove('hidden');

        try {
            const response = await fetch('/api/process', {
                method: 'POST',
                body: formData
            });

            if (!response.ok) {
                throw new Error(`Server returned error: ${response.statusText}`);
            }

            runResults = await response.json();
            displayResults(runResults);
        } catch (error) {
            console.error('Error during Sobel benchmark run:', error);
            alert(`Processing failed: ${error.message}. Please verify Docker is running.`);
            dropZone.classList.remove('hidden');
            loadingState.classList.add('hidden');
        }
    }

    // Interactive split slider logic
    function setSliderPos(clientX) {
        const rect = slider.getBoundingClientRect();
        let x = clientX - rect.left;
        // Clamp position within boundary
        x = Math.max(0, Math.min(x, rect.width));
        const percentage = (x / rect.width) * 100;
        slider.style.setProperty('--clip-pos', `${percentage}%`);
    }

    handle.addEventListener('mousedown', (e) => {
        isDragging = true;
        e.preventDefault();
    });

    window.addEventListener('mousemove', (e) => {
        if (!isDragging) return;
        setSliderPos(e.clientX);
    });

    window.addEventListener('mouseup', () => {
        isDragging = false;
    });

    // Mobile touch controls for slider
    handle.addEventListener('touchstart', () => {
        isDragging = true;
    });

    window.addEventListener('touchmove', (e) => {
        if (!isDragging) return;
        setSliderPos(e.touches[0].clientX);
    });

    window.addEventListener('touchend', () => {
        isDragging = false;
    });

    slider.addEventListener('click', (e) => {
        if (e.target !== handle && !handle.contains(e.target)) {
            setSliderPos(e.clientX);
        }
    });

    // Handle visualization method change
    methodSelect.addEventListener('change', () => {
        const selectedMode = methodSelect.value;
        if (runResults && runResults.methods[selectedMode]) {
            const methodData = runResults.methods[selectedMode];
            if (methodData.success) {
                imgProcessed.src = methodData.outputUrl;
                processedBadge.textContent = methodData.displayName;
                
                // Show verification
                if (selectedMode === 'serial') {
                    verificationStatus.className = 'verification-badge success';
                    verificationStatus.textContent = 'Verification: Reference';
                } else if (methodData.rmse !== undefined && methodData.rmse !== null) {
                    verificationStatus.className = 'verification-badge success';
                    verificationStatus.textContent = `Verification: RMSE=${methodData.rmse} (${methodData.psnr || 'N/A'})`;
                } else {
                    verificationStatus.className = 'verification-badge success';
                    verificationStatus.textContent = 'Verification: Match';
                }
            } else {
                alert(`Cannot visualize output: ${methodData.displayName} failed execution.`);
            }
        }
    });

    // Populate UI with benchmarking results
    function displayResults(data) {
        loadingState.classList.add('hidden');
        viewerContainer.classList.remove('hidden');

        // Set Images
        imgOriginal.src = data.originalUrl;
        
        // Resolution & details
        if (data.dimensions) {
            imgResolution.textContent = `Resolution: ${data.dimensions.width} x ${data.dimensions.height} px`;
        }

        // Populating visualization selector
        methodSelect.innerHTML = '';
        let initialMode = 'serial';

        Object.keys(data.methods).forEach(mode => {
            const m = data.methods[mode];
            if (m.success) {
                const option = document.createElement('option');
                option.value = mode;
                option.textContent = m.displayName;
                methodSelect.appendChild(option);
                
                // Set default display to first successful parallel method if available
                if (mode !== 'serial' && initialMode === 'serial') {
                    initialMode = mode;
                }
            }
        });

        // Fallback if only serial succeeded
        methodSelect.value = initialMode;
        imgProcessed.src = data.methods[initialMode].outputUrl;
        processedBadge.textContent = data.methods[initialMode].displayName;

        // Reset verification status
        if (initialMode === 'serial') {
            verificationStatus.className = 'verification-badge success';
            verificationStatus.textContent = 'Verification: Reference';
        } else {
            const initialData = data.methods[initialMode];
            verificationStatus.className = 'verification-badge success';
            verificationStatus.textContent = initialData.rmse !== undefined ? 
                `Verification: RMSE=${initialData.rmse} (${initialData.psnr || 'N/A'})` : 'Verification: Match';
        }

        // Benchmarks Summary
        let fastestName = '-';
        let fastestMs = Infinity;
        let maxSpeedupVal = 0;
        let maxSpeedupName = '-';
        let maxTimeMs = 0;

        Object.keys(data.methods).forEach(mode => {
            const m = data.methods[mode];
            if (m.success && m.timeMs) {
                if (m.timeMs < fastestMs) {
                    fastestMs = m.timeMs;
                    fastestName = m.displayName;
                }
                if (m.speedup && m.speedup > maxSpeedupVal) {
                    maxSpeedupVal = m.speedup;
                    maxSpeedupName = m.displayName;
                }
                if (m.timeMs > maxTimeMs) {
                    maxTimeMs = m.timeMs;
                }
            }
        });

        fastestMethod.textContent = fastestMs !== Infinity ? fastestName.split(' ')[0] : '-';
        fastestTime.textContent = fastestMs !== Infinity ? `${fastestMs.toFixed(2)} ms` : '-';
        maxSpeedup.textContent = maxSpeedupVal > 0 ? `${maxSpeedupVal.toFixed(2)}x` : '-';
        maxSpeedupMethod.textContent = maxSpeedupVal > 0 ? `Speedup by ${maxSpeedupName.split(' ')[0]}` : 'vs Serial Baseline';

        // Render custom styled HTML bar chart
        barChartWrapper.innerHTML = '';
        const chartBars = document.createElement('div');
        chartBars.className = 'chart-bars';

        Object.keys(data.methods).forEach(mode => {
            const m = data.methods[mode];
            const row = document.createElement('div');
            row.className = 'chart-row';

            const label = document.createElement('div');
            label.className = 'chart-label';
            label.textContent = m.displayName;

            const barContainer = document.createElement('div');
            barContainer.className = 'chart-bar-container';

            const bar = document.createElement('div');
            if (m.success) {
                bar.className = mode === 'serial' ? 'chart-bar serial' : 'chart-bar parallel';
                // Delay setting width for animation effect
                setTimeout(() => {
                    bar.style.width = `${(m.timeMs / maxTimeMs) * 100}%`;
                }, 100);
            } else {
                bar.className = 'chart-bar failed';
                bar.style.width = '10%';
            }

            barContainer.appendChild(bar);

            const value = document.createElement('div');
            value.className = 'chart-value';
            value.textContent = m.success ? `${m.timeMs.toFixed(2)} ms` : 'Failed';

            row.appendChild(label);
            row.appendChild(barContainer);
            row.appendChild(value);
            chartBars.appendChild(row);
        });

        barChartWrapper.appendChild(chartBars);

        // Render Table Body
        metricsTbody.innerHTML = '';
        Object.keys(data.methods).forEach(mode => {
            const m = data.methods[mode];
            const tr = document.createElement('tr');

            // Name
            const tdName = document.createElement('td');
            tdName.textContent = m.displayName;
            tr.appendChild(tdName);

            // Time
            const tdTime = document.createElement('td');
            tdTime.textContent = m.success ? `${m.timeMs.toFixed(2)} ms` : 'N/A (Error)';
            if (!m.success) tdTime.style.color = 'var(--danger)';
            tr.appendChild(tdTime);

            // Speedup
            const tdSpeedup = document.createElement('td');
            if (m.success) {
                if (mode === 'serial') {
                    tdSpeedup.textContent = '1.00x';
                } else if (m.speedup) {
                    const badge = document.createElement('span');
                    if (m.speedup >= 1.0) {
                        badge.className = 'badge-speedup';
                        badge.textContent = `${m.speedup.toFixed(2)}x`;
                    } else {
                        badge.className = 'badge-slowdown';
                        badge.textContent = `${m.speedup.toFixed(2)}x`;
                    }
                    tdSpeedup.appendChild(badge);
                } else {
                    tdSpeedup.textContent = '-';
                }
            } else {
                tdSpeedup.textContent = '-';
            }
            tr.appendChild(tdSpeedup);

            // Verification Details
            const tdVerify = document.createElement('td');
            if (m.success) {
                if (mode === 'serial') {
                    tdVerify.textContent = 'Reference';
                } else if (m.psnr) {
                    tdVerify.textContent = `Passed (${m.psnr})`;
                } else {
                    tdVerify.textContent = 'Passed';
                }
                tdVerify.style.color = 'var(--success)';
            } else {
                tdVerify.textContent = 'Failed';
                tdVerify.style.color = 'var(--danger)';
            }
            tr.appendChild(tdVerify);

            metricsTbody.appendChild(tr);
        });

        // CUDA notice updating based on result
        const cudaData = data.methods['cuda'];
        if (cudaData) {
            if (cudaData.success) {
                cudaStatusCard.className = 'cuda-status-card success';
                cudaCardTitle.textContent = 'CUDA GPU Accelerated: Active';
                cudaStatusBody.innerHTML = `Sobel filter successfully executed on host CUDA core in <strong>${cudaData.timeMs.toFixed(2)} ms</strong>. 
                Speedup is <strong>${cudaData.speedup ? cudaData.speedup.toFixed(2) + 'x' : '1.00x'}</strong>. Fully parallelized via local GPU threads.`;
            } else {
                cudaStatusCard.className = 'cuda-status-card error';
                cudaCardTitle.textContent = 'CUDA GPU Execution: Failed/Not Available';
                cudaStatusBody.innerHTML = `CUDA compilation succeeded inside Docker, but execution failed at runtime. This is typical when running inside macOS host virtual machines without NVIDIA GPU access. 
                <br><strong>C++ stdout/stderr debug logs:</strong>
                <pre>${escapeHtml(cudaData.error || 'Unknown error details')}</pre>`;
            }
        }
    }

    // Helper function to escape HTML string
    function escapeHtml(unsafe) {
        return unsafe
             .replace(/&/g, "&amp;")
             .replace(/</g, "&lt;")
             .replace(/>/g, "&gt;")
             .replace(/"/g, "&quot;")
             .replace(/'/g, "&#039;");
    }
});
