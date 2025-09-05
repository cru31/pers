let allResults = [];
let filteredResults = [];
let sortColumn = null;
let sortDirection = 'asc';

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

// Sort results
function sortResults(column) {
    // Toggle direction if same column
    if (sortColumn === column) {
        sortDirection = sortDirection === 'asc' ? 'desc' : 'asc';
    } else {
        sortColumn = column;
        sortDirection = 'asc';
    }
    
    // Sort the filtered results
    filteredResults.sort((a, b) => {
        let aVal, bVal;
        
        switch(column) {
            case 'id':
                aVal = a.id || '';
                bVal = b.id || '';
                break;
            case 'category':
                aVal = a.category || '';
                bVal = b.category || '';
                break;
            case 'testType':
                aVal = a.testType || '';
                bVal = b.testType || '';
                break;
            case 'status':
                // Sort by status priority: passed, failed, na, nyi
                aVal = getStatusPriority(a);
                bVal = getStatusPriority(b);
                break;
            case 'time':
                aVal = a.execution_time_ms || 0;
                bVal = b.execution_time_ms || 0;
                break;
            case 'expected':
                aVal = a.expected_result || '';
                bVal = b.expected_result || '';
                break;
            case 'actual':
                aVal = a.actual_result || '';
                bVal = b.actual_result || '';
                break;
            default:
                return 0;
        }
        
        // Handle numeric vs string comparison
        if (typeof aVal === 'number' && typeof bVal === 'number') {
            return sortDirection === 'asc' ? aVal - bVal : bVal - aVal;
        } else {
            // String comparison
            aVal = String(aVal).toLowerCase();
            bVal = String(bVal).toLowerCase();
            if (aVal < bVal) return sortDirection === 'asc' ? -1 : 1;
            if (aVal > bVal) return sortDirection === 'asc' ? 1 : -1;
            return 0;
        }
    });
    
    // Update sort indicators
    updateSortIndicators();
    
    // Re-display results
    displayResults();
}

// Get status priority for sorting
function getStatusPriority(result) {
    if (result.actual_result && result.actual_result.includes('Test Not Implemented')) {
        return 3; // NA (Test NYI)
    } else if (result.passed) {
        return 0; // Passed
    } else if (result.log_messages && result.log_messages.some(log => 
               log.includes('[TODO_OR_DIE]') || log.includes('[TODO_OR_DIE ]'))) {
        return 2; // Engine NYI
    } else {
        return 1; // Failed
    }
}

// Update sort indicators in table headers
function updateSortIndicators() {
    const headers = document.querySelectorAll('th.sortable');
    headers.forEach(header => {
        const indicator = header.querySelector('.sort-indicator');
        if (indicator) {
            if (header.dataset.column === sortColumn) {
                indicator.className = `sort-indicator ${sortDirection}`;
            } else {
                indicator.className = 'sort-indicator';
            }
        }
    });
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

// Track selected test cases
let selectedTestCases = new Set();

// Display results in table
function displayResults() {
    const tbody = document.getElementById('results-body');
    tbody.innerHTML = '';
    
    if (filteredResults.length === 0) {
        tbody.innerHTML = '<tr><td colspan="8" class="no-results">No test results found</td></tr>';
        return;
    }
    
    filteredResults.forEach((result, index) => {
        // Main row
        const row = document.createElement('tr');
        row.dataset.index = index;
        
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
            <td class="checkbox-column">
                <input type="checkbox" class="test-checkbox" data-index="${index}" 
                       onclick="event.stopPropagation(); toggleTestSelection(${index})"
                       ${selectedTestCases.has(index) ? 'checked' : ''}>
            </td>
            <td onclick="toggleDetails(${index})"><strong>${result.id}</strong></td>
            <td onclick="toggleDetails(${index})">${result.category || '-'}</td>
            <td onclick="toggleDetails(${index})">${result.testType || '-'}</td>
            <td onclick="toggleDetails(${index})">${badgeHtml}</td>
            <td onclick="toggleDetails(${index})">${result.execution_time_ms ? result.execution_time_ms.toFixed(2) : '-'}</td>
            <td onclick="toggleDetails(${index})">${truncate(result.expected_result, 30)}</td>
            <td onclick="toggleDetails(${index})">${truncate(result.actual_result, 30)}</td>
        `;
        tbody.appendChild(row);
        
        // Detail row
        const detailRow = document.createElement('tr');
        detailRow.className = 'detail-row';
        detailRow.dataset.detailIndex = index;
        
        const detailCell = document.createElement('td');
        detailCell.colSpan = 8;
        
        // Parse input parameters - use input_parameters if available, otherwise fall back to input string
        let inputHtml = '';
        if (result.input_parameters) {
            inputHtml = formatInputParameters(result.input_parameters);
        } else if (result.input) {
            const params = parseInput(result.input);
            inputHtml = '<div class="input-params">';
            for (const [key, value] of Object.entries(params)) {
                inputHtml += `<div class="param-item"><strong>${key}:</strong> ${value}</div>`;
            }
            inputHtml += '</div>';
        }
        
        // Format actual properties for display
        let actualPropsHtml = '';
        if (result.actual_properties) {
            actualPropsHtml = formatInputParameters(result.actual_properties);
        }
        
        detailCell.innerHTML = `
            <div class="detail-content">
                <h4>Input Parameters</h4>
                ${inputHtml || '<p>No input parameters</p>'}
                
                <h4>Expected Result</h4>
                <pre>${result.expected_result || 'N/A'}</pre>
                
                <h4>Actual Result</h4>
                <pre>${result.actual_result || 'N/A'}</pre>
                
                ${actualPropsHtml ? `
                    <h4>Actual Properties</h4>
                    ${actualPropsHtml}
                ` : ''}
                
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
                
                <button class="generate-test-btn" onclick='openTestEditor(${JSON.stringify(result).replace(/'/g, "&apos;")})'>
                    Generate Test Case JSON
                </button>
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

// Format input parameters from JSON object
function formatInputParameters(params) {
    if (!params || typeof params !== 'object') return '';
    
    let html = '<div class="input-params">';
    
    for (const [key, value] of Object.entries(params)) {
        if (value === null || value === undefined) {
            html += `<div class="param-item"><strong>${key}:</strong> <em>null</em></div>`;
        } else if (Array.isArray(value)) {
            // Format arrays with one item per line
            let arrayContent = `<div class="param-item"><strong>${key}:</strong>`;
            value.forEach(item => {
                arrayContent += `<br>&nbsp;&nbsp;&nbsp;&nbsp;- ${item}`;
            });
            arrayContent += '</div>';
            html += arrayContent;
        } else if (typeof value === 'object') {
            // Format nested objects with indentation in a single box
            let objContent = `<div class="param-item"><strong>${key}:</strong>`;
            const entries = Object.entries(value);
            entries.forEach(([k, v]) => {
                objContent += `<br>&nbsp;&nbsp;&nbsp;&nbsp;<strong>${k}:</strong> ${v}`;
            });
            objContent += '</div>';
            html += objContent;
        } else if (typeof value === 'boolean') {
            html += `<div class="param-item"><strong>${key}:</strong> <span class="bool-value">${value}</span></div>`;
        } else {
            html += `<div class="param-item"><strong>${key}:</strong> <span class="value">${value}</span></div>`;
        }
    }
    
    html += '</div>';
    return html;
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
    
    return logs.map((log, logIndex) => {
        let logHtml = '';
        let sourceInfo = null;
        
        // New format: "[LEVEL] [Category] [fullpath:line function] message"
        // Example: "[INFO] [WebGPUInstance] [D:\path\to\file.cpp:105 namespace::class::function] Created successfully"
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
            
            // Store source info for expandable section
            sourceInfo = {
                fileName,
                filePath,
                lineNumber,
                functionName
            };
            
            // Build log without source info (will be added as expandable)
            logHtml = `${level} ${category} ${message}`;
        } else {
            logHtml = log;
        }
        
        // Parse log level and apply colored span
        const levelMatch = logHtml.match(/^\[(TRACE|DEBUG|INFO|TODO_SOMEDAY|WARNING|TODO_OR_DIE|ERROR|CRITICAL)\]/);
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
            logHtml = logHtml.replace(
                /^\[(TRACE|DEBUG|INFO|TODO_SOMEDAY|WARNING|TODO_OR_DIE|ERROR|CRITICAL)\]/,
                `<span class="log-level-inline ${logClass}">[${level}]</span>`
            );
        }
        
        // Create expandable log entry with source info
        const logId = `log-${Date.now()}-${logIndex}`;
        let html = '<div class="log-line-wrapper">';
        html += `<div class="log-line ${sourceInfo ? 'clickable-log' : ''}" ${sourceInfo ? `onclick="toggleLogSource('${logId}')"` : ''}>${logHtml}</div>`;
        
        if (sourceInfo) {
            html += `<div id="${logId}" class="log-source-details" style="display: none;">`;
            html += `<div class="source-detail"><span class="source-label">File:</span> `;
            html += `<a href="#" class="source-path-link" data-path="${sourceInfo.filePath}" data-line="${sourceInfo.lineNumber}">${sourceInfo.fileName}</a></div>`;
            html += `<div class="source-detail"><span class="source-label">Line:</span> ${sourceInfo.lineNumber}</div>`;
            html += `<div class="source-detail"><span class="source-label">Function:</span> ${sourceInfo.functionName}</div>`;
            html += '</div>';
        }
        
        html += '</div>';
        return html;
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
                result.testType,
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
                    testType: result.testType,
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
        if (entry.result.input_parameters) {
            inputHtml = '<div class="inline-params">';
            for (const [key, value] of Object.entries(entry.result.input_parameters)) {
                let valueStr = value;
                if (Array.isArray(value)) {
                    valueStr = '[' + value.join(', ') + ']';
                } else if (typeof value === 'object' && value !== null) {
                    valueStr = '{' + Object.entries(value).map(([k, v]) => `${k}: ${v}`).join(', ') + '}';
                }
                inputHtml += `<span class="param-inline"><strong>${key}:</strong> ${valueStr}</span>`;
            }
            inputHtml += '</div>';
        } else if (entry.result.input) {
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
    
    modalTitle.textContent = `Test ${testResult.id}: ${testResult.testType}`;
    
    // Parse input parameters - use input_parameters if available
    let inputHtml = '';
    if (testResult.input_parameters) {
        inputHtml = formatInputParameters(testResult.input_parameters);
    } else if (testResult.input) {
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
        
        <button class="generate-test-btn" onclick='openTestEditor(${JSON.stringify(testResult).replace(/'/g, "&apos;")})'>
            Generate Test Case JSON
        </button>
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

// Test Editor Functions
let currentTestCase = null;
let parameterCounter = 0;
let updatePreviewTimeout = null;

// Debounced update function
function scheduleJsonPreviewUpdate() {
    if (updatePreviewTimeout) {
        clearTimeout(updatePreviewTimeout);
    }
    updatePreviewTimeout = setTimeout(() => {
        updateJsonPreview();
    }, 200); // Update after 200ms of no changes
}

function openTestEditor(testResult) {
    currentTestCase = testResult;
    parameterCounter = 0;
    
    // Populate form fields
    document.getElementById('edit-id').value = testResult.id || '';
    document.getElementById('edit-category').value = testResult.category || '';
    document.getElementById('edit-test-type').value = testResult.testType || '';
    document.getElementById('edit-expected').value = testResult.expected_result || '';
    document.getElementById('edit-description').value = testResult.description || '';
    
    // Clear and populate parameters
    const container = document.getElementById('parameters-container');
    container.innerHTML = '';
    
    if (testResult.input_parameters) {
        Object.entries(testResult.input_parameters).forEach(([key, value]) => {
            addParameterRow(key, value);
        });
    }
    
    // Update JSON preview
    updateJsonPreview();
    
    // Show modal
    document.getElementById('test-editor-modal').classList.add('show');
}

function closeTestEditor() {
    document.getElementById('test-editor-modal').classList.remove('show');
}

function addParameter() {
    addParameterRow('', '');
}

function addParameterRow(key, value) {
    const container = document.getElementById('parameters-container');
    const rowDiv = document.createElement('div');
    rowDiv.className = 'parameter-row';
    rowDiv.dataset.paramId = parameterCounter++;
    
    rowDiv.innerHTML = `
        <input type="text" placeholder="Parameter name" value="${escapeHtml(key)}" oninput="scheduleJsonPreviewUpdate()">
        <input type="text" placeholder="Parameter value" value="${escapeHtml(value)}" oninput="scheduleJsonPreviewUpdate()">
        <button onclick="removeParameter(${rowDiv.dataset.paramId})" class="remove-param-btn">Remove</button>
    `;
    
    container.appendChild(rowDiv);
}

function removeParameter(id) {
    const row = document.querySelector(`.parameter-row[data-param-id="${id}"]`);
    if (row) {
        row.remove();
        updateJsonPreview(); // Update immediately when removing
    }
}

function updateJsonPreview() {
    const testCase = buildTestCaseJson();
    const previewElement = document.getElementById('json-preview-content');
    previewElement.textContent = JSON.stringify(testCase, null, 2);
}

function buildTestCaseJson() {
    // Get form values
    const id = parseInt(document.getElementById('edit-id').value) || 1;
    const category = document.getElementById('edit-category').value || 'unknown';
    const testType = document.getElementById('edit-test-type').value || 'test';
    const expected = document.getElementById('edit-expected').value || '';
    const description = document.getElementById('edit-description').value || '';
    
    // Build parameters object
    const params = {};
    document.querySelectorAll('.parameter-row').forEach(row => {
        const inputs = row.querySelectorAll('input');
        const key = inputs[0].value.trim();
        const value = inputs[1].value.trim();
        if (key) {
            // Try to parse as number or boolean
            if (value === 'true') {
                params[key] = true;
            } else if (value === 'false') {
                params[key] = false;
            } else if (!isNaN(value) && value !== '') {
                params[key] = parseFloat(value);
            } else {
                params[key] = value;
            }
        }
    });
    
    const testCase = {
        id: id,
        category: category,
        testType: testType,
        input_parameters: params,
        expected_result: expected
    };
    
    // Add description if provided
    if (description) {
        testCase.description = description;
    }
    
    return testCase;
}

function exportTestCase() {
    const testCase = buildTestCaseJson();
    const jsonStr = JSON.stringify(testCase, null, 2);
    
    // Get filename or generate random
    let filename = document.getElementById('export-filename').value.trim();
    if (!filename) {
        // Generate random 6-character hex
        const randomHex = Math.floor(Math.random() * 0xFFFFFF).toString(16).padStart(6, '0');
        filename = `testcase_${randomHex}.json`;
    } else if (!filename.endsWith('.json')) {
        filename += '.json';
    }
    
    // Create blob and download
    const blob = new Blob([jsonStr], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = filename;
    a.click();
    URL.revokeObjectURL(url);
}

function copyToClipboard() {
    const testCase = buildTestCaseJson();
    const jsonStr = JSON.stringify(testCase, null, 2);
    
    // Get the button element that was clicked
    const btn = document.querySelector('.copy-btn');
    
    // Try modern clipboard API first, fallback to older method
    if (navigator.clipboard && window.isSecureContext) {
        navigator.clipboard.writeText(jsonStr).then(() => {
            // Show feedback
            const originalText = btn.textContent;
            btn.textContent = 'Copied!';
            btn.style.background = 'linear-gradient(135deg, #48bb78, #38a169)';
            
            setTimeout(() => {
                btn.textContent = originalText;
                btn.style.background = '';
            }, 2000);
        }).catch(err => {
            console.error('Clipboard API failed:', err);
            fallbackCopyToClipboard(jsonStr, btn);
        });
    } else {
        fallbackCopyToClipboard(jsonStr, btn);
    }
}

function fallbackCopyToClipboard(text, btn) {
    // Fallback method using textarea
    const textarea = document.createElement('textarea');
    textarea.value = text;
    textarea.style.position = 'fixed';
    textarea.style.opacity = '0';
    document.body.appendChild(textarea);
    textarea.select();
    
    try {
        document.execCommand('copy');
        // Show feedback
        const originalText = btn.textContent;
        btn.textContent = 'Copied!';
        btn.style.background = 'linear-gradient(135deg, #48bb78, #38a169)';
        
        setTimeout(() => {
            btn.textContent = originalText;
            btn.style.background = '';
        }, 2000);
    } catch (err) {
        console.error('Fallback copy failed:', err);
        alert('Failed to copy to clipboard. Please copy manually from the JSON preview.');
    } finally {
        document.body.removeChild(textarea);
    }
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
    
    // Add click handlers for sortable headers
    document.querySelectorAll('th.sortable').forEach(header => {
        header.addEventListener('click', () => {
            sortResults(header.dataset.column);
        });
    });
    
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

// Checkbox selection functions
function toggleTestSelection(index) {
    if (selectedTestCases.has(index)) {
        selectedTestCases.delete(index);
    } else {
        selectedTestCases.add(index);
    }
    updateBulkExportButton();
    updateSelectAllCheckbox();
}

function toggleAllCheckboxes() {
    const selectAll = document.getElementById('select-all-checkbox');
    const checkboxes = document.querySelectorAll('.test-checkbox');
    
    if (selectAll.checked) {
        // Select all visible items
        checkboxes.forEach(cb => {
            const index = parseInt(cb.dataset.index);
            selectedTestCases.add(index);
            cb.checked = true;
        });
    } else {
        // Deselect all
        selectedTestCases.clear();
        checkboxes.forEach(cb => {
            cb.checked = false;
        });
    }
    updateBulkExportButton();
}

function updateSelectAllCheckbox() {
    const selectAll = document.getElementById('select-all-checkbox');
    const checkboxes = document.querySelectorAll('.test-checkbox');
    const checkedBoxes = document.querySelectorAll('.test-checkbox:checked');
    
    if (checkboxes.length === 0) {
        selectAll.checked = false;
        selectAll.indeterminate = false;
    } else if (checkedBoxes.length === 0) {
        selectAll.checked = false;
        selectAll.indeterminate = false;
    } else if (checkedBoxes.length === checkboxes.length) {
        selectAll.checked = true;
        selectAll.indeterminate = false;
    } else {
        selectAll.checked = false;
        selectAll.indeterminate = true;
    }
}

function updateBulkExportButton() {
    const btn = document.getElementById('bulk-export-btn');
    const count = selectedTestCases.size;
    
    if (count > 0) {
        btn.style.display = 'inline-block';
        document.getElementById('selected-count').textContent = count;
    } else {
        btn.style.display = 'none';
    }
}

// Bulk export modal functions
function openBulkExport() {
    if (selectedTestCases.size === 0) return;
    
    // Create modal if it doesn't exist
    let modal = document.getElementById('bulk-export-modal');
    if (!modal) {
        modal = createBulkExportModal();
    }
    
    // Populate with selected test cases
    populateBulkExportModal();
    modal.classList.add('show');
}

function createBulkExportModal() {
    const modalHtml = `
        <div id="bulk-export-modal" class="modal">
            <div class="modal-content bulk-export-modal">
                <span class="close-modal" onclick="closeBulkExportModal()">&times;</span>
                <h2>Export Selected Test Cases</h2>
                <div class="bulk-export-controls">
                    <button onclick="expandAllBulkItems()" class="expand-all-btn">Expand All</button>
                    <button onclick="collapseAllBulkItems()" class="collapse-all-btn">Collapse All</button>
                    <button onclick="exportBulkTestCases()" class="export-btn">Export All as JSON</button>
                    <button onclick="copyBulkToClipboard()" class="copy-btn">Copy All to Clipboard</button>
                </div>
                <div id="bulk-export-list">
                    <!-- Selected test cases will be listed here -->
                </div>
            </div>
        </div>
    `;
    
    document.body.insertAdjacentHTML('beforeend', modalHtml);
    return document.getElementById('bulk-export-modal');
}

function populateBulkExportModal() {
    const container = document.getElementById('bulk-export-list');
    container.innerHTML = '';
    
    // Convert set to array and sort by test ID
    const selectedArray = Array.from(selectedTestCases).sort((a, b) => {
        const testA = filteredResults[a];
        const testB = filteredResults[b];
        return testA.id - testB.id;
    });
    
    selectedArray.forEach(index => {
        const result = filteredResults[index];
        const itemDiv = document.createElement('div');
        itemDiv.className = 'bulk-export-item';
        itemDiv.dataset.index = index;
        
        // Create header (collapsed view)
        const headerDiv = document.createElement('div');
        headerDiv.className = 'bulk-item-header';
        headerDiv.onclick = () => toggleBulkItemExpand(index);
        headerDiv.innerHTML = `
            <span class="expand-icon">▶</span>
            <span class="test-id">Test #${result.id}</span>
            <span class="test-category">${result.category || 'N/A'}</span>
            <span class="test-type">${result.testType || 'N/A'}</span>
        `;
        
        // Create expanded content
        const contentDiv = document.createElement('div');
        contentDiv.className = 'bulk-item-content';
        contentDiv.style.display = 'none';
        
        // Build parameters HTML
        let parametersHtml = '';
        if (result.input_parameters) {
            Object.entries(result.input_parameters).forEach(([key, value]) => {
                let valueStr = value;
                let isArray = Array.isArray(value);
                if (isArray) {
                    // Format array as comma-separated string for display
                    valueStr = value.map(item => {
                        if (typeof item === 'string') {
                            return `"${item}"`;
                        }
                        return JSON.stringify(item);
                    }).join(', ');
                } else if (typeof value === 'object' && value !== null) {
                    valueStr = JSON.stringify(value);
                }
                // Escape quotes for HTML attribute
                valueStr = String(valueStr).replace(/"/g, '&quot;');
                
                // Add edit button for arrays
                const editButton = isArray ? 
                    `<button class="edit-array-btn" onclick="openArrayEditor(this.parentElement.querySelector('.param-value'), ${index}, '${key}')">Edit</button>` : '';
                
                parametersHtml += `
                    <div class="param-row">
                        <input type="text" class="param-name" value="${key}" placeholder="Parameter name">
                        <input type="text" class="param-value" value="${valueStr}" placeholder="Parameter value" 
                               data-is-array="${isArray}" data-param-key="${key}">
                        ${editButton}
                        <button class="remove-param-btn" onclick="this.parentElement.remove()">×</button>
                    </div>
                `;
            });
        }
        
        contentDiv.innerHTML = `
            <div class="bulk-item-form">
                <div class="form-group">
                    <label>Category</label>
                    <input type="text" id="bulk-category-${index}" value="${result.category || ''}">
                </div>
                <div class="form-group">
                    <label>Test Type</label>
                    <input type="text" id="bulk-test-type-${index}" value="${result.testType || ''}">
                </div>
                <div class="form-group">
                    <label>Expected Result</label>
                    <input type="text" id="bulk-expected-${index}" value="${result.expected_result || ''}">
                </div>
                <div class="form-group">
                    <label>Description</label>
                    <textarea id="bulk-description-${index}" rows="2"></textarea>
                </div>
                <div class="form-group" style="grid-column: 1 / -1;">
                    <label>Parameters <button onclick="addBulkParameter(${index})" class="add-param-btn">+ Add</button></label>
                    <div id="bulk-params-${index}" class="parameters-container">
                        ${parametersHtml}
                    </div>
                </div>
            </div>
        `;
        
        itemDiv.appendChild(headerDiv);
        itemDiv.appendChild(contentDiv);
        container.appendChild(itemDiv);
    });
}

function toggleBulkItemExpand(index) {
    const item = document.querySelector(`.bulk-export-item[data-index="${index}"]`);
    const content = item.querySelector('.bulk-item-content');
    const icon = item.querySelector('.expand-icon');
    
    if (content.style.display === 'none') {
        content.style.display = 'block';
        icon.textContent = '▼';
    } else {
        content.style.display = 'none';
        icon.textContent = '▶';
    }
}

function expandAllBulkItems() {
    document.querySelectorAll('.bulk-item-content').forEach(content => {
        content.style.display = 'block';
    });
    document.querySelectorAll('.expand-icon').forEach(icon => {
        icon.textContent = '▼';
    });
}

function collapseAllBulkItems() {
    document.querySelectorAll('.bulk-item-content').forEach(content => {
        content.style.display = 'none';
    });
    document.querySelectorAll('.expand-icon').forEach(icon => {
        icon.textContent = '▶';
    });
}

function closeBulkExportModal() {
    const modal = document.getElementById('bulk-export-modal');
    if (modal) {
        modal.classList.remove('show');
    }
}

function addBulkParameter(index) {
    const container = document.getElementById(`bulk-params-${index}`);
    const paramRow = document.createElement('div');
    paramRow.className = 'param-row';
    paramRow.innerHTML = `
        <input type="text" class="param-name" placeholder="Parameter name">
        <input type="text" class="param-value" placeholder="Parameter value">
        <button class="remove-param-btn" onclick="this.parentElement.remove()">×</button>
    `;
    container.appendChild(paramRow);
}

function exportBulkTestCases() {
    const testCases = [];
    
    // Auto-generate IDs starting from 1
    let autoId = 1;
    
    Array.from(selectedTestCases).forEach(index => {
        const result = filteredResults[index];
        
        // Get values from form if item is expanded, otherwise use defaults
        const categoryInput = document.getElementById(`bulk-category-${index}`);
        const testTypeInput = document.getElementById(`bulk-test-type-${index}`);
        const expectedInput = document.getElementById(`bulk-expected-${index}`);
        const descriptionInput = document.getElementById(`bulk-description-${index}`);
        
        const testCase = {
            id: autoId++,  // Auto-increment ID
            category: categoryInput ? categoryInput.value : (result.category || ''),
            test_type: testTypeInput ? testTypeInput.value : (result.testType || ''),
            expected_result: expectedInput ? expectedInput.value : (result.expected_result || ''),
            description: descriptionInput ? descriptionInput.value : '',
            parameters: []
        };
        
        // Collect parameters from the form
        const paramsContainer = document.getElementById(`bulk-params-${index}`);
        if (paramsContainer) {
            const paramRows = paramsContainer.querySelectorAll('.param-row');
            paramRows.forEach(row => {
                const nameInput = row.querySelector('.param-name');
                const valueInput = row.querySelector('.param-value');
                const isArray = valueInput && valueInput.dataset.isArray === 'true';
                
                if (nameInput && valueInput && nameInput.value) {
                    let value = valueInput.value.trim();
                    
                    if (isArray) {
                        // Parse comma-separated array format
                        const items = [];
                        if (value) {
                            const regex = /"([^"]*)"|\S+/g;
                            let match;
                            while ((match = regex.exec(value)) !== null) {
                                let item = match[1] !== undefined ? match[1] : match[0];
                                item = item.replace(/,$/, '');
                                if (item) {
                                    items.push(item);
                                }
                            }
                        }
                        value = items;
                    } else {
                        // Try to parse JSON objects
                        try {
                            if (value.startsWith('{')) {
                                value = JSON.parse(value);
                            }
                        } catch (e) {
                            // Keep as string if not valid JSON
                        }
                    }
                    testCase.parameters.push({ name: nameInput.value, value: value });
                }
            });
        } else if (result.input_parameters) {
            // Fallback to original parameters if form not expanded
            for (const [key, value] of Object.entries(result.input_parameters)) {
                testCase.parameters.push({ name: key, value: value });
            }
        }
        
        testCases.push(testCase);
    });
    
    // Create and download JSON file
    const jsonContent = JSON.stringify(testCases, null, 2);
    const blob = new Blob([jsonContent], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `test_cases_bulk_${new Date().toISOString().split('T')[0]}.json`;
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
    
    showNotification(`Exported ${testCases.length} test cases`);
}

function copyBulkToClipboard() {
    const testCases = [];
    
    // Auto-generate IDs starting from 1
    let autoId = 1;
    
    Array.from(selectedTestCases).forEach(index => {
        const result = filteredResults[index];
        
        // Get values from form if item is expanded, otherwise use defaults
        const categoryInput = document.getElementById(`bulk-category-${index}`);
        const testTypeInput = document.getElementById(`bulk-test-type-${index}`);
        const expectedInput = document.getElementById(`bulk-expected-${index}`);
        const descriptionInput = document.getElementById(`bulk-description-${index}`);
        
        const testCase = {
            id: autoId++,  // Auto-increment ID
            category: categoryInput ? categoryInput.value : (result.category || ''),
            test_type: testTypeInput ? testTypeInput.value : (result.testType || ''),
            expected_result: expectedInput ? expectedInput.value : (result.expected_result || ''),
            description: descriptionInput ? descriptionInput.value : '',
            parameters: []
        };
        
        // Collect parameters from the form
        const paramsContainer = document.getElementById(`bulk-params-${index}`);
        if (paramsContainer) {
            const paramRows = paramsContainer.querySelectorAll('.param-row');
            paramRows.forEach(row => {
                const nameInput = row.querySelector('.param-name');
                const valueInput = row.querySelector('.param-value');
                const isArray = valueInput && valueInput.dataset.isArray === 'true';
                
                if (nameInput && valueInput && nameInput.value) {
                    let value = valueInput.value.trim();
                    
                    if (isArray) {
                        // Parse comma-separated array format
                        const items = [];
                        if (value) {
                            const regex = /"([^"]*)"|\S+/g;
                            let match;
                            while ((match = regex.exec(value)) !== null) {
                                let item = match[1] !== undefined ? match[1] : match[0];
                                item = item.replace(/,$/, '');
                                if (item) {
                                    items.push(item);
                                }
                            }
                        }
                        value = items;
                    } else {
                        // Try to parse JSON objects
                        try {
                            if (value.startsWith('{')) {
                                value = JSON.parse(value);
                            }
                        } catch (e) {
                            // Keep as string if not valid JSON
                        }
                    }
                    testCase.parameters.push({ name: nameInput.value, value: value });
                }
            });
        } else if (result.input_parameters) {
            // Fallback to original parameters if form not expanded
            for (const [key, value] of Object.entries(result.input_parameters)) {
                testCase.parameters.push({ name: key, value: value });
            }
        }
        
        testCases.push(testCase);
    });
    
    const jsonContent = JSON.stringify(testCases, null, 2);
    
    // Try modern clipboard API first
    if (navigator.clipboard && navigator.clipboard.writeText) {
        navigator.clipboard.writeText(jsonContent)
            .then(() => showNotification(`Copied ${testCases.length} test cases to clipboard`))
            .catch(() => fallbackCopyBulk(jsonContent, testCases.length));
    } else {
        fallbackCopyBulk(jsonContent, testCases.length);
    }
}

function fallbackCopyBulk(text, count) {
    const textarea = document.createElement('textarea');
    textarea.value = text;
    textarea.style.position = 'fixed';
    textarea.style.top = '0';
    textarea.style.left = '0';
    textarea.style.width = '2em';
    textarea.style.height = '2em';
    textarea.style.padding = '0';
    textarea.style.border = 'none';
    textarea.style.outline = 'none';
    textarea.style.boxShadow = 'none';
    textarea.style.background = 'transparent';
    
    document.body.appendChild(textarea);
    textarea.select();
    
    try {
        const successful = document.execCommand('copy');
        if (successful) {
            showNotification(`Copied ${count} test cases to clipboard`);
        } else {
            showNotification('Failed to copy to clipboard', 'error');
        }
    } catch (err) {
        showNotification('Failed to copy to clipboard', 'error');
    }
    
    document.body.removeChild(textarea);
}

function showNotification(message, type = 'success') {
    // Remove existing notification if any
    const existing = document.querySelector('.notification');
    if (existing) {
        existing.remove();
    }
    
    const notification = document.createElement('div');
    notification.className = `notification notification-${type}`;
    notification.textContent = message;
    document.body.appendChild(notification);
    
    // Auto remove after 3 seconds
    setTimeout(() => {
        notification.remove();
    }, 3000);
}

// Array Editor Functions
let currentArrayInput = null;
let currentArrayItems = [];

function openArrayEditor(inputElement, testIndex, paramName) {
    currentArrayInput = inputElement;
    
    // Parse current array value
    const currentValue = inputElement.value.trim();
    try {
        // Check if it's already JSON format
        if (currentValue.startsWith('[')) {
            currentArrayItems = JSON.parse(currentValue);
        } else {
            // Parse comma-separated format like: "item1", "item2", item3
            currentArrayItems = [];
            if (currentValue) {
                // Use regex to match items, handling quoted strings
                const regex = /"([^"]*)"|\S+/g;
                let match;
                while ((match = regex.exec(currentValue)) !== null) {
                    let item = match[1] !== undefined ? match[1] : match[0];
                    // Remove trailing comma if exists
                    item = item.replace(/,$/, '');
                    if (item) {
                        currentArrayItems.push(item);
                    }
                }
            }
        }
        
        if (!Array.isArray(currentArrayItems)) {
            currentArrayItems = [];
        }
    } catch (e) {
        currentArrayItems = [];
    }
    
    // Populate array editor
    populateArrayEditor();
    
    // Show modal
    const modal = document.getElementById('array-editor-modal');
    modal.classList.add('show');
}

function populateArrayEditor() {
    const container = document.getElementById('array-items-list');
    container.innerHTML = '';
    
    if (currentArrayItems.length === 0) {
        container.innerHTML = '<div class="no-items">No items in array. Add items using the form above.</div>';
        return;
    }
    
    currentArrayItems.forEach((item, index) => {
        const itemDiv = document.createElement('div');
        itemDiv.className = 'array-item';
        
        let itemValue = item;
        if (typeof item === 'object') {
            itemValue = JSON.stringify(item);
        }
        
        itemDiv.innerHTML = `
            <span class="array-item-index">${index}:</span>
            <input type="text" class="array-item-value" value="${String(itemValue).replace(/"/g, '&quot;')}" 
                   onchange="updateArrayItem(${index}, this.value)">
            <button class="remove-array-item" onclick="removeArrayItem(${index})">Remove</button>
        `;
        
        container.appendChild(itemDiv);
    });
}

function addArrayItem() {
    const input = document.getElementById('new-array-item');
    const value = input.value.trim();
    
    if (!value) {
        showNotification('Please enter a value', 'error');
        return;
    }
    
    // Try to parse as JSON if it looks like JSON
    let parsedValue = value;
    try {
        if (value.startsWith('{') || value.startsWith('[')) {
            parsedValue = JSON.parse(value);
        }
    } catch (e) {
        // Keep as string if not valid JSON
    }
    
    currentArrayItems.push(parsedValue);
    input.value = '';
    populateArrayEditor();
}

function updateArrayItem(index, value) {
    // Try to parse as JSON if it looks like JSON
    let parsedValue = value;
    try {
        if (value.startsWith('{') || value.startsWith('[')) {
            parsedValue = JSON.parse(value);
        }
    } catch (e) {
        // Keep as string if not valid JSON
    }
    
    currentArrayItems[index] = parsedValue;
}

function removeArrayItem(index) {
    currentArrayItems.splice(index, 1);
    populateArrayEditor();
}

function saveArrayChanges() {
    if (currentArrayInput) {
        // Format array as comma-separated string for display
        const formattedValue = currentArrayItems.map(item => {
            if (typeof item === 'string') {
                return `"${item}"`;
            }
            return JSON.stringify(item);
        }).join(', ');
        
        // Update the input field with formatted value
        currentArrayInput.value = formattedValue;
        
        // Trigger change event to update preview if needed
        const changeEvent = new Event('change', { bubbles: true });
        currentArrayInput.dispatchEvent(changeEvent);
    }
    
    closeArrayEditor();
    showNotification('Array updated successfully');
}

function closeArrayEditor() {
    const modal = document.getElementById('array-editor-modal');
    modal.classList.remove('show');
    currentArrayInput = null;
    currentArrayItems = [];
}

// Toggle log source details visibility
function toggleLogSource(logId) {
    const sourceDetails = document.getElementById(logId);
    if (sourceDetails) {
        if (sourceDetails.style.display === 'none') {
            sourceDetails.style.display = 'block';
        } else {
            sourceDetails.style.display = 'none';
        }
    }
}