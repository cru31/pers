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
    if (!logs || logs.length === 0) return 'No logs captured';
    
    return logs.map(log => {
        // Apply color based on log level
        let coloredLog = log;
        if (log.includes('[TRACE]')) {
            coloredLog = `<span style="color: #888">${log}</span>`;
        } else if (log.includes('[DEBUG]')) {
            coloredLog = `<span style="color: #666">${log}</span>`;
        } else if (log.includes('[INFO]')) {
            coloredLog = `<span style="color: #333">${log}</span>`;
        } else if (log.includes('[TODO_SOMEDAY]')) {
            coloredLog = `<span style="color: #FFA500">${log}</span>`;
        } else if (log.includes('[WARNING]')) {
            coloredLog = `<span style="color: #FF8C00">${log}</span>`;
        } else if (log.includes('[TODO_OR_DIE]')) {
            coloredLog = `<span style="color: #FF4500">${log}</span>`;
        } else if (log.includes('[ERROR]')) {
            coloredLog = `<span style="color: #DC143C">${log}</span>`;
        } else if (log.includes('[CRITICAL]')) {
            coloredLog = `<span style="color: #8B0000; font-weight: bold">${log}</span>`;
        }
        return coloredLog;
    }).join('\n');
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

// Set up event listeners
document.addEventListener('DOMContentLoaded', () => {
    loadResults();
    
    // Set up filters
    document.getElementById('search').addEventListener('input', filterResults);
    document.getElementById('category-filter').addEventListener('change', filterResults);
    document.getElementById('status-filter').addEventListener('change', filterResults);
    
    // Auto-refresh disabled to prevent closing expanded rows
    // setInterval(loadResults, 5000);
});