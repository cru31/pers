let allResults = [];
let filteredResults = [];

// Fetch and display results
async function loadResults() {
    try {
        // Get session info
        const sessionResponse = await fetch('/api/session');
        const sessionData = await sessionResponse.json();
        document.getElementById('session-info').textContent = `Session: ${sessionData.sessionId}`;
        
        // Display Result JSON path as hyperlink
        if (sessionData.dataPath) {
            const pathElement = document.getElementById('json-file-path');
            pathElement.innerHTML = `Result JSON: <a href="#" class="source-path-link" data-path="${sessionData.dataPath}" style="color: #667eea;">${sessionData.dataPath}</a>`;
        }

        // Get test results
        const response = await fetch('/api/results');
        if (!response.ok) {
            throw new Error('Failed to load test results');
        }

        const data = await response.json();
        allResults = data.results || [];
        
        // Update summary
        updateSummary(data.metadata);
        
        // Display both Test Cases and Result JSON paths from metadata if available
        if (data && data.metadata && data.metadata.test_case_json) {
            const pathElement = document.getElementById('json-file-path');
            const testCasesLink = `Test Cases: <a href="#" class="source-path-link" data-path="${data.metadata.test_case_json}" style="color: #667eea;">${data.metadata.test_case_json}</a>`;
            const resultLink = sessionData.dataPath ? `<br>Result: <a href="#" class="source-path-link" data-path="${sessionData.dataPath}" style="color: #667eea;">${sessionData.dataPath}</a>` : '';
            pathElement.innerHTML = testCasesLink + resultLink;
        }
        
        // Populate category filter
        populateCategoryFilter();
        
        // Display results
        filteredResults = [...allResults];
        displayResults();
        
        // Hide loading
        document.getElementById('loading').style.display = 'none';
    } catch (error) {
        console.error('Error loading results:', error);
        document.getElementById('loading').style.display = 'none';
        document.getElementById('error').textContent = `Error: ${error.message}`;
        document.getElementById('error').style.display = 'block';
    }
}

// Update summary cards
function updateSummary(metadata) {
    if (!metadata) return;
    
    document.getElementById('total-tests').textContent = metadata.total_tests || 0;
    document.getElementById('passed-tests').textContent = metadata.passed || 0;
    document.getElementById('failed-tests').textContent = metadata.failed || 0;
    
    // Update Test Not Implemented count
    let naElement = document.getElementById('na-tests');
    if (naElement) {
        naElement.textContent = metadata.test_not_implemented || 0;
    }
    
    // Update Engine Feature NYI count
    let nyiElement = document.getElementById('nyi-tests');
    if (nyiElement) {
        nyiElement.textContent = metadata.engine_feature_nyi || 0;
    }
    
    document.getElementById('pass-rate').textContent = 
        metadata.pass_rate ? `${metadata.pass_rate.toFixed(1)}%` : '0%';
    document.getElementById('total-time').textContent = 
        metadata.total_time_ms ? `${metadata.total_time_ms.toFixed(0)}ms` : '0ms';
}

// Populate category filter
function populateCategoryFilter() {
    const categories = [...new Set(allResults.map(r => r.category))];
    const select = document.getElementById('category-filter');
    
    // Clear existing options except first
    select.innerHTML = '<option value="">All Categories</option>';
    
    categories.sort().forEach(category => {
        const option = document.createElement('option');
        option.value = category;
        option.textContent = category;
        select.appendChild(option);
    });
}

// Display results in table
function displayResults() {
    const tbody = document.getElementById('results-body');
    tbody.innerHTML = '';
    
    if (filteredResults.length === 0) {
        tbody.innerHTML = '<tr><td colspan="7" class="no-results">No test results found</td></tr>';
        return;
    }
    
    filteredResults.forEach((result, index) => {
        // Main row
        const row = document.createElement('tr');
        row.dataset.index = index;
        row.onclick = () => toggleDetails(index);
        
        // Determine status - can have multiple badges
        let badges = [];
        
        // Check Test Not Implemented first (highest priority)
        if (result.actual_result && result.actual_result.includes('Test Not Implemented')) {
            badges.push({ status: 'na', text: 'TEST NYI' });
        } else {
            // Check pass/fail status
            if (result.passed) {
                badges.push({ status: 'passed', text: 'PASS' });
            } else {
                badges.push({ status: 'failed', text: 'FAIL' });
            }
        }
        
        // Check NYI independently (can coexist with pass/fail/na)
        if (result.log_messages && result.log_messages.some(log => 
                   log.includes('[TODO_OR_DIE]') || log.includes('[TODO_OR_DIE ]'))) {
            badges.push({ status: 'nyi', text: 'NYI' });
        }
        
        // Create badge HTML
        const badgeHtml = badges.map(badge => 
            `<span class="status-badge status-${badge.status}">${badge.text}</span>`
        ).join(' ');
        
        row.innerHTML = `
            <td><strong>${result.id}</strong></td>
            <td>${result.category || '-'}</td>
            <td>${result.test_type || '-'}</td>
            <td>${badgeHtml}</td>
            <td>${result.execution_time_ms ? result.execution_time_ms.toFixed(2) : '-'}</td>
            <td>${truncate(result.expected_result, 30)}</td>
            <td>${truncate(result.actual_result, 30)}</td>
        `;
        tbody.appendChild(row);
        
        // Detail row
        const detailRow = document.createElement('tr');
        detailRow.className = 'detail-row';
        detailRow.dataset.detailIndex = index;
        
        const detailCell = document.createElement('td');
        detailCell.colSpan = 7;
        
        // Parse input parameters
        let inputHtml = '';
        if (result.input) {
            const params = parseInput(result.input);
            inputHtml = '<div class="input-params">';
            for (const [key, value] of Object.entries(params)) {
                inputHtml += `<div class="param-item"><strong>${key}:</strong> ${value}</div>`;
            }
            inputHtml += '</div>';
        }
        
        detailCell.innerHTML = `
            <div class="detail-content">
                <h4>Input Parameters</h4>
                ${inputHtml || '<p>No input parameters</p>'}
                
                <h4>Expected Result</h4>
                <pre>${result.expected_result || 'N/A'}</pre>
                
                <h4>Actual Result</h4>
                <pre>${result.actual_result || 'N/A'}</pre>
                
                ${result.failure_reason ? `
                    <h4>Failure Reason</h4>
                    <pre>${result.failure_reason}</pre>
                ` : ''}
                
                ${result.expected_callstack && result.expected_callstack.length > 0 ? `
                    <h4>Expected Callstack</h4>
                    <pre>${result.expected_callstack.join('\n')}</pre>
                ` : ''}
                
                ${result.log_messages && result.log_messages.length > 0 ? `
                    <h4>Engine Log Messages (${result.log_messages.length} entries)</h4>
                    <div class="log-messages">
                        ${formatLogMessages(result.log_messages)}
                    </div>
                ` : ''}
                
                <h4>Execution Details</h4>
                <div class="input-params">
                    <div class="param-item"><strong>Time:</strong> ${result.execution_time_ms?.toFixed(2) || '-'}ms</div>
                    <div class="param-item"><strong>Timestamp:</strong> ${result.timestamp || '-'}</div>
                </div>
            </div>
        `;
        
        detailRow.appendChild(detailCell);
        tbody.appendChild(detailRow);
    });
}

// Toggle detail row visibility
function toggleDetails(index) {
    const detailRow = document.querySelector(`tr[data-detail-index="${index}"]`);
    const mainRow = document.querySelector(`tr[data-index="${index}"]`);
    
    if (detailRow.style.display === 'table-row') {
        detailRow.style.display = 'none';
        mainRow.classList.remove('expanded');
    } else {
        // Show this detail row (without hiding others)
        detailRow.style.display = 'table-row';
        mainRow.classList.add('expanded');
    }
}

// Parse input string into key-value pairs
function parseInput(input) {
    const params = {};
    if (!input) return params;
    
    // Split by comma and equals
    const pairs = input.split(', ');
    pairs.forEach(pair => {
        const [key, ...valueParts] = pair.split('=');
        if (key) {
            params[key.trim()] = valueParts.join('=').trim();
        }
    });
    
    return params;
}

// Truncate long strings
function truncate(str, length) {
    if (!str) return '-';
    if (str.length <= length) return str;
    return str.substring(0, length) + '...';
}

// Format log messages with color coding and clickable paths
function formatLogMessages(logs) {
    if (!logs || logs.length === 0) return '<div class="no-logs">No logs captured</div>';
    
    return logs.map(log => {
        let processedLog = log;
        
        // New format: "[LEVEL] [Category] [fullpath:line function] message"
        // Example: "[INFO] [WebGPUInstance] [D:\path\to\file.cpp:105 namespace::class::function] Created successfully"
        // The path may contain drive letter and backslashes on Windows
        const newFormatMatch = log.match(/^(\[[^\]]+\])\s*(\[[^\]]+\])\s*\[([^:\]]+(?::[^:\]]+)?):(\d+)\s+([^\]]+)\]\s*(.*)$/);
        
        if (newFormatMatch) {
            const level = newFormatMatch[1];
            const category = newFormatMatch[2];
            const filePath = newFormatMatch[3];
            const lineNumber = newFormatMatch[4];
            const functionName = newFormatMatch[5];
            const message = newFormatMatch[6];
            
            // Extract just the filename from the path
            const fileName = filePath.split(/[\/\\]/).pop();
            
            // Create clickable link for the source location  
            const linkHtml = `[<a href="#" class="source-path-link" data-path="${filePath}" data-line="${lineNumber}" style="color: #667eea; text-decoration: none;">${fileName}:${lineNumber} ${functionName}</a>]`;
            
            // Rebuild the log with the link at the end
            processedLog = `${level} ${category} ${message} ${linkHtml}`;
        }
        
        // Parse log level and apply colored span
        const levelMatch = processedLog.match(/^\[(TRACE|DEBUG|INFO|TODO_SOMEDAY|WARNING|TODO_OR_DIE|ERROR|CRITICAL)\]/);
        if (levelMatch) {
            const level = levelMatch[1];
            const levelClassMap = {
                'TRACE': 'log-trace',
                'DEBUG': 'log-debug',
                'INFO': 'log-info',
                'TODO_SOMEDAY': 'log-todo-someday',
                'WARNING': 'log-warning',
                'TODO_OR_DIE': 'log-todo-or-die',
                'ERROR': 'log-error',
                'CRITICAL': 'log-critical'
            };
            const logClass = levelClassMap[level] || '';
            
            // Replace the log level with a colored span
            const coloredLog = processedLog.replace(
                /^\[(TRACE|DEBUG|INFO|TODO_SOMEDAY|WARNING|TODO_OR_DIE|ERROR|CRITICAL)\]/,
                `<span class="log-level-inline ${logClass}">[${level}]</span>`
            );
            return `<div class="log-line">${coloredLog}</div>`;
        } else {
            return `<div class="log-line">${processedLog}</div>`;
        }
    }).join('');
}

// Filter results
function filterResults() {
    const searchTerm = document.getElementById('search').value.toLowerCase();
    const categoryFilter = document.getElementById('category-filter').value;
    const statusFilter = document.getElementById('status-filter').value;
    
    filteredResults = allResults.filter(result => {
        // Search filter
        if (searchTerm) {
            const searchableText = [
                result.id,
                result.category,
                result.test_type,
                result.input,
                result.expected_result,
                result.actual_result
            ].join(' ').toLowerCase();
            
            if (!searchableText.includes(searchTerm)) {
                return false;
            }
        }
        
        // Category filter
        if (categoryFilter && result.category !== categoryFilter) {
            return false;
        }
        
        // Status filter
        if (statusFilter) {
            // Check if test has the selected status
            let hasSelectedStatus = false;
            
            if (statusFilter === 'na') {
                hasSelectedStatus = result.actual_result && result.actual_result.includes('Test Not Implemented');
            } else if (statusFilter === 'nyi') {
                hasSelectedStatus = result.log_messages && result.log_messages.some(log => 
                                   log.includes('[TODO_OR_DIE]') || log.includes('[TODO_OR_DIE ]'));
            } else if (statusFilter === 'passed') {
                hasSelectedStatus = result.passed && (!result.actual_result || !result.actual_result.includes('N/A'));
            } else if (statusFilter === 'failed') {
                hasSelectedStatus = !result.passed && (!result.actual_result || !result.actual_result.includes('N/A'));
            }
            
            // Return true if the test has the selected status
            if (!hasSelectedStatus) {
                return false;
            }
        }
        
        return true;
    });
    
    displayResults();
}

// Create full logs view
function createFullLogsView() {
    const container = document.getElementById('full-logs-container');
    container.innerHTML = '';
    
    // Collect all logs with their test IDs
    const allLogs = [];
    allResults.forEach(result => {
        if (result.log_messages && result.log_messages.length > 0) {
            result.log_messages.forEach(log => {
                allLogs.push({
                    testId: result.id,
                    testType: result.test_type,
                    category: result.category,
                    passed: result.passed,
                    log: log,
                    result: result
                });
            });
        }
    });
    
    if (allLogs.length === 0) {
        container.innerHTML = '<div class="no-logs">No logs captured</div>';
        return;
    }
    
    // Track which test details are expanded
    const expandedTests = new Set();
    
    // Display all logs
    allLogs.forEach((entry, index) => {
        const logWrapper = document.createElement('div');
        logWrapper.className = 'log-wrapper';
        
        // Create log entry
        const logDiv = document.createElement('div');
        logDiv.className = 'log-entry';
        logDiv.dataset.index = index;
        logDiv.dataset.testId = entry.testId;
        
        // Parse log components
        let logLevel = '';
        let logCategory = '';
        let logMessage = '';
        let logFile = '';
        let logLine = '';
        let logFunction = '';
        let logClass = '';
        
        // Extract log level
        const levelMatch = entry.log.match(/^\[(TRACE|DEBUG|INFO|TODO_SOMEDAY|WARNING|TODO_OR_DIE|ERROR|CRITICAL)\]\s*/);
        if (levelMatch) {
            logLevel = levelMatch[1];
            logDiv.dataset.logLevel = logLevel; // Store for filtering
            
            // Extract rest of the log after level
            const afterLevel = entry.log.substring(levelMatch[0].length);
            
            // Parse category, message and source location
            // New format: "[Category] [fullpath:line function] message"
            // The path may contain drive letter and backslashes on Windows
            const newSourceMatch = afterLevel.match(/^(\[[^\]]+\])\s*\[([^:\]]+(?::[^:\]]+)?):(\d+)\s+([^\]]+)\]\s*(.*)$/);
            if (newSourceMatch) {
                logCategory = newSourceMatch[1].replace(/[\[\]]/g, '').trim();
                logFile = newSourceMatch[2].trim();
                logLine = newSourceMatch[3];
                logFunction = newSourceMatch[4].trim();
                logMessage = newSourceMatch[5].trim();
            } else {
                // Try old format: "Category: Message (FilePath:LineNumber)"
                const oldSourceMatch = afterLevel.match(/^([^:]+):\s+(.*)\s+\(([^)]+)\)$/);
                if (oldSourceMatch) {
                    logCategory = oldSourceMatch[1].trim();
                    logMessage = oldSourceMatch[2].trim();
                    const sourceInfo = oldSourceMatch[3];
                    
                    // Split the source info to get file path and line number
                    const lastColon = sourceInfo.lastIndexOf(':');
                    if (lastColon !== -1) {
                        logFile = sourceInfo.substring(0, lastColon);
                        logLine = sourceInfo.substring(lastColon + 1);
                    } else {
                        logFile = sourceInfo;
                    }
                    logFunction = 'N/A'; // Old format doesn't have function
                } else {
                    logMessage = afterLevel;
                    logFunction = 'N/A';
                }
            }
            
            // Map to CSS class
            const levelClassMap = {
                'TRACE': 'log-trace',
                'DEBUG': 'log-debug',
                'INFO': 'log-info',
                'TODO_SOMEDAY': 'log-todo-someday',
                'WARNING': 'log-warning',
                'TODO_OR_DIE': 'log-todo-or-die',
                'ERROR': 'log-error',
                'CRITICAL': 'log-critical'
            };
            logClass = levelClassMap[logLevel] || '';
        }
        
        // Apply the log level class to get the color
        const levelSpan = `<span class="log-level-badge ${logClass}">[${logLevel}]</span>`;
        const categorySpan = logCategory ? `<span class="log-category">${escapeHtml(logCategory)}:</span>` : '';
        const messageSpan = `<span class="log-message">${escapeHtml(logMessage)}</span>`;
        
        logDiv.innerHTML = `<span class="test-id-badge">${entry.testId}</span>${levelSpan}${categorySpan} ${messageSpan}`;
        
        // Create source info container (hidden by default)
        const sourceDiv = document.createElement('div');
        sourceDiv.className = 'log-source-info';
        sourceDiv.style.display = 'none';
        
        // Extract just the filename from the full path
        const fileName = logFile ? logFile.split('\\').pop() : '';
        
        sourceDiv.innerHTML = `
            <div class="source-detail">
                <span class="source-label">Function:</span> ${escapeHtml(logFunction || 'N/A')}
            </div>
            <div class="source-detail">
                <span class="source-label">File:</span> ${escapeHtml(fileName || 'N/A')}
            </div>
            <div class="source-detail">
                <span class="source-label">Line:</span> ${escapeHtml(logLine || 'N/A')}
            </div>
            <div class="source-detail">
                <span class="source-label">Full Path:</span> 
                ${logFile ? `<a href="#" class="source-path-link" data-path="${escapeHtml(logFile)}" data-line="${escapeHtml(logLine)}">${escapeHtml(logFile)}</a>` : 'N/A'}
            </div>
        `;
        
        // Create test details container (hidden by default)
        const detailsDiv = document.createElement('div');
        detailsDiv.className = 'log-test-details-inline';
        detailsDiv.style.display = 'none';
        detailsDiv.dataset.testId = entry.testId;
        
        // Generate test details content
        const status = entry.passed ? '<span style="color: #48bb78">PASSED</span>' : '<span style="color: #f56565">FAILED</span>';
        let inputHtml = '';
        if (entry.result.input) {
            const params = parseInput(entry.result.input);
            inputHtml = '<div class="inline-params">';
            for (const [key, value] of Object.entries(params)) {
                inputHtml += `<span class="param-inline"><strong>${key}:</strong> ${value}</span>`;
            }
            inputHtml += '</div>';
        }
        
        detailsDiv.innerHTML = `
            <div class="test-detail-inline">
                <div class="detail-row-inline">
                    <span class="detail-label">Test:</span> ${entry.testType}
                </div>
                <div class="detail-row-inline">
                    <span class="detail-label">Category:</span> ${entry.category || 'N/A'}
                </div>
                <div class="detail-row-inline">
                    <span class="detail-label">Status:</span> ${status}
                </div>
                <div class="detail-row-inline">
                    <span class="detail-label">Input:</span> ${inputHtml || 'No parameters'}
                </div>
                <div class="detail-row-inline">
                    <span class="detail-label">Expected:</span> <code>${entry.result.expected_result || 'N/A'}</code>
                </div>
                <div class="detail-row-inline">
                    <span class="detail-label">Actual:</span> <code>${entry.result.actual_result || 'N/A'}</code>
                </div>
                ${entry.result.failure_reason ? `
                    <div class="detail-row-inline">
                        <span class="detail-label">Failure:</span> <code>${entry.result.failure_reason}</code>
                    </div>
                ` : ''}
            </div>
        `;
        
        // Add click handler to log entry
        logDiv.addEventListener('click', () => {
            // Toggle source info and test details
            if (sourceDiv.style.display === 'none') {
                sourceDiv.style.display = 'block';
                detailsDiv.style.display = 'block';
                logDiv.classList.add('selected');
            } else {
                sourceDiv.style.display = 'none';
                detailsDiv.style.display = 'none';
                logDiv.classList.remove('selected');
            }
        });
        
        logWrapper.appendChild(logDiv);
        logWrapper.appendChild(sourceDiv);
        logWrapper.appendChild(detailsDiv);
        container.appendChild(logWrapper);
    });
}

// Show test case modal
function showTestModal(testResult) {
    const modal = document.getElementById('test-modal');
    const modalTitle = document.getElementById('modal-title');
    const modalBody = document.getElementById('modal-body');
    
    modalTitle.textContent = `Test ${testResult.id}: ${testResult.test_type}`;
    
    // Parse input parameters
    let inputHtml = '';
    if (testResult.input) {
        const params = parseInput(testResult.input);
        inputHtml = '<div class="input-params">';
        for (const [key, value] of Object.entries(params)) {
            inputHtml += `<div class="param-item"><strong>${key}:</strong> ${value}</div>`;
        }
        inputHtml += '</div>';
    }
    
    modalBody.innerHTML = `
        <div class="test-detail">
            <h3>Category</h3>
            <p>${testResult.category || 'N/A'}</p>
        </div>
        
        <div class="test-detail">
            <h3>Status</h3>
            <p>${testResult.passed ? '<span style="color: #48bb78">PASSED</span>' : '<span style="color: #f56565">FAILED</span>'}</p>
        </div>
        
        <div class="test-detail">
            <h3>Input Parameters</h3>
            ${inputHtml || '<p>No input parameters</p>'}
        </div>
        
        <div class="test-detail">
            <h3>Expected Result</h3>
            <pre>${testResult.expected_result || 'N/A'}</pre>
        </div>
        
        <div class="test-detail">
            <h3>Actual Result</h3>
            <pre>${testResult.actual_result || 'N/A'}</pre>
        </div>
        
        ${testResult.failure_reason ? `
            <div class="test-detail">
                <h3>Failure Reason</h3>
                <pre>${testResult.failure_reason}</pre>
            </div>
        ` : ''}
        
        <div class="test-detail">
            <h3>Execution Time</h3>
            <p>${testResult.execution_time_ms ? testResult.execution_time_ms.toFixed(2) + ' ms' : 'N/A'}</p>
        </div>
    `;
    
    modal.classList.add('show');
}

// Escape HTML for security
function escapeHtml(str) {
    const div = document.createElement('div');
    div.textContent = str;
    return div.innerHTML;
}

// Filter logs
function filterLogs() {
    const searchTerm = document.getElementById('log-search').value.toLowerCase();
    const levelFilter = document.getElementById('log-level-filter').value;
    
    const logWrappers = document.querySelectorAll('.log-wrapper');
    logWrappers.forEach(wrapper => {
        const logEntry = wrapper.querySelector('.log-entry');
        const text = logEntry.textContent.toLowerCase();
        const logLevel = logEntry.dataset.logLevel || '';
        
        const matchesSearch = !searchTerm || text.includes(searchTerm);
        const matchesLevel = !levelFilter || logLevel === levelFilter;
        
        wrapper.style.display = matchesSearch && matchesLevel ? 'block' : 'none';
    });
}

// Load JSON from file
function loadJsonFile(file) {
    const reader = new FileReader();
    reader.onload = (e) => {
        try {
            const data = JSON.parse(e.target.result);
            
            // Update data
            allResults = data.results || [];
            updateSummary(data.metadata);
            populateCategoryFilter();
            filteredResults = [...allResults];
            displayResults();
            createFullLogsView();
            
            // Update file name display
            document.getElementById('loaded-file-name').textContent = file.name;
            document.getElementById('session-info').textContent = `Local File: ${file.name}`;
            
            // Hide loading/error
            document.getElementById('loading').style.display = 'none';
            document.getElementById('error').style.display = 'none';
        } catch (error) {
            console.error('Error parsing JSON:', error);
            document.getElementById('error').textContent = `Error parsing JSON: ${error.message}`;
            document.getElementById('error').style.display = 'block';
        }
    };
    reader.readAsText(file);
}

// Open file in VSCode
function openInVSCode(path, line) {
    // Create VSCode URI
    // Format: vscode://file/{full_path}:{line}:{column}
    const vscodePath = path.replace(/\\/g, '/');
    const vscodeUri = `vscode://file/${vscodePath}:${line || 1}:1`;
    
    // Try to open in VSCode
    window.location.href = vscodeUri;
}

// Set up event listeners
document.addEventListener('DOMContentLoaded', () => {
    loadResults().then(() => {
        // Create full logs view after data is loaded
        createFullLogsView();
    });
    
    // Set up filters
    document.getElementById('search').addEventListener('input', filterResults);
    document.getElementById('category-filter').addEventListener('change', filterResults);
    document.getElementById('status-filter').addEventListener('change', filterResults);
    
    // Set up clickable status cards
    document.querySelectorAll('.card.clickable').forEach(card => {
        card.addEventListener('click', () => {
            const filterValue = card.dataset.filter;
            
            // Remove active class from all cards
            document.querySelectorAll('.card.clickable').forEach(c => c.classList.remove('active'));
            
            // If clicking the same card, clear filter
            const statusFilter = document.getElementById('status-filter');
            if (statusFilter.value === filterValue) {
                statusFilter.value = '';
                card.classList.remove('active');
            } else {
                // Set filter and add active class
                statusFilter.value = filterValue;
                card.classList.add('active');
            }
            
            // Trigger filter
            filterResults();
        });
    });
    
    // Sync status filter dropdown with clickable cards
    document.getElementById('status-filter').addEventListener('change', () => {
        const filterValue = document.getElementById('status-filter').value;
        
        // Remove active class from all cards
        document.querySelectorAll('.card.clickable').forEach(card => {
            card.classList.remove('active');
            
            // Add active class to matching card
            if (card.dataset.filter === filterValue) {
                card.classList.add('active');
            }
        });
    });
    
    // Set up log filters
    document.getElementById('log-search').addEventListener('input', filterLogs);
    document.getElementById('log-level-filter').addEventListener('change', filterLogs);
    
    // File upload handlers
    document.getElementById('load-json-btn').addEventListener('click', () => {
        document.getElementById('json-file-input').click();
    });
    
    document.getElementById('json-file-input').addEventListener('change', (e) => {
        const file = e.target.files[0];
        if (file) {
            loadJsonFile(file);
        }
    });
    
    // Tab switching
    document.querySelectorAll('.tab-button').forEach(button => {
        button.addEventListener('click', () => {
            // Remove active class from all tabs and contents
            document.querySelectorAll('.tab-button').forEach(b => b.classList.remove('active'));
            document.querySelectorAll('.tab-content').forEach(c => c.classList.remove('active'));
            
            // Add active class to clicked tab and corresponding content
            button.classList.add('active');
            const tabName = button.dataset.tab;
            document.getElementById(`${tabName}-tab`).classList.add('active');
            
            // Refresh logs view when switching to it
            if (tabName === 'logs') {
                createFullLogsView();
            }
        });
    });
    
    // Modal close handlers
    document.querySelector('.close-modal').addEventListener('click', () => {
        document.getElementById('test-modal').classList.remove('show');
    });
    
    document.getElementById('test-modal').addEventListener('click', (e) => {
        if (e.target.id === 'test-modal') {
            document.getElementById('test-modal').classList.remove('show');
        }
    });
    
    // Handle source path clicks (use event delegation)
    document.addEventListener('click', (e) => {
        if (e.target.classList.contains('source-path-link')) {
            e.preventDefault();
            const path = e.target.dataset.path;
            const line = e.target.dataset.line;
            openInVSCode(path, line);
        }
    });
    
    // Auto-refresh disabled to prevent closing expanded rows
    // setInterval(loadResults, 5000);
});