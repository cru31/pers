let allResults = [];
let filteredResults = [];

// Fetch and display results
async function loadResults() {
    try {
        // Get session info
        const sessionResponse = await fetch('/api/session');
        const sessionData = await sessionResponse.json();
        document.getElementById('session-info').textContent = `Session: ${sessionData.sessionId}`;

        // Get test results
        const response = await fetch('/api/results');
        if (!response.ok) {
            throw new Error('Failed to load test results');
        }

        const data = await response.json();
        allResults = data.results || [];
        
        // Update summary
        updateSummary(data.metadata);
        
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
        
        // Determine status
        let status = 'passed';
        let statusText = 'PASS';
        if (result.actual_result && result.actual_result.includes('N/A')) {
            status = 'skipped';
            statusText = 'N/A';
        } else if (!result.passed) {
            status = 'failed';
            statusText = 'FAIL';
        }
        
        row.innerHTML = `
            <td><strong>${result.id}</strong></td>
            <td>${result.category || '-'}</td>
            <td>${result.test_type || '-'}</td>
            <td><span class="status-badge status-${status}">${statusText}</span></td>
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

// Format log messages with color coding
function formatLogMessages(logs) {
    if (!logs || logs.length === 0) return '<div class="no-logs">No logs captured</div>';
    
    return logs.map(log => {
        // Escape HTML to prevent XSS
        const escapedLog = log.replace(/</g, '&lt;').replace(/>/g, '&gt;');
        
        // Apply CSS class based on log level
        if (log.includes('[TRACE]')) {
            return `<div class="log-trace">${escapedLog}</div>`;
        } else if (log.includes('[DEBUG]')) {
            return `<div class="log-debug">${escapedLog}</div>`;
        } else if (log.includes('[INFO]')) {
            return `<div class="log-info">${escapedLog}</div>`;
        } else if (log.includes('[TODO_SOMEDAY]')) {
            return `<div class="log-todo-someday">${escapedLog}</div>`;
        } else if (log.includes('[WARNING]')) {
            return `<div class="log-warning">${escapedLog}</div>`;
        } else if (log.includes('[TODO_OR_DIE]')) {
            return `<div class="log-todo-or-die">${escapedLog}</div>`;
        } else if (log.includes('[ERROR]')) {
            return `<div class="log-error">${escapedLog}</div>`;
        } else if (log.includes('[CRITICAL]')) {
            return `<div class="log-critical">${escapedLog}</div>`;
        } else {
            return `<div>${escapedLog}</div>`;
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
            const isSkipped = result.actual_result && result.actual_result.includes('N/A');
            const isPassed = result.passed && !isSkipped;
            const isFailed = !result.passed && !isSkipped;
            
            if (statusFilter === 'passed' && !isPassed) return false;
            if (statusFilter === 'failed' && !isFailed) return false;
            if (statusFilter === 'skipped' && !isSkipped) return false;
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
        
        // Determine log level and apply class
        let logClass = '';
        if (entry.log.includes('[TRACE]')) logClass = 'log-trace';
        else if (entry.log.includes('[DEBUG]')) logClass = 'log-debug';
        else if (entry.log.includes('[INFO]')) logClass = 'log-info';
        else if (entry.log.includes('[TODO_SOMEDAY]')) logClass = 'log-todo-someday';
        else if (entry.log.includes('[WARNING]')) logClass = 'log-warning';
        else if (entry.log.includes('[TODO_OR_DIE]')) logClass = 'log-todo-or-die';
        else if (entry.log.includes('[ERROR]')) logClass = 'log-error';
        else if (entry.log.includes('[CRITICAL]')) logClass = 'log-critical';
        
        logDiv.className += ' ' + logClass;
        logDiv.innerHTML = `<span class="test-id-badge">${entry.testId}</span>${escapeHtml(entry.log)}`;
        
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
            const testId = entry.testId;
            
            // Hide all other test details
            document.querySelectorAll('.log-test-details-inline').forEach(d => {
                if (d.dataset.testId !== testId) {
                    d.style.display = 'none';
                }
            });
            
            // Remove selected class from all other logs
            document.querySelectorAll('.log-entry').forEach(l => {
                if (l.dataset.testId !== testId) {
                    l.classList.remove('selected');
                }
            });
            
            // Toggle this test's details
            if (detailsDiv.style.display === 'none') {
                detailsDiv.style.display = 'block';
                logDiv.classList.add('selected');
            } else {
                detailsDiv.style.display = 'none';
                logDiv.classList.remove('selected');
            }
        });
        
        logWrapper.appendChild(logDiv);
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
    
    const logEntries = document.querySelectorAll('.log-entry');
    logEntries.forEach(entry => {
        const text = entry.textContent.toLowerCase();
        const matchesSearch = !searchTerm || text.includes(searchTerm);
        const matchesLevel = !levelFilter || text.includes('[' + levelFilter + ']');
        
        entry.style.display = matchesSearch && matchesLevel ? 'block' : 'none';
    });
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
    
    // Set up log filters
    document.getElementById('log-search').addEventListener('input', filterLogs);
    document.getElementById('log-level-filter').addEventListener('change', filterLogs);
    
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
    
    // Auto-refresh disabled to prevent closing expanded rows
    // setInterval(loadResults, 5000);
});