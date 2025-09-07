let allResults = [];
let filteredResults = [];
let sortColumn = null;
let sortDirection = 'asc';
let lastGeneratedTestId = -1; // Track last generated test ID

// Parameter value collection for autocomplete
let parameterValues = new Map(); // Map<parameterName, Set<value>>
let parameterNamesByTestType = new Map(); // Map<testType, Set<parameterNames>>
let expectedResults = new Set(); // Set of all expected results
let arrayParameters = new Set(); // Set of parameter names that are arrays
let objectParameters = new Set(); // Set of parameter names that are objects
let objectPropertyValues = new Map(); // Map<objectParam.property, Set<values>>

// Track active dropdown to handle proper closing
let activeDropdown = null;

// Track which input triggered the dropdown
let activeInput = null;
let isOpeningDropdown = false;

// Helper function to close any active dropdown
function closeActiveDropdown() {
    if (activeDropdown) {
        activeDropdown.style.display = 'none';
        activeDropdown = null;
        activeInput = null;
    }
}

// Close all dropdowns
function closeAllDropdowns() {
    if (activeDropdown) {
        activeDropdown.style.display = 'none';
        activeDropdown = null;
        activeInput = null;
    }
}

// Unified dropdown show function
function showDropdown(input, values, onSelect, filterExactMatch = true) {
    // Find dropdown element
    let dropdown;
    if (input.parentElement && input.parentElement.classList.contains('autocomplete-wrapper')) {
        dropdown = input.parentElement.querySelector('.autocomplete-dropdown');
    } else {
        // For special cases like expected-dropdown
        dropdown = document.getElementById(input.dataset.dropdownId);
    }
    
    if (!dropdown) return;
    
    // Convert Set to Array if needed
    const valuesArray = Array.isArray(values) ? values : Array.from(values);
    
    if (!valuesArray || valuesArray.length === 0) {
        dropdown.style.display = 'none';
        return;
    }
    
    const currentValue = input.value.toLowerCase();
    
    // Filter values
    let filteredValues;
    
    // If input is empty, show all values
    if (currentValue.length === 0) {
        filteredValues = valuesArray.slice(0, 10);
    } else {
        filteredValues = valuesArray.filter(v => {
            const vLower = v.toLowerCase();
            if (filterExactMatch) {
                return vLower.includes(currentValue) && vLower !== currentValue;
            }
            return vLower.includes(currentValue);
        }).slice(0, 10);
    }
    
    if (filteredValues.length === 0) {
        dropdown.style.display = 'none';
        return;
    }
    
    // Build dropdown content
    dropdown.innerHTML = '';
    filteredValues.forEach(value => {
        const item = document.createElement('div');
        item.className = 'autocomplete-item';
        item.textContent = value;
        item.onclick = () => {
            onSelect(value);
            closeAllDropdowns();
        };
        dropdown.appendChild(item);
    });
    
    // Show dropdown
    dropdown.style.display = 'block';
    activeDropdown = dropdown;
    activeInput = input;
    adjustDropdownPosition(dropdown);
}

// Global click handler to close dropdowns
document.addEventListener('click', (e) => {
    // If clicking on dropdown item, let it handle itself
    if (e.target.classList.contains('autocomplete-item')) {
        return;
    }
    
    // If not clicking on an input field, close all dropdowns
    const isInput = e.target.tagName === 'INPUT';
    if (!isInput) {
        closeAllDropdowns();
    }
});

// Common test editor functions
function createParameterRow(key = '', value = '', isArray = false, prefix = 'edit', isForArrayParam = false, isForObjectParam = false) {
    let valueStr;
    if (isArray && Array.isArray(value)) {
        valueStr = value.map(item => typeof item === 'string' ? `"${item}"` : JSON.stringify(item)).join(', ');
    } else if (typeof value === 'string') {
        // Check if it's already a JSON string
        try {
            const parsed = JSON.parse(value);
            if (typeof parsed === 'object' && parsed !== null && !Array.isArray(parsed)) {
                // It's a JSON object string, format it nicely for display
                valueStr = JSON.stringify(parsed, null, 2);
            } else if (Array.isArray(parsed)) {
                // It's a JSON array string, show as comma-separated
                valueStr = parsed.map(item => typeof item === 'string' ? `"${item}"` : JSON.stringify(item)).join(', ');
            } else {
                valueStr = String(value);
            }
        } catch (e) {
            // Not JSON, treat as regular string
            valueStr = String(value);
        }
    } else if (typeof value === 'object' && value !== null) {
        valueStr = JSON.stringify(value, null, 2);
    } else {
        valueStr = String(value);
    }
    
    const escapedValue = valueStr.replace(/"/g, '&quot;');
    
    const nameHandlers = prefix === 'edit' ? 
        `oninput="scheduleJsonPreviewUpdate()" onblur="checkForDuplicateParam(this)" onfocus="closeAllDropdowns(); showParameterNameAutocomplete(this, ${isForArrayParam}, ${isForObjectParam})"` :
        `oninput="updateBulkJsonPreview()" onblur="checkForDuplicateBulkParam(this)" onfocus="closeAllDropdowns(); showBulkParameterNameAutocomplete(this, ${isForArrayParam}, ${isForObjectParam})"`;
    
    // Determine if value is a JSON object (not array)
    const isObject = !isArray && isForObjectParam;
    
    // For arrays and objects, make the input read-only and clickable to open editor
    let valueHandlers;
    if (isArray) {
        valueHandlers = `readonly style="cursor: pointer; background: #1e1e2e;" onclick="closeAllDropdowns(); openArrayEditor(this, null, this.parentElement.parentElement.querySelector('.param-name').value)"`;
    } else if (isObject) {
        valueHandlers = `readonly style="cursor: pointer; background: #1e1e2e;" onclick="closeAllDropdowns(); openJsonObjectEditor(this, this.parentElement.parentElement.querySelector('.param-name').value)"`;
    } else {
        valueHandlers = prefix === 'edit' ? 
            `oninput="showAutocomplete(this, this.parentElement.parentElement.querySelector('.param-name').value); scheduleJsonPreviewUpdate()" onfocus="closeAllDropdowns(); showAutocomplete(this, this.parentElement.parentElement.querySelector('.param-name').value)"` :
            `oninput="showBulkAutocomplete(this)" onfocus="closeAllDropdowns(); showBulkAutocomplete(this)"`;
    }
    
    return `
        <div class="param-row">
            <div class="autocomplete-wrapper param-name-wrapper">
                <input type="text" class="param-name autocomplete-input" value="${key}" placeholder="Parameter name" 
                       ${nameHandlers}>
                <div class="autocomplete-dropdown" style="display: none;"></div>
            </div>
            <div class="autocomplete-wrapper param-value-wrapper">
                <input type="text" class="param-value autocomplete-input" value="${escapedValue}" placeholder="${isArray ? 'Click to edit array' : (isObject ? 'Click to edit object' : 'Parameter value')}" 
                       data-is-array="${isArray}" data-is-object="${isObject}" data-param-key="${key}"
                       ${valueHandlers}>
                <div class="autocomplete-dropdown" style="display: none;"></div>
            </div>
            <button class="remove-param-btn" onclick="this.parentElement.remove(); ${prefix === 'edit' ? 'scheduleJsonPreviewUpdate()' : ''}">Remove</button>
        </div>
    `;
}

// Check for duplicate parameter when focus is lost
function checkForDuplicateParam(input) {
    const paramName = input.value.trim();
    
    if (paramName) {
        // Find all parameter rows with the same name
        const duplicateInputs = [];
        document.querySelectorAll('.param-row').forEach(row => {
            const nameInput = row.querySelector('.param-name');
            if (nameInput && nameInput !== input && nameInput.value.trim() === paramName) {
                duplicateInputs.push(nameInput);
            }
        });
        
        // If there are duplicates, show message box and refocus
        if (duplicateInputs.length > 0) {
            // Store the current value to ensure it's not lost
            const currentValue = input.value;
            
            // Show alert message box
            alert(`Parameter "${paramName}" already exists!\nPlease use a different name.`);
            
            // Ensure the value is still there and return focus with cursor at end
            setTimeout(() => {
                // Restore value if somehow it was lost
                if (input.value !== currentValue) {
                    input.value = currentValue;
                }
                input.focus();
                // Set cursor position to the end of the text
                const length = input.value.length;
                input.setSelectionRange(length, length);
            }, 50);
        }
    }
}

// Handle parameter name change - just update preview
function handleParamNameChange(input) {
    scheduleJsonPreviewUpdate();
}


// Collect parameter values from all test results
function collectParameterValues(results) {
    parameterValues.clear();
    parameterNamesByTestType.clear();
    expectedResults.clear();
    arrayParameters.clear();
    objectParameters.clear();
    objectPropertyValues.clear();
    
    results.forEach(result => {
        const testType = result.testType || '';
        
        // Initialize test type entry if needed
        if (!parameterNamesByTestType.has(testType)) {
            parameterNamesByTestType.set(testType, new Set());
        }
        
        // Collect from input_parameters
        if (result.input_parameters) {
            collectValuesFromObject(result.input_parameters);
            // Collect parameter names by test type
            Object.keys(result.input_parameters).forEach(key => {
                parameterNamesByTestType.get(testType).add(key);
            });
        }
        
        // Collect from expected_behavior if it has properties
        if (result.expected_behavior && result.expected_behavior.properties) {
            collectValuesFromObject(result.expected_behavior.properties);
        }
        
        // Collect from actual_properties
        if (result.actual_properties) {
            collectValuesFromObject(result.actual_properties);
        }
        
        // Collect expected results
        if (result.expected_result) {
            expectedResults.add(result.expected_result);
        }
    });
}

// Recursively collect values from an object
function collectValuesFromObject(obj, prefix = '') {
    if (!obj || typeof obj !== 'object') return;
    
    Object.entries(obj).forEach(([key, value]) => {
        const fullKey = prefix ? `${prefix}.${key}` : key;
        
        // Track if this parameter is used as an array
        if (Array.isArray(value)) {
            arrayParameters.add(key);
        }
        
        // Always ensure the key exists in the map
        if (!parameterValues.has(key)) {
            parameterValues.set(key, new Set());
        }
        if (!parameterValues.has(fullKey)) {
            parameterValues.set(fullKey, new Set());
        }
        
        if (Array.isArray(value)) {
            // Add each array element to the set individually
            value.forEach(item => {
                if (typeof item === 'string' || typeof item === 'number' || typeof item === 'boolean') {
                    // Store individual array items for autocomplete
                    parameterValues.get(key).add(String(item));
                    parameterValues.get(fullKey).add(String(item));
                } else if (typeof item === 'object' && item !== null) {
                    // For object items in arrays, store as JSON
                    const jsonStr = JSON.stringify(item);
                    parameterValues.get(key).add(jsonStr);
                    parameterValues.get(fullKey).add(jsonStr);
                }
            });
            
            // Don't store the combined array as a single value
            // We want individual items for autocomplete
        } else if (typeof value === 'object' && value !== null) {
            // Track this as an object parameter
            objectParameters.add(key);
            
            // For objects, store the JSON representation as a suggestion
            const jsonStr = JSON.stringify(value);
            parameterValues.get(key).add(jsonStr);
            parameterValues.get(fullKey).add(jsonStr);
            
            // Collect object property names and values for autocomplete
            Object.entries(value).forEach(([propKey, propValue]) => {
                const objectPropKey = `${key}.${propKey}`;
                if (!objectPropertyValues.has(objectPropKey)) {
                    objectPropertyValues.set(objectPropKey, new Set());
                }
                if (typeof propValue === 'string' || typeof propValue === 'number' || typeof propValue === 'boolean') {
                    objectPropertyValues.get(objectPropKey).add(String(propValue));
                }
            });
            
            // Also recurse into nested objects
            collectValuesFromObject(value, fullKey);
        } else if (value !== null && value !== undefined) {
            // Add primitive values to both the simple key and full path
            parameterValues.get(key).add(String(value));
            parameterValues.get(fullKey).add(String(value));
            
            // For boolean values, always ensure both true and false are available
            if (typeof value === 'boolean') {
                parameterValues.get(key).add('true');
                parameterValues.get(key).add('false');
                parameterValues.get(fullKey).add('true');
                parameterValues.get(fullKey).add('false');
            }
        }
    });
}

// Fetch and display results
async function loadResults() {
    try {
        // Get session info
        const sessionResponse = await fetch('/api/session');
        const sessionData = await sessionResponse.json();
        document.getElementById('session-info').innerHTML = `<div style="line-height: 1.6; font-size: 0.95em;">Session: ${sessionData.sessionId}</div>`;
        
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
        
        // Initialize lastGeneratedTestId based on existing test IDs
        allResults.forEach(result => {
            const id = parseInt(result.id);
            if (!isNaN(id)) {
                lastGeneratedTestId = Math.max(lastGeneratedTestId, id);
            }
        });
        
        // Collect parameter values for autocomplete
        collectParameterValues(allResults);
        
        // Update summary
        updateSummary(data.metadata);
        
        // Display both Test Cases and Result JSON paths from metadata if available
        if (data && data.metadata && data.metadata.test_case_json) {
            const pathElement = document.getElementById('json-file-path');
            const testCasesLink = `<div style="text-align: left; margin-top: 10px; line-height: 1.6; font-size: 0.95em;"><span style="display: inline-block; width: 85px;">Test Cases:</span><a href="#" class="source-path-link" data-path="${data.metadata.test_case_json}" style="color: #667eea;">${data.metadata.test_case_json}</a></div>`;
            const resultLink = sessionData.dataPath ? `<div style="text-align: left; margin-top: 5px; line-height: 1.6; font-size: 0.95em;"><span style="display: inline-block; width: 85px;">Result:</span><a href="#" class="source-path-link" data-path="${sessionData.dataPath}" style="color: #667eea;">${sessionData.dataPath}</a></div>` : '';
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
    } else if (result.log_messages && result.log_messages.some(log => {
               // Handle both string and object formats
               if (typeof log === 'string') {
                   return log.includes('[TODO_OR_DIE]') || log.includes('[TODO_OR_DIE ]');
               } else if (typeof log === 'object' && log.level) {
                   return log.level === 'TODO_OR_DIE';
               }
               return false;
           })) {
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
        if (result.log_messages && result.log_messages.some(log => {
                   // Handle both string and object formats
                   if (typeof log === 'string') {
                       return log.includes('[TODO_OR_DIE]') || log.includes('[TODO_OR_DIE ]');
                   } else if (typeof log === 'object' && log.level) {
                       return log.level === 'TODO_OR_DIE';
                   }
                   return false;
               })) {
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
                const valueStr = String(value);
                if (valueStr.length > 50) {
                    inputHtml += `<div class="param-item"><strong>${key}:</strong> ${createTruncatedValue(value)}</div>`;
                } else {
                    inputHtml += `<div class="param-item"><strong>${key}:</strong> ${value}</div>`;
                }
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
                <div class="detail-buttons-container">
                    <button class="generate-test-btn detail-generate-btn" onclick='openTestEditor(${JSON.stringify(result).replace(/'/g, "&apos;")})' title="Generate test case JSON">
                        Generate Test Case
                    </button>
                </div>
                
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
                    <div class="log-messages">${formatLogMessages(result.log_messages, index, 0)}</div>
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

// Helper function to create truncated text with clickable tooltip
function createTruncatedValue(value, maxLength = 50) {
    const valueStr = String(value);
    if (valueStr.length <= maxLength) {
        return valueStr;
    }
    
    const truncated = valueStr.substring(0, maxLength) + '...';
    const uniqueId = 'tooltip-' + Math.random().toString(36).substr(2, 9);
    
    // Escape for display
    const escapedForHtml = valueStr.replace(/</g, '&lt;').replace(/>/g, '&gt;');
    const escapedForAttr = valueStr.replace(/"/g, '&quot;').replace(/'/g, '&#39;');
    
    return `<span class="param-value-truncated" 
                  onclick="showClickableTooltip(event, '${uniqueId}', '${escapedForAttr}')"
                  title="Click to view full text">${truncated}</span>`;
}

// Show clickable tooltip function
function showClickableTooltip(event, uniqueId, fullValue) {
    event.stopPropagation();
    
    // Close any existing tooltip
    const existingTooltip = document.querySelector('.param-tooltip-window');
    if (existingTooltip) {
        existingTooltip.remove();
    }
    
    // Decode the escaped value
    const decodedValue = fullValue
        .replace(/&quot;/g, '"')
        .replace(/&#39;/g, "'")
        .replace(/&lt;/g, '<')
        .replace(/&gt;/g, '>');
    
    // Create new tooltip window - simplified
    const tooltip = document.createElement('div');
    tooltip.className = 'param-tooltip-window';
    tooltip.innerHTML = `
        <textarea class="tooltip-text" readonly>${decodedValue}</textarea>
    `;
    
    document.body.appendChild(tooltip);
    
    // Position the tooltip near the clicked element
    const rect = event.target.getBoundingClientRect();
    const tooltipRect = tooltip.getBoundingClientRect();
    
    let left = rect.left;
    let top = rect.bottom + 5;
    
    // Adjust if tooltip goes off screen
    if (left + tooltipRect.width > window.innerWidth) {
        left = window.innerWidth - tooltipRect.width - 10;
    }
    if (top + tooltipRect.height > window.innerHeight) {
        top = rect.top - tooltipRect.height - 5;
    }
    
    tooltip.style.left = left + 'px';
    tooltip.style.top = top + 'px';
    
    // Close tooltip when clicking outside
    setTimeout(() => {
        document.addEventListener('click', function closeTooltip(e) {
            if (!tooltip.contains(e.target)) {
                tooltip.remove();
                document.removeEventListener('click', closeTooltip);
            }
        });
    }, 100);
}

// Copy tooltip text function
function copyTooltipText(button) {
    const textarea = button.parentElement.querySelector('.tooltip-text');
    textarea.select();
    document.execCommand('copy');
    
    // Show feedback
    const originalText = button.textContent;
    button.textContent = 'Copied!';
    button.style.background = '#48bb78';
    
    setTimeout(() => {
        button.textContent = originalText;
        button.style.background = '';
    }, 1500);
}

// Format input parameters from JSON object
function formatInputParameters(params) {
    if (!params || typeof params !== 'object') return '';
    
    let html = '<div class="input-params">';
    
    for (const [key, value] of Object.entries(params)) {
        if (value === null || value === undefined) {
            html += `<div class="param-item"><strong>${key}:</strong> <em>null</em></div>`;
        } else if (Array.isArray(value)) {
            // Format arrays with truncation for long items
            const arrayStr = JSON.stringify(value);
            if (arrayStr.length > 100) {
                // For long arrays, show truncated version with tooltip
                html += `<div class="param-item"><strong>${key}:</strong> ${createTruncatedValue(arrayStr, 80)}</div>`;
            } else {
                let arrayContent = `<div class="param-item"><strong>${key}:</strong>`;
                value.forEach(item => {
                    const itemStr = String(item);
                    if (itemStr.length > 50) {
                        arrayContent += `<br>&nbsp;&nbsp;&nbsp;&nbsp;- ${createTruncatedValue(item)}`;
                    } else {
                        arrayContent += `<br>&nbsp;&nbsp;&nbsp;&nbsp;- ${item}`;
                    }
                });
                arrayContent += '</div>';
                html += arrayContent;
            }
        } else if (typeof value === 'object') {
            // Format nested objects in readable multi-line format
            let objContent = `<div class="param-item"><strong>${key}:</strong> {`;
            const entries = Object.entries(value);
            entries.forEach(([k, v], index) => {
                const vStr = String(v);
                const comma = index < entries.length - 1 ? ',' : '';
                if (vStr.length > 50) {
                    objContent += `<br>&nbsp;&nbsp;${k}: ${createTruncatedValue(v)}${comma}`;
                } else {
                    objContent += `<br>&nbsp;&nbsp;${k}: ${v}${comma}`;
                }
            });
            objContent += '<br>}</div>';
            html += objContent;
        } else if (typeof value === 'boolean') {
            html += `<div class="param-item"><strong>${key}:</strong> <span class="bool-value">${value}</span></div>`;
        } else {
            const valueStr = String(value);
            if (valueStr.length > 50) {
                html += `<div class="param-item"><strong>${key}:</strong> ${createTruncatedValue(value)}</div>`;
            } else {
                html += `<div class="param-item"><strong>${key}:</strong> <span class="value">${value}</span></div>`;
            }
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
function formatLogMessages(logs, testIndex, variationIndex) {
    if (!logs || logs.length === 0) return '<div class="no-logs">No logs captured</div>';
    
    return logs.map((log, logIndex) => {
        let logHtml = '';
        let sourceInfo = null;
        
        // New format: "[LEVEL] [Category] [fullpath:line function] message"
        // Example: "[INFO] [WebGPUInstance] [D:\path\to\file.cpp:105 namespace::class::function] Created successfully"
        // Check if log is object or string
        let newFormatMatch = null;
        if (typeof log === 'object' && log !== null) {
            // New structured format
            const timestamp = log.timestamp || '';
            const level = log.level || 'UNKNOWN';
            const category = log.category || '';
            const message = log.message || '';
            
            // Store source info if available
            if (log.file || log.function) {
                const fileName = log.file ? log.file.split(/[\/\\]/).pop() : '';
                sourceInfo = {
                    fileName: fileName,
                    filePath: log.file || '',
                    lineNumber: log.line || 0,
                    functionName: log.function || ''
                };
            }
            
            // Build log HTML without timestamp (will be added separately)
            logHtml = `[${level}] <span class="log-category">[${category}]</span> ${message}`;
            
            // Store timestamp separately for later use
            sourceInfo = sourceInfo || {};
            sourceInfo.timestamp = timestamp;
            
        } else if (typeof log === 'string') {
            newFormatMatch = log.match(/^(\[[^\]]+\])\s*(\[[^\]]+\])\s*\[([^:\]]+(?::[^:\]]+)?):(\d+)\s+([^\]]+)\]\s*(.*)$/);
            
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
                // String log without special format
                logHtml = log;
            }
        }
        
        // Parse log level and structure the output
        const levelMatch = (typeof logHtml === 'string') ? logHtml.match(/^\[(TRACE|DEBUG|INFO|TODO_SOMEDAY|WARNING|TODO_OR_DIE|ERROR|CRITICAL)\]\s*(.*)$/) : null;
        if (levelMatch) {
            const level = levelMatch[1];
            const restOfLog = levelMatch[2];
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
            
            // Structure with fixed-width level and message - truncate long levels to 8 chars
            const truncatedLevel = level.length > 8 ? level.substring(0, 8) : level;
            logHtml = `<span class="log-level-inline ${logClass}" title="[${level}]">[${truncatedLevel}]</span><span class="log-message-text">${restOfLog}</span>`;
        }
        
        // Create expandable log entry with source info
        const logId = `log-${testIndex}-${variationIndex}-${logIndex}`;
        let html = '<div class="log-line-wrapper">';
        
        // Add timestamp if available
        let fullLogHtml = logHtml;
        if (sourceInfo && sourceInfo.timestamp) {
            fullLogHtml = `<span class="log-timestamp">${sourceInfo.timestamp}</span> ${logHtml}`;
        }
        
        html += `<div class="log-line ${sourceInfo ? 'clickable-log' : ''}" ${sourceInfo ? `onclick="toggleLogSource('${logId}')"` : ''}>${fullLogHtml}</div>`;
        
        if (sourceInfo) {
            html += `<div id="${logId}" class="log-source-details" style="display: none;">`;
            html += `<div class="source-detail"><span class="source-label">File:</span> `;
            html += `<a href="#" class="source-path-link" data-path="${sourceInfo.filePath}" data-line="${sourceInfo.lineNumber}">${sourceInfo.filePath}</a></div>`;
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
                hasSelectedStatus = result.log_messages && result.log_messages.some(log => {
                                   // Handle both string and object formats
                                   if (typeof log === 'string') {
                                       return log.includes('[TODO_OR_DIE]') || log.includes('[TODO_OR_DIE ]');
                                   } else if (typeof log === 'object' && log.level) {
                                       return log.level === 'TODO_OR_DIE';
                                   }
                                   return false;
                               });
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
        let timestamp = '';
        
        // Check if log is object or string
        if (typeof entry.log === 'object' && entry.log !== null) {
            // New structured format
            timestamp = entry.log.timestamp || '';
            logLevel = entry.log.level || 'UNKNOWN';
            logCategory = entry.log.category || '';
            logMessage = entry.log.message || '';
            logFile = entry.log.file || '';
            logLine = entry.log.line || '';
            logFunction = entry.log.function || '';
        } else if (typeof entry.log === 'string') {
            // Extract log level from string
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
        
        // Store log level for filtering
        logDiv.dataset.logLevel = logLevel;
        
        // Apply the log level class to get the color with fixed width - truncate to 8 chars
        const truncatedLevel = (logLevel || 'INFO').length > 8 ? (logLevel || 'INFO').substring(0, 8) : (logLevel || 'INFO');
        const levelSpan = `<span class="log-level-badge ${logClass}" title="[${logLevel || 'INFO'}]">[${truncatedLevel}]</span>`;
        const categorySpan = logCategory ? `<span class="log-category">${escapeHtml(logCategory)}:</span>` : '';
        const messageSpan = `<span class="log-message">${escapeHtml(logMessage)}</span>`;
        
        const timestampSpan = timestamp ? `<span class="log-timestamp">${timestamp}</span>` : '';
        logDiv.innerHTML = `<span class="test-id-badge">${entry.testId}</span>${timestampSpan}<span class="log-content-aligned">${levelSpan}${categorySpan} ${messageSpan}</span>`;
        
        // Create source info container (hidden by default)
        const sourceDiv = document.createElement('div');
        sourceDiv.className = 'log-source-info';
        sourceDiv.style.display = 'none';
        
        // Extract just the filename from the full path
        const fileName = logFile ? logFile.split('\\').pop() : '';
        
        sourceDiv.innerHTML = `
            <div class="source-detail">
                <span class="source-label">File:</span> 
                ${logFile ? `<a href="#" class="source-path-link" data-path="${escapeHtml(logFile)}" data-line="${escapeHtml(logLine)}">${escapeHtml(logFile)}</a>` : 'N/A'}
            </div>
            <div class="source-detail">
                <span class="source-label">Line:</span> ${escapeHtml(logLine || 'N/A')}
            </div>
            <div class="source-detail">
                <span class="source-label">Function:</span> ${escapeHtml(logFunction || 'N/A')}
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
                    // Format object in multi-line format for inline display
                    const entries = Object.entries(value);
                    if (entries.length <= 3) {
                        // For small objects, show in one line
                        valueStr = '{' + entries.map(([k, v]) => `${k}: ${v}`).join(', ') + '}';
                    } else {
                        // For larger objects, truncate and use tooltip
                        const preview = '{' + entries.slice(0, 2).map(([k, v]) => `${k}: ${v}`).join(', ') + ', ...}';
                        const fullStr = JSON.stringify(value, null, 2);
                        valueStr = createTruncatedValue(fullStr, preview.length);
                    }
                }
                // Apply truncation for long values
                const displayValue = createTruncatedValue(valueStr);
                inputHtml += `<span class="param-inline"><strong>${key}:</strong> ${displayValue}</span>`;
            }
            inputHtml += '</div>';
        } else if (entry.result.input) {
            const params = parseInput(entry.result.input);
            inputHtml = '<div class="inline-params">';
            for (const [key, value] of Object.entries(params)) {
                // Apply truncation for long values
                const displayValue = createTruncatedValue(value);
                inputHtml += `<span class="param-inline"><strong>${key}:</strong> ${displayValue}</span>`;
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
    
    modalTitle.innerHTML = `Test ${testResult.id}: ${testResult.testType}
        <button class="generate-test-btn modal-generate-btn" onclick='openTestEditor(${JSON.stringify(testResult).replace(/'/g, "&apos;")})'>
            Generate Test Case JSON
        </button>`;
    
    // Parse input parameters - use input_parameters if available
    let inputHtml = '';
    if (testResult.input_parameters) {
        inputHtml = formatInputParameters(testResult.input_parameters);
    } else if (testResult.input) {
        const params = parseInput(testResult.input);
        inputHtml = '<div class="input-params">';
        for (const [key, value] of Object.entries(params)) {
            // Apply truncation for long values
            const displayValue = createTruncatedValue(value);
            inputHtml += `<div class="param-item"><strong>${key}:</strong> ${displayValue}</div>`;
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
            
            // Initialize lastGeneratedTestId based on existing test IDs
            allResults.forEach(result => {
                const id = parseInt(result.id);
                if (!isNaN(id)) {
                    lastGeneratedTestId = Math.max(lastGeneratedTestId, id);
                }
            });
            
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

function openTestEditor(testResult, mode = "testcase") {
    // Create a clean test case based on mode
    let cleanTestCase;
    
    // Convert from result format to test case format
    const testCaseParams = testResult.input_parameters || {};
    
    cleanTestCase = {
        id: testResult.id,
        variationName: testResult.description || testResult.testType || `Test ${testResult.id}`,
        options: testCaseParams,
        expectedBehavior: {
            returnValue: testResult.expected_result || "success"
        }
    };
    
    // Store category and testType separately for the form
    cleanTestCase.category = testResult.category;
    cleanTestCase.testType = testResult.testType;
    
    currentTestCase = cleanTestCase;
    parameterCounter = 0;
    
    // Populate form fields
    const idInput = document.getElementById('edit-id');
    
    // Check if ID is empty or non-numeric, then show next available ID
    if (!cleanTestCase.id || cleanTestCase.id === '' || cleanTestCase.id === null || isNaN(parseInt(cleanTestCase.id))) {
        const generatedId = String(lastGeneratedTestId + 1);
        idInput.value = generatedId;
    } else {
        idInput.value = cleanTestCase.id;
    }
    document.getElementById('edit-category').value = cleanTestCase.category || '';
    document.getElementById('edit-test-type').value = cleanTestCase.testType || '';
    
    // Set up expected result with autocomplete
    const expectedInput = document.getElementById('edit-expected');
    expectedInput.value = cleanTestCase.expectedBehavior?.returnValue || '';
    
    // Add autocomplete handlers to expected result
    expectedInput.oninput = () => {
        scheduleJsonPreviewUpdate();
        showExpectedResultAutocomplete(expectedInput);
    };
    expectedInput.onfocus = () => {
        closeAllDropdowns();
        showExpectedResultAutocomplete(expectedInput);
    };
    
    document.getElementById('edit-description').value = cleanTestCase.variationName || '';
    
    // Clear ALL parameter containers
    const singleContainer = document.getElementById('single-parameters-container');
    const arrayContainer = document.getElementById('array-parameters-container');
    const objectContainer = document.getElementById('object-parameters-container');
    
    // Clear all containers
    if (singleContainer) singleContainer.innerHTML = '';
    if (arrayContainer) arrayContainer.innerHTML = '';
    if (objectContainer) objectContainer.innerHTML = '';
    
    // Only populate parameters if we have options
    if (cleanTestCase.options) {
        // For backward compatibility, check if old container exists
        const oldContainer = document.getElementById('parameters-container');
        if (oldContainer && (!singleContainer || !arrayContainer)) {
            // Old structure, use old container
            oldContainer.innerHTML = '';
            Object.entries(cleanTestCase.options).forEach(([key, value]) => {
                const isArray = Array.isArray(value);
                const rowHtml = createParameterRow(key, value, isArray, 'edit', isArray);
                const tempDiv = document.createElement('div');
                tempDiv.innerHTML = rowHtml;
                oldContainer.appendChild(tempDiv.firstElementChild);
            });
        } else if (singleContainer && arrayContainer) {
            // New structure with separated containers
            Object.entries(cleanTestCase.options).forEach(([key, value]) => {
                const isArray = Array.isArray(value);
                const isObject = typeof value === 'object' && value !== null && !Array.isArray(value);
                
                if (isArray) {
                    // Add to array container
                    addArrayParameterWithValue(key, value);
                } else if (isObject) {
                    // Add to object container
                    addObjectParameterWithValue(key, value);
                } else {
                    // Add to single value container
                    addParameterWithValue(key, value);
                }
            });
        }
    }
    
    // Update JSON preview
    updateJsonPreview();
    
    // Show modal
    document.getElementById('test-editor-modal').classList.add('show');
    document.body.classList.add('modal-open');
    
    // Adjust preview height after modal is shown
    setTimeout(() => {
        const previewElement = document.getElementById('json-preview-content');
        if (previewElement) {
            adjustTextareaHeight(previewElement);
        }
    }, 50);
}

function closeTestEditor() {
    document.getElementById('test-editor-modal').classList.remove('show');
    document.body.classList.remove('modal-open');
}

function addParameter() {
    const container = document.getElementById('single-parameters-container');
    const rowHtml = createParameterRow('', '', false, 'edit', false, false);
    
    // Create a temporary div to parse the HTML
    const tempDiv = document.createElement('div');
    tempDiv.innerHTML = rowHtml;
    
    // Add change handler to check for duplicates
    const newRow = tempDiv.firstElementChild;
    const nameInput = newRow.querySelector('.param-name');
    nameInput.addEventListener('blur', () => checkForDuplicateParameter(nameInput));
    
    // Append the parsed element to the container
    container.appendChild(newRow);
    scheduleJsonPreviewUpdate();
}

function addArrayParameter() {
    const container = document.getElementById('array-parameters-container');
    const rowHtml = createParameterRow('', '', true, 'edit', true, false);
    
    // Create a temporary div to parse the HTML
    const tempDiv = document.createElement('div');
    tempDiv.innerHTML = rowHtml;
    
    // Add change handler to check for duplicates
    const newRow = tempDiv.firstElementChild;
    const nameInput = newRow.querySelector('.param-name');
    nameInput.addEventListener('blur', () => checkForDuplicateParameter(nameInput));
    
    // Append the parsed element to the container
    container.appendChild(newRow);
    scheduleJsonPreviewUpdate();
}

function addObjectParameter() {
    const container = document.getElementById('object-parameters-container');
    const rowHtml = createParameterRow('', '{}', false, 'edit', false, true);
    
    // Create a temporary div to parse the HTML
    const tempDiv = document.createElement('div');
    tempDiv.innerHTML = rowHtml;
    
    // Add change handler to check for duplicates
    const newRow = tempDiv.firstElementChild;
    const nameInput = newRow.querySelector('.param-name');
    nameInput.addEventListener('blur', () => checkForDuplicateParameter(nameInput));
    
    // Append the parsed element to the container
    container.appendChild(newRow);
    scheduleJsonPreviewUpdate();
}

// Helper functions to add parameters with values (for loading actual results)
function addParameterWithValue(key, value) {
    const container = document.getElementById('single-parameters-container');
    const rowHtml = createParameterRow(key, value, false, 'edit', false, false);
    
    const tempDiv = document.createElement('div');
    tempDiv.innerHTML = rowHtml;
    
    // Add blur handler for duplicate checking
    const newRow = tempDiv.firstElementChild;
    const nameInput = newRow.querySelector('.param-name');
    if (nameInput) {
        nameInput.addEventListener('blur', () => checkForDuplicateParam(nameInput));
    }
    
    container.appendChild(newRow);
    scheduleJsonPreviewUpdate();
}

function addArrayParameterWithValue(key, value) {
    const container = document.getElementById('array-parameters-container');
    // Convert value to proper format if it's an array
    let displayValue = value;
    if (Array.isArray(value)) {
        displayValue = JSON.stringify(value);
    }
    const rowHtml = createParameterRow(key, displayValue, true, 'edit', true, false);
    
    const tempDiv = document.createElement('div');
    tempDiv.innerHTML = rowHtml;
    
    // Add blur handler for duplicate checking
    const newRow = tempDiv.firstElementChild;
    const nameInput = newRow.querySelector('.param-name');
    if (nameInput) {
        nameInput.addEventListener('blur', () => checkForDuplicateParam(nameInput));
    }
    
    // Store the actual array value in the data attribute
    const valueInput = newRow.querySelector('.param-value');
    if (valueInput && Array.isArray(value)) {
        valueInput.dataset.arrayValue = JSON.stringify(value);
    }
    
    container.appendChild(newRow);
    scheduleJsonPreviewUpdate();
}

function addObjectParameterWithValue(key, value) {
    const container = document.getElementById('object-parameters-container');
    // Convert value to proper format if it's an object
    let displayValue = value;
    if (typeof value === 'object' && value !== null) {
        displayValue = JSON.stringify(value);
    }
    const rowHtml = createParameterRow(key, displayValue, false, 'edit', false, true);
    
    const tempDiv = document.createElement('div');
    tempDiv.innerHTML = rowHtml;
    
    // Add blur handler for duplicate checking
    const newRow = tempDiv.firstElementChild;
    const nameInput = newRow.querySelector('.param-name');
    if (nameInput) {
        nameInput.addEventListener('blur', () => checkForDuplicateParam(nameInput));
    }
    
    // Store the actual object value in the data attribute
    const valueInput = newRow.querySelector('.param-value');
    if (valueInput && typeof value === 'object') {
        valueInput.dataset.objectValue = JSON.stringify(value);
    }
    
    container.appendChild(newRow);
    scheduleJsonPreviewUpdate();
}

function checkForDuplicateParameter(input) {
    const paramName = input.value.trim();
    if (!paramName) return;
    
    // Get all parameter names from both containers
    const allParamNames = [];
    document.querySelectorAll('.param-row').forEach(row => {
        const nameInput = row.querySelector('.param-name');
        if (nameInput && nameInput !== input) {
            const name = nameInput.value.trim();
            if (name) allParamNames.push(name);
        }
    });
    
    // Check for duplicate
    if (allParamNames.includes(paramName)) {
        showNotification(`Parameter "${paramName}" already exists!`, 'error');
        input.value = '';
        input.focus();
    }
}

function addParameterRow(key, value, isArray = false) {
    const container = isArray ? 
        document.getElementById('array-parameters-container') : 
        document.getElementById('single-parameters-container');
    
    if (!container) return; // Safety check for old code
    
    const rowHtml = createParameterRow(key, value, isArray, 'edit', isArray);
    
    // Create a temporary div to parse the HTML
    const tempDiv = document.createElement('div');
    tempDiv.innerHTML = rowHtml;
    
    // Append the parsed element to the container
    container.appendChild(tempDiv.firstElementChild);
    
    scheduleJsonPreviewUpdate();
}

function removeParameter(id) {
    const row = document.querySelector(`.parameter-row[data-param-id="${id}"]`);
    if (row) {
        row.remove();
        updateJsonPreview(); // Update immediately when removing
    }
}

// Show autocomplete dropdown with suggestions
function showAutocomplete(input, parameterName) {
    // Get values for this parameter
    const values = parameterValues.get(parameterName) || new Set();
    
    // If no parameter name or no values, don't show dropdown
    if (!parameterName || values.size === 0) {
        const wrapper = input.parentElement;
        const dropdown = wrapper.querySelector('.autocomplete-dropdown');
        if (dropdown) {
            dropdown.style.display = 'none';
        }
        return;
    }
    
    showDropdown(input, values, (value) => {
        input.value = value;
        scheduleJsonPreviewUpdate();
    });
}

// Hide autocomplete dropdown
function hideAutocomplete(input) {
    const wrapper = input.parentElement;
    const dropdown = wrapper.querySelector('.autocomplete-dropdown');
    if (dropdown) {
        dropdown.style.display = 'none';
    }
}

// Update value autocomplete when parameter name changes
function updateValueAutocomplete(rowDiv) {
    const nameInput = rowDiv.querySelector('input[type="text"]:first-child');
    const valueInput = rowDiv.querySelector('.autocomplete-input');
    
    if (nameInput && valueInput) {
        // Clear and update suggestions based on new parameter name
        hideAutocomplete(valueInput);
    }
}

// Show bulk autocomplete dropdown with suggestions
function showBulkAutocomplete(input) {
    const paramRow = input.closest('.param-row');
    const nameInput = paramRow.querySelector('.param-name');
    const parameterName = nameInput ? nameInput.value : '';
    
    // Get values for this parameter
    const values = parameterValues.get(parameterName) || [];
    
    showDropdown(input, values, (value) => {
        input.value = value;
        if (typeof updateBulkJsonPreview === 'function') {
            updateBulkJsonPreview();
        }
    });
}

// Hide bulk autocomplete dropdown
function hideBulkAutocomplete(input) {
    const wrapper = input.parentElement;
    const dropdown = wrapper.querySelector('.autocomplete-dropdown');
    if (dropdown) {
        dropdown.style.display = 'none';
    }
}

// Check for duplicate bulk parameter when focus is lost
function checkForDuplicateBulkParam(input) {
    const paramName = input.value.trim();
    
    if (paramName) {
        // Find all parameter rows with the same name in the same bulk export item
        const container = input.closest('.bulk-export-item');
        if (container) {
            const duplicateInputs = [];
            container.querySelectorAll('.param-row').forEach(row => {
                const nameInput = row.querySelector('.param-name');
                if (nameInput && nameInput !== input && nameInput.value.trim() === paramName) {
                    duplicateInputs.push(nameInput);
                }
            });
            
            // If there are duplicates, show message box and refocus
            if (duplicateInputs.length > 0) {
                // Store the current value to ensure it's not lost
                const currentValue = input.value;
                
                // Show alert message box
                alert(`Parameter "${paramName}" already exists!\nPlease use a different name.`);
                
                // Ensure the value is still there and return focus with cursor at end
                setTimeout(() => {
                    // Restore value if somehow it was lost
                    if (input.value !== currentValue) {
                        input.value = currentValue;
                    }
                    input.focus();
                    // Set cursor position to the end of the text
                    const length = input.value.length;
                    input.setSelectionRange(length, length);
                }, 50);
            }
        }
    }
}

// Handle bulk parameter name change - just update preview
function handleBulkParamNameChange(input) {
    // Update JSON preview if in bulk export
    if (typeof updateBulkJsonPreview === 'function') {
        updateBulkJsonPreview();
    }
}

// Show bulk parameter name autocomplete
function showBulkParameterNameAutocomplete(input, isArrayParam = false, isObjectParam = false) {
    // Find the test type for this bulk item
    const bulkItem = input.closest('.bulk-export-item');
    const index = bulkItem ? bulkItem.dataset.index : null;
    let testType = '';
    
    if (index !== null) {
        const testTypeInput = document.getElementById(`bulk-test-type-${index}`);
        testType = testTypeInput ? testTypeInput.value : '';
    }
    
    // Get parameter names for this test type
    let parameterNames = parameterNamesByTestType.get(testType) || new Set();
    
    // Filter based on parameter type
    if (isArrayParam) {
        parameterNames = new Set([...parameterNames].filter(name => arrayParameters.has(name)));
    } else if (isObjectParam) {
        parameterNames = new Set([...parameterNames].filter(name => objectParameters.has(name)));
    } else {
        // For single value parameters, exclude both array and object parameters
        parameterNames = new Set([...parameterNames].filter(name => 
            !arrayParameters.has(name) && !objectParameters.has(name)));
    }
    
    // Filter out already used parameters in bulk export
    const usedParams = new Set();
    const bulkExportItems = document.querySelectorAll('.bulk-export-item');
    if (bulkExportItems.length > 0) {
        // We're in bulk export modal
        const currentItem = input.closest('.bulk-export-item');
        currentItem?.querySelectorAll('.param-row').forEach(row => {
            const nameInput = row.querySelector('.param-name');
            if (nameInput && nameInput !== input) {
                const name = nameInput.value.trim();
                if (name) usedParams.add(name);
            }
        });
    }
    
    // Filter available names
    const availableNames = Array.from(parameterNames).filter(name => !usedParams.has(name));
    
    showDropdown(input, availableNames, (value) => {
        input.value = value;
        handleBulkParamNameChange(input);
        if (typeof updateBulkJsonPreview === 'function') {
            updateBulkJsonPreview();
        }
    });
}

// Hide bulk parameter name dropdown
function hideBulkParameterNameDropdown(input) {
    const wrapper = input.parentElement;
    const dropdown = wrapper.querySelector('.autocomplete-dropdown');
    if (dropdown) {
        dropdown.style.display = 'none';
    }
}

// Show parameter name autocomplete
function showParameterNameAutocomplete(input, isArrayParam = false, isObjectParam = false) {
    // Get current test type from the form
    const testTypeInput = document.getElementById('edit-test-type');
    const testType = testTypeInput ? testTypeInput.value : '';
    
    // Get parameter names for this test type
    let parameterNames = parameterNamesByTestType.get(testType) || new Set();
    
    // Filter based on parameter type
    if (isArrayParam) {
        parameterNames = new Set([...parameterNames].filter(name => arrayParameters.has(name)));
    } else if (isObjectParam) {
        parameterNames = new Set([...parameterNames].filter(name => objectParameters.has(name)));
    } else {
        // For single value parameters, exclude both array and object parameters
        parameterNames = new Set([...parameterNames].filter(name => 
            !arrayParameters.has(name) && !objectParameters.has(name)));
    }
    
    // Filter out already used parameter names
    const usedParamNames = [];
    document.querySelectorAll('.param-row').forEach(row => {
        const nameInput = row.querySelector('.param-name');
        if (nameInput && nameInput !== input) {
            const name = nameInput.value.trim();
            if (name) usedParamNames.push(name);
        }
    });
    
    // Remove used parameters from suggestions
    parameterNames = new Set([...parameterNames].filter(name => !usedParamNames.includes(name)));
    
    showDropdown(input, parameterNames, (value) => {
        // Check for duplicate one more time before setting
        if (usedParamNames.includes(value)) {
            showNotification(`Parameter "${value}" already exists!`, 'error');
            return;
        }
        
        input.value = value;
        scheduleJsonPreviewUpdate();
    });
}

// Show expected result autocomplete
function showExpectedResultAutocomplete(input) {
    input.dataset.dropdownId = 'expected-dropdown';
    showDropdown(input, expectedResults, (value) => {
        input.value = value;
        scheduleJsonPreviewUpdate();
    });
}

// Hide expected result autocomplete
function hideExpectedAutocomplete(input) {
    const dropdown = document.getElementById('expected-dropdown');
    if (dropdown) {
        dropdown.style.display = 'none';
    }
}

// Show array item autocomplete
function showArrayItemAutocomplete(input) {
    // Get values for current parameter
    const values = parameterValues.get(currentArrayParamName) || new Set();
    
    showDropdown(input, values, (value) => {
        input.value = value;
        // Update array item
        const index = parseInt(input.closest('.array-item').querySelector('.array-item-index').textContent);
        updateArrayItem(index, value);
    });
}

// Hide array item autocomplete
function hideArrayItemAutocomplete(input) {
    const wrapper = input.parentElement;
    const dropdown = wrapper.querySelector('.autocomplete-dropdown');
    if (dropdown) {
        dropdown.style.display = 'none';
    }
}

// Show array new item autocomplete
function showArrayNewItemAutocomplete(input) {
    input.dataset.dropdownId = 'array-new-item-dropdown';
    // Get values for current parameter
    const values = parameterValues.get(currentArrayParamName) || new Set();
    
    showDropdown(input, values, (value) => {
        input.value = value;
    });
}

// Hide array new item autocomplete
function hideArrayNewItemAutocomplete(input) {
    const dropdown = document.getElementById('array-new-item-dropdown');
    if (dropdown) {
        dropdown.style.display = 'none';
    }
}

// Show bulk expected result autocomplete
function showBulkExpectedAutocomplete(input) {
    showDropdown(input, expectedResults, (value) => {
        input.value = value;
    }, false); // Don't filter exact matches in bulk mode
}

// Hide bulk expected autocomplete
function hideBulkExpectedAutocomplete(input) {
    const wrapper = input.parentElement;
    const dropdown = wrapper.querySelector('.autocomplete-dropdown');
    if (dropdown) {
        dropdown.style.display = 'none';
    }
}

// Adjust dropdown position if it's cut off
function adjustDropdownPosition(dropdown) {
    if (!dropdown) return;
    
    // Reset position first
    dropdown.style.position = 'absolute';
    dropdown.style.top = '100%';
    dropdown.style.bottom = 'auto';
    
    // Force layout update
    dropdown.offsetHeight;
    
    const rect = dropdown.getBoundingClientRect();
    const viewportHeight = window.innerHeight;
    
    // Check if dropdown is cut off at bottom
    if (rect.bottom > viewportHeight - 20) {
        // Calculate available space above
        const parent = dropdown.parentElement;
        const parentRect = parent.getBoundingClientRect();
        const spaceAbove = parentRect.top;
        const spaceBelow = viewportHeight - parentRect.bottom;
        
        // If more space above, show dropdown above input
        if (spaceAbove > spaceBelow && spaceAbove > 100) {
            const inputHeight = parent.querySelector('input').offsetHeight;
            dropdown.style.bottom = inputHeight + 'px';
            dropdown.style.top = 'auto';
            dropdown.style.maxHeight = Math.min(spaceAbove - 20, 300) + 'px';
        } else {
            // Keep below but limit height
            dropdown.style.maxHeight = Math.min(spaceBelow - 20, 300) + 'px';
        }
    }
}

function updateJsonPreview() {
    const testCase = buildTestCaseJson();
    const previewElement = document.getElementById('json-preview-content');
    previewElement.value = JSON.stringify(testCase, null, 2);
    
    // Auto-adjust height based on content
    adjustTextareaHeight(previewElement);
}

function adjustTextareaHeight(textarea) {
    // Reset height to auto to get the correct scrollHeight
    textarea.style.height = 'auto';
    // Set height to scrollHeight to fit content
    const newHeight = Math.max(250, textarea.scrollHeight);
    textarea.style.height = newHeight + 'px';
    
    // Ensure the modal scrolls to show changes if needed
    const modalScroll = document.querySelector('.test-editor-modal-scroll');
    if (modalScroll) {
        // Small delay to ensure DOM is updated
        setTimeout(() => {
            modalScroll.scrollTop = modalScroll.scrollTop;
        }, 0);
    }
}

function buildTestCaseJson() {
    // Get form values
    const idValue = document.getElementById('edit-id').value;
    const id = idValue || String(lastGeneratedTestId + 1);
    const category = document.getElementById('edit-category').value || 'unknown';
    const testType = document.getElementById('edit-test-type').value || 'test';
    const expected = document.getElementById('edit-expected').value || '';
    const description = document.getElementById('edit-description').value || '';
    
    // Build parameters object
    const params = {};
    document.querySelectorAll('.param-row').forEach(row => {
        const nameInput = row.querySelector('.param-name');
        const valueInput = row.querySelector('.param-value');
        if (!nameInput || !valueInput) return;
        
        const key = nameInput.value.trim();
        const value = valueInput.value.trim();
        const isArray = valueInput.dataset.isArray === 'true';
        const isObject = valueInput.dataset.isObject === 'true';
        
        if (key) {
            if (isArray) {
                // Parse array value
                try {
                    if (value.startsWith('[') && value.endsWith(']')) {
                        params[key] = JSON.parse(value);
                    } else if (value === '') {
                        params[key] = [];
                    } else {
                        // Try to parse as comma-separated values
                        params[key] = value.split(',').map(v => {
                            v = v.trim();
                            // Remove quotes if present
                            if ((v.startsWith('"') && v.endsWith('"')) || 
                                (v.startsWith("'") && v.endsWith("'"))) {
                                return v.slice(1, -1);
                            }
                            // Try to parse as number or boolean
                            if (v === 'true') return true;
                            if (v === 'false') return false;
                            if (!isNaN(v) && v !== '') return parseFloat(v);
                            return v;
                        });
                    }
                } catch (e) {
                    // If parsing fails, treat as comma-separated string
                    params[key] = value.split(',').map(v => v.trim());
                }
            } else if (isObject) {
                // Parse JSON object value
                try {
                    if (value.startsWith('{') && value.endsWith('}')) {
                        params[key] = JSON.parse(value);
                    } else {
                        params[key] = {};
                    }
                } catch (e) {
                    // If parsing fails, treat as empty object
                    params[key] = {};
                }
            } else {
                // Try to parse as JSON object first (for single values that might be objects)
                if (value.startsWith('{') && value.endsWith('}')) {
                    try {
                        params[key] = JSON.parse(value);
                    } catch (e) {
                        // Not valid JSON, treat as string
                        params[key] = value;
                    }
                } else if (value === 'true') {
                    params[key] = true;
                } else if (value === 'false') {
                    params[key] = false;
                } else if (!isNaN(value) && value !== '') {
                    params[key] = parseFloat(value);
                } else {
                    params[key] = value;
                }
            }
        }
    });
    
    // Build test case in the correct format for variations array
    const testCase = {
        id: parseInt(id),
        variationName: description || `Test ${id}`,
        options: params,  // Parameters go in options
        expectedBehavior: {
            returnValue: expected || "success"
        }
    };
    
    return testCase;
}

function exportTestCase() {
    // Get the edited JSON from the preview textarea
    const jsonStr = document.getElementById('json-preview-content').value;
    
    // Parse the JSON to get the ID and update lastGeneratedTestId
    try {
        const testCase = JSON.parse(jsonStr);
        const id = parseInt(testCase.id);
        if (!isNaN(id)) {
            lastGeneratedTestId = Math.max(lastGeneratedTestId, id);
        }
    } catch (e) {
        console.error('Failed to parse test case JSON:', e);
    }
    
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
    // Get the edited JSON from the preview textarea
    const jsonStr = document.getElementById('json-preview-content').value;
    
    // Parse the JSON to get the ID and update lastGeneratedTestId
    try {
        const testCase = JSON.parse(jsonStr);
        const id = parseInt(testCase.id);
        if (!isNaN(id)) {
            lastGeneratedTestId = Math.max(lastGeneratedTestId, id);
        }
    } catch (e) {
        console.error('Failed to parse test case JSON:', e);
    }
    
    // Get the button element that was clicked
    const btn = document.querySelector('.test-copy-btn');
    
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
    const btnTestCase = document.getElementById('bulk-export-testcase-btn');
    const count = selectedTestCases.size;
    
    // Update counter
    const countTc = document.getElementById('selected-count-tc');
    if (countTc) countTc.textContent = count;
    
    // Update button
    if (btnTestCase) {
        btnTestCase.style.display = 'inline-block'; // Always visible
        
        if (count > 0) {
            btnTestCase.disabled = false;
            btnTestCase.style.opacity = '1';
            btnTestCase.style.cursor = 'pointer';
        } else {
            btnTestCase.disabled = true;
            btnTestCase.style.opacity = '0.5';
            btnTestCase.style.cursor = 'not-allowed';
        }
    }
    
    // Backward compatibility for old bulk-export-btn
    const oldBtn = document.getElementById('bulk-export-btn');
    if (oldBtn) {
        oldBtn.style.display = 'none';
    }
    const oldCount = document.getElementById('selected-count');
    if (oldCount) oldCount.textContent = count;
}

// Bulk export modal functions
function openBulkExport(mode = 'testcase') {
    if (selectedTestCases.size === 0) return;
    
    // Store the export mode
    window.bulkExportMode = mode;
    
    // Create modal if it doesn't exist
    let modal = document.getElementById('bulk-export-modal');
    if (!modal) {
        modal = createBulkExportModal();
    }
    
    // Populate with selected test cases
    populateBulkExportModal();
    modal.classList.add('show');
    document.body.classList.add('modal-open');
}

function createBulkExportModal() {
    const modalHtml = `
        <div id="bulk-export-modal" class="modal">
            <div class="modal-content bulk-export-modal">
                <span class="close-modal" onclick="closeBulkExportModal()">&times;</span>
                <h2>Export Selected Test Cases (${window.bulkExportMode === 'applied' ? 'Applied Properties' : 'Test Case JSON'})</h2>
                <div class="bulk-export-modal-scroll">
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
        </div>
    `;
    
    document.body.insertAdjacentHTML('beforeend', modalHtml);
    return document.getElementById('bulk-export-modal');
}

function updateBulkJsonPreview() {
    // Removed JSON preview functionality
}

function buildBulkTestCases() {
    const mode = window.bulkExportMode || 'testcase';
    const testCases = [];
    let autoId = 1;
    
    Array.from(selectedTestCases).forEach(index => {
        const result = filteredResults[index];
        
        // Get values from form if item is expanded, otherwise use defaults
        const categoryInput = document.getElementById(`bulk-category-${index}`);
        const testTypeInput = document.getElementById(`bulk-test-type-${index}`);
        const expectedInput = document.getElementById(`bulk-expected-${index}`);
        const descriptionInput = document.getElementById(`bulk-description-${index}`);
        
        let params = {};
        
        // Collect parameters from single, array, and object containers
        const singleParamsContainer = document.getElementById(`bulk-single-params-${index}`);
        const arrayParamsContainer = document.getElementById(`bulk-array-params-${index}`);
        const objectParamsContainer = document.getElementById(`bulk-object-params-${index}`);
        
        const collectParams = (container) => {
            if (container) {
                container.querySelectorAll('.param-row').forEach(row => {
                const nameInput = row.querySelector('.param-name');
                const valueInput = row.querySelector('.param-value');
                const isArray = valueInput && valueInput.dataset.isArray === 'true';
                const isObject = valueInput && valueInput.dataset.isObject === 'true';
                
                if (nameInput && valueInput && nameInput.value) {
                    const key = nameInput.value.trim();
                    let value = valueInput.value.trim();
                    
                    if (isArray) {
                        // Parse array value using the same logic as buildTestCaseJson
                        try {
                            if (value.startsWith('[') && value.endsWith(']')) {
                                value = JSON.parse(value);
                            } else if (value === '') {
                                value = [];
                            } else {
                                // Try to parse as comma-separated values
                                value = value.split(',').map(v => {
                                    v = v.trim();
                                    // Remove quotes if present
                                    if ((v.startsWith('"') && v.endsWith('"')) || 
                                        (v.startsWith("'") && v.endsWith("'"))) {
                                        return v.slice(1, -1);
                                    }
                                    // Try to parse as number or boolean
                                    if (v === 'true') return true;
                                    if (v === 'false') return false;
                                    if (!isNaN(v) && v !== '') return parseFloat(v);
                                    return v;
                                });
                            }
                        } catch (e) {
                            // If parsing fails, treat as comma-separated string
                            value = value.split(',').map(v => v.trim());
                        }
                    } else if (isObject) {
                        // Parse JSON object value
                        try {
                            if (value.startsWith('{') && value.endsWith('}')) {
                                value = JSON.parse(value);
                            } else {
                                value = {};
                            }
                        } catch (e) {
                            // If parsing fails, treat as empty object
                            value = {};
                        }
                    } else {
                        // Check if it's a JSON object string (for single values that might be objects)
                        if (value.startsWith('{') && value.endsWith('}')) {
                            try {
                                value = JSON.parse(value);
                            } catch (e) {
                                // Not valid JSON, keep as string
                            }
                        } else if (value === 'true') {
                            value = true;
                        } else if (value === 'false') {
                            value = false;
                        } else if (!isNaN(value) && value !== '') {
                            value = parseFloat(value);
                        }
                    }
                    params[key] = value;
                }
                });
            }
        };
        
        collectParams(singleParamsContainer);
        collectParams(arrayParamsContainer);
        collectParams(objectParamsContainer);
        
        if (!singleParamsContainer && !arrayParamsContainer) {
            // Use original parameters if not expanded
            if (mode === 'testcase') {
                // Filter out runtime-only properties for test case mode
                const runtimeOnlyProps = [
                    'builder_width', 'builder_height', 
                    'preferred_format', 'preferred_present_mode', 'preferred_alpha_mode',
                    'format_fallbacks', 'present_mode_fallbacks', 'alpha_mode_fallbacks',
                    'debugEnabled', 'validationEnabled'
                ];
                
                const originalParams = result.input_parameters || {};
                Object.entries(originalParams).forEach(([key, value]) => {
                    if (!runtimeOnlyProps.includes(key)) {
                        params[key] = value;
                    }
                });
            } else {
                // For 'applied' mode, use all parameters
                Object.assign(params, result.input_parameters || {});
            }
        }
        
        const testCase = {
            id: autoId++,
            variationName: descriptionInput ? descriptionInput.value : `Test ${autoId}`,
            options: params,
            expectedBehavior: {
                returnValue: expectedInput ? expectedInput.value : (result.expected_result || 'success')
            }
        };
        
        // Add category and testType if they're different from the original
        if (categoryInput && categoryInput.value && categoryInput.value !== result.category) {
            testCase.category = categoryInput.value;
        }
        if (testTypeInput && testTypeInput.value && testTypeInput.value !== result.testType) {
            testCase.testType = testTypeInput.value;
        }
        
        // Description is now handled as variationName above
        
        testCases.push(testCase);
    });
    
    return testCases;
}

function populateBulkExportModal() {
    const mode = window.bulkExportMode || 'testcase';
    const container = document.getElementById('bulk-export-list');
    container.innerHTML = '';
    
    // Update modal title to show mode
    const modalTitle = document.querySelector('#bulk-export-modal h2');
    if (modalTitle) {
        modalTitle.textContent = `Export Selected Test Cases (${mode === 'applied' ? 'Applied Properties' : 'Test Case JSON'})`;
    }
    
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
        
        // Build parameters HTML based on mode
        let parametersHtml = '';
        let paramsToShow = {};
        
        if (mode === 'testcase') {
            // Filter out runtime-only properties for test case mode
            const runtimeOnlyProps = [
                'builder_width', 'builder_height', 
                'preferred_format', 'preferred_present_mode', 'preferred_alpha_mode',
                'format_fallbacks', 'present_mode_fallbacks', 'alpha_mode_fallbacks',
                'debugEnabled', 'validationEnabled'
            ];
            
            if (result.input_parameters) {
                Object.entries(result.input_parameters).forEach(([key, value]) => {
                    if (!runtimeOnlyProps.includes(key)) {
                        paramsToShow[key] = value;
                    }
                });
            }
        } else {
            // For 'applied' mode, show all parameters
            paramsToShow = result.input_parameters || {};
        }
        
        Object.entries(paramsToShow).forEach(([key, value]) => {
            const isArray = Array.isArray(value);
            parametersHtml += createParameterRow(key, value, isArray, 'bulk');
        })
        
        // Build single, array, and object parameters HTML separately
        let singleParametersHtml = '';
        let arrayParametersHtml = '';
        let objectParametersHtml = '';
        if (result.input_parameters) {
            Object.entries(result.input_parameters).forEach(([key, value]) => {
                let actualIsArray = Array.isArray(value);
                let actualIsObject = false;
                
                // Check if value is a string that represents an array or object
                if (!actualIsArray && typeof value === 'string') {
                    const trimmed = value.trim();
                    if (trimmed.startsWith('[')) {
                        try {
                            const parsed = JSON.parse(value);
                            actualIsArray = Array.isArray(parsed);
                        } catch (e) {
                            // Not valid JSON, treat as single value
                        }
                    } else if (trimmed.startsWith('{')) {
                        try {
                            const parsed = JSON.parse(value);
                            actualIsObject = typeof parsed === 'object' && parsed !== null && !Array.isArray(parsed);
                        } catch (e) {
                            // Not valid JSON, treat as single value
                        }
                    }
                } else if (typeof value === 'object' && value !== null && !Array.isArray(value)) {
                    actualIsObject = true;
                }
                
                if (actualIsArray) {
                    arrayParametersHtml += createParameterRow(key, value, true, 'bulk', true, false);
                } else if (actualIsObject) {
                    objectParametersHtml += createParameterRow(key, value, false, 'bulk', false, true);
                } else {
                    singleParametersHtml += createParameterRow(key, value, false, 'bulk', false, false);
                }
            });
        }
        
        contentDiv.innerHTML = `
            <div class="bulk-item-form">
                <!-- General Properties Section -->
                <div class="section-divider">
                    <h3 class="section-title">General Properties</h3>
                    <div class="section-content">
                        <div class="two-column-grid">
                            <div class="form-group">
                                <label>Category</label>
                                <input type="text" id="bulk-category-${index}" value="${result.category || ''}" oninput="updateBulkJsonPreview()">
                            </div>
                            <div class="form-group">
                                <label>Test Type</label>
                                <input type="text" id="bulk-test-type-${index}" value="${result.testType || ''}" oninput="updateBulkJsonPreview()">
                            </div>
                        </div>
                        <div class="form-group">
                            <label>Expected Result</label>
                            <div class="autocomplete-wrapper">
                                <input type="text" id="bulk-expected-${index}" class="autocomplete-input" value="${result.expected_result || ''}"
                                       oninput="showBulkExpectedAutocomplete(this); updateBulkJsonPreview()"
                                       onfocus="closeAllDropdowns(); showBulkExpectedAutocomplete(this)">
                                <div class="autocomplete-dropdown" style="display: none;"></div>
                            </div>
                        </div>
                        <div class="form-group">
                            <label>Description</label>
                            <textarea id="bulk-description-${index}" rows="3" oninput="updateBulkJsonPreview()"></textarea>
                        </div>
                    </div>
                </div>
                
                <!-- Parameters Section -->
                <div class="section-divider">
                    <h3 class="section-title">Parameters</h3>
                    <div class="section-content">
                        <!-- Single Value Parameters -->
                        <div class="param-subsection">
                            <div class="param-section-header">
                                <button onclick="addBulkParameter(${index}, false, false)" class="add-param-btn">Add</button>
                                <label>Single Value Parameters</label>
                            </div>
                            <div id="bulk-single-params-${index}" class="param-container">
                                ${singleParametersHtml}
                            </div>
                        </div>
                        
                        <!-- Array Parameters -->
                        <div class="param-subsection">
                            <div class="param-section-header">
                                <button onclick="addBulkParameter(${index}, true, false)" class="add-param-btn">Add</button>
                                <label>Array Parameters</label>
                            </div>
                            <div id="bulk-array-params-${index}" class="param-container">
                                ${arrayParametersHtml}
                            </div>
                        </div>
                        
                        <!-- Object Parameters -->
                        <div class="param-subsection">
                            <div class="param-section-header">
                                <button onclick="addBulkParameter(${index}, false, true)" class="add-param-btn">Add</button>
                                <label>Object Parameters</label>
                            </div>
                            <div id="bulk-object-params-${index}" class="param-container">
                                ${objectParametersHtml}
                            </div>
                        </div>
                    </div>
                </div>
            </div>
        `;
        
        itemDiv.appendChild(headerDiv);
        itemDiv.appendChild(contentDiv);
        container.appendChild(itemDiv);
    });
    
    // Update JSON preview
    updateBulkJsonPreview();
    
    // Add event listeners for real-time updates
    container.addEventListener('input', (e) => {
        if (e.target.tagName === 'INPUT' || e.target.tagName === 'TEXTAREA') {
            updateBulkJsonPreview();
        }
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
        document.body.classList.remove('modal-open');
    }
}

function addBulkParameter(index, isArray = false, isObject = false) {
    let container;
    if (isArray) {
        container = document.getElementById(`bulk-array-params-${index}`);
    } else if (isObject) {
        container = document.getElementById(`bulk-object-params-${index}`);
    } else {
        container = document.getElementById(`bulk-single-params-${index}`);
    }
    
    const initialValue = isObject ? '{}' : '';
    const rowHtml = createParameterRow('', initialValue, isArray, 'bulk', isArray, isObject);
    
    // Create a temporary div to parse the HTML
    const tempDiv = document.createElement('div');
    tempDiv.innerHTML = rowHtml;
    
    // Add change handler to check for duplicates
    const newRow = tempDiv.firstElementChild;
    const nameInput = newRow.querySelector('.param-name');
    nameInput.addEventListener('blur', () => checkForDuplicateParameter(nameInput));
    
    // Append the parsed element to the container
    container.appendChild(newRow);
    updateBulkJsonPreview();
}

function exportBulkTestCases() {
    const testCases = buildBulkTestCases();
    
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
    const testCases = buildBulkTestCases();
    
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
let currentArrayParamName = '';

function openArrayEditor(inputElement, testIndex, paramName) {
    currentArrayInput = inputElement;
    currentArrayParamName = paramName;
    
    // Set up autocomplete for new item input
    const newItemInput = document.getElementById('new-array-item');
    newItemInput.oninput = () => showArrayNewItemAutocomplete(newItemInput);
    newItemInput.onfocus = () => showArrayNewItemAutocomplete(newItemInput);
    newItemInput.onblur = () => {
        setTimeout(() => hideArrayNewItemAutocomplete(newItemInput), 200);
    };
    
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
    
    // Update preview
    updateArrayPreview();
    
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
            <div class="autocomplete-wrapper" style="flex: 1;">
                <input type="text" class="array-item-value autocomplete-input" value="${String(itemValue).replace(/"/g, '&quot;')}" 
                       onchange="updateArrayItem(${index}, this.value)"
                       oninput="showArrayItemAutocomplete(this)"
                       onfocus="showArrayItemAutocomplete(this)"
                       onblur="setTimeout(() => hideArrayItemAutocomplete(this), 200)">
                <div class="autocomplete-dropdown" style="display: none;"></div>
            </div>
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
        // Get the original value
        const originalValue = currentArrayInput.value;
        
        // Format array as comma-separated string for display
        const formattedValue = currentArrayItems.map(item => {
            if (typeof item === 'string') {
                return `"${item}"`;
            }
            return JSON.stringify(item);
        }).join(', ');
        
        // Check if anything actually changed
        const hasChanges = originalValue !== formattedValue || 
                          (originalValue === '' && currentArrayItems.length > 0);
        
        // Update the input field with formatted value
        currentArrayInput.value = formattedValue;
        
        // Trigger change event to update preview if needed
        const changeEvent = new Event('change', { bubbles: true });
        currentArrayInput.dispatchEvent(changeEvent);
        
        closeArrayEditor();
        
        // Only show success message if there were actual changes
        if (hasChanges) {
            showNotification('Array updated successfully');
        }
    } else {
        closeArrayEditor();
    }
}

function closeArrayEditor() {
    const modal = document.getElementById('array-editor-modal');
    modal.classList.remove('show');
    currentArrayInput = null;
    currentArrayItems = [];
}

function updateArrayPreview() {
    const preview = document.getElementById('array-preview-content');
    if (preview) {
        preview.value = JSON.stringify(currentArrayItems, null, 2);
    }
}

// JSON Object Editor Functions
let currentJsonObjectInput = null;
let currentJsonObject = {};
let currentJsonObjectParamName = '';

function openJsonObjectEditor(inputElement, paramName) {
    currentJsonObjectInput = inputElement;
    currentJsonObjectParamName = paramName;
    
    // Parse current JSON object value
    const currentValue = inputElement.value.trim();
    try {
        if (currentValue.startsWith('{')) {
            currentJsonObject = JSON.parse(currentValue);
        } else {
            currentJsonObject = {};
        }
        
        if (typeof currentJsonObject !== 'object' || currentJsonObject === null || Array.isArray(currentJsonObject)) {
            currentJsonObject = {};
        }
    } catch (e) {
        currentJsonObject = {};
    }
    
    // Set up autocomplete for key and value inputs
    const keyInput = document.getElementById('new-object-key');
    const valueInput = document.getElementById('new-object-value');
    
    keyInput.oninput = () => showObjectKeyAutocomplete(keyInput);
    keyInput.onfocus = () => showObjectKeyAutocomplete(keyInput);
    keyInput.onblur = () => {
        setTimeout(() => hideObjectKeyAutocomplete(keyInput), 200);
    };
    
    valueInput.oninput = () => showObjectValueAutocomplete(valueInput, keyInput.value);
    valueInput.onfocus = () => showObjectValueAutocomplete(valueInput, keyInput.value);
    valueInput.onblur = () => {
        setTimeout(() => hideObjectValueAutocomplete(valueInput), 200);
    };
    
    // Update modal title
    const modal = document.getElementById('json-object-editor-modal');
    modal.querySelector('h2').textContent = `Edit JSON Object: ${paramName}`;
    
    // Populate JSON object editor
    populateJsonObjectEditor();
    
    // Show modal
    modal.classList.add('show');
}

function populateJsonObjectEditor() {
    const container = document.getElementById('json-properties-list');
    container.innerHTML = '';
    
    const entries = Object.entries(currentJsonObject);
    
    // Add scrollable class only if there are many items
    if (entries.length > 5) {
        container.classList.add('scrollable');
    } else {
        container.classList.remove('scrollable');
    }
    
    if (entries.length === 0) {
        container.innerHTML = '<div class="no-items">No properties. Add properties using the form above.</div>';
    } else {
        entries.forEach(([key, value]) => {
            const itemDiv = document.createElement('div');
            itemDiv.className = 'json-property-item';
            
            let valueStr = value;
            if (typeof value === 'object') {
                valueStr = JSON.stringify(value);
            }
            
            itemDiv.innerHTML = `
                <div class="autocomplete-wrapper">
                    <input type="text" value="${key}" data-original-key="${key}"
                           onchange="updateJsonPropertyKey(this, '${key}')"
                           onfocus="showObjectKeyAutocomplete(this)"
                           oninput="showObjectKeyAutocomplete(this)"
                           onblur="setTimeout(() => hideObjectKeyAutocomplete(this), 200)">
                    <div class="autocomplete-dropdown" style="display: none;"></div>
                </div>
                <div class="autocomplete-wrapper">
                    <input type="text" value="${String(valueStr).replace(/"/g, '&quot;')}" 
                           onchange="updateJsonPropertyValue('${key}', this.value)"
                           onfocus="showObjectValueAutocomplete(this, '${key}')"
                           oninput="showObjectValueAutocomplete(this, '${key}')"
                           onblur="setTimeout(() => hideObjectValueAutocomplete(this), 200)">
                    <div class="autocomplete-dropdown" style="display: none;"></div>
                </div>
                <button class="remove-btn" onclick="removeJsonProperty('${key}')">Remove</button>
            `;
            
            container.appendChild(itemDiv);
        });
    }
    
    // Update preview
    updateJsonObjectPreview();
}

function addJsonProperty() {
    const keyInput = document.getElementById('new-object-key');
    const valueInput = document.getElementById('new-object-value');
    
    const key = keyInput.value.trim();
    const valueStr = valueInput.value.trim();
    
    if (!key) {
        showNotification('Please enter a property name', 'error');
        return;
    }
    
    // Try to parse value as JSON, number, or boolean
    let value = valueStr;
    try {
        if (valueStr.startsWith('{') || valueStr.startsWith('[')) {
            value = JSON.parse(valueStr);
        } else if (valueStr === 'true') {
            value = true;
        } else if (valueStr === 'false') {
            value = false;
        } else if (!isNaN(valueStr) && valueStr !== '') {
            value = parseFloat(valueStr);
        }
    } catch (e) {
        // Keep as string if not valid JSON
    }
    
    currentJsonObject[key] = value;
    keyInput.value = '';
    valueInput.value = '';
    populateJsonObjectEditor();
}

function updateJsonPropertyKey(input, oldKey) {
    const newKey = input.value.trim();
    
    if (newKey && newKey !== oldKey) {
        const value = currentJsonObject[oldKey];
        delete currentJsonObject[oldKey];
        currentJsonObject[newKey] = value;
        populateJsonObjectEditor();
    } else if (!newKey) {
        // Revert if empty
        input.value = oldKey;
    }
}

function updateJsonPropertyValue(key, newValue) {
    let value = newValue;
    try {
        if (newValue.startsWith('{') || newValue.startsWith('[')) {
            value = JSON.parse(newValue);
        } else if (newValue === 'true') {
            value = true;
        } else if (newValue === 'false') {
            value = false;
        } else if (!isNaN(newValue) && newValue !== '') {
            value = parseFloat(newValue);
        }
    } catch (e) {
        // Keep as string
    }
    currentJsonObject[key] = value;
    updateJsonObjectPreview();
}

function removeJsonProperty(key) {
    delete currentJsonObject[key];
    populateJsonObjectEditor();
}

function updateJsonObjectPreview() {
    const preview = document.getElementById('json-object-preview-content');
    if (preview) {
        preview.value = JSON.stringify(currentJsonObject, null, 2);
    }
}

function saveJsonObjectChanges() {
    if (currentJsonObjectInput) {
        // Update the input field with formatted JSON
        currentJsonObjectInput.value = JSON.stringify(currentJsonObject, null, 2);
        
        // Trigger change event to update preview if needed
        const changeEvent = new Event('change', { bubbles: true });
        currentJsonObjectInput.dispatchEvent(changeEvent);
        
        // Update the data-is-object attribute
        currentJsonObjectInput.setAttribute('data-is-object', 'true');
        
        closeJsonObjectEditor();
        showNotification('JSON object updated successfully');
    } else {
        closeJsonObjectEditor();
    }
}

function closeJsonObjectEditor() {
    const modal = document.getElementById('json-object-editor-modal');
    modal.classList.remove('show');
    currentJsonObjectInput = null;
    currentJsonObject = {};
    currentJsonObjectParamName = '';
}

// Autocomplete functions for object editor
function showObjectKeyAutocomplete(input) {
    const dropdown = input.parentElement.querySelector('.autocomplete-dropdown');
    
    // Get all known property names for object parameters
    const knownKeys = new Set();
    objectPropertyValues.forEach((values, key) => {
        // key format is "paramName.propertyName"
        const parts = key.split('.');
        if (parts.length >= 2) {
            knownKeys.add(parts[parts.length - 1]);
        }
    });
    
    // Add common WebGPU object properties
    const commonKeys = [
        'maxTextureDimension1D', 'maxTextureDimension2D', 'maxTextureDimension3D',
        'maxTextureArrayLayers', 'maxBindGroups', 'maxBindingsPerBindGroup',
        'maxDynamicUniformBuffersPerPipelineLayout', 'maxDynamicStorageBuffersPerPipelineLayout',
        'maxSampledTexturesPerShaderStage', 'maxSamplersPerShaderStage',
        'maxStorageBuffersPerShaderStage', 'maxStorageTexturesPerShaderStage',
        'maxUniformBuffersPerShaderStage', 'maxUniformBufferBindingSize',
        'maxStorageBufferBindingSize', 'maxVertexBuffers', 'maxVertexAttributes',
        'maxVertexBufferArrayStride', 'maxInterStageShaderVariables',
        'maxComputeWorkgroupStorageSize', 'maxComputeInvocationsPerWorkgroup',
        'maxComputeWorkgroupSizeX', 'maxComputeWorkgroupSizeY', 'maxComputeWorkgroupSizeZ',
        'maxComputeWorkgroupsPerDimension'
    ];
    
    commonKeys.forEach(key => knownKeys.add(key));
    
    // Filter out properties that already exist in currentJsonObject
    const existingKeys = Object.keys(currentJsonObject || {});
    const availableKeys = Array.from(knownKeys).filter(key => !existingKeys.includes(key));
    
    showDropdown(input, availableKeys, (value) => {
        input.value = value;
        dropdown.style.display = 'none';
    });
}

function hideObjectKeyAutocomplete(input) {
    const dropdown = input.parentElement.querySelector('.autocomplete-dropdown');
    if (dropdown) {
        dropdown.style.display = 'none';
    }
}

function showObjectValueAutocomplete(input, propertyKey) {
    const dropdown = input.parentElement.querySelector('.autocomplete-dropdown');
    
    // Get known values for this property
    const fullKey = `${currentJsonObjectParamName}.${propertyKey}`;
    let knownValues = new Set();
    
    if (objectPropertyValues.has(fullKey)) {
        knownValues = objectPropertyValues.get(fullKey);
    }
    
    // Add common values based on property name
    if (propertyKey.includes('max') || propertyKey.includes('size') || propertyKey.includes('count')) {
        // Add common size/limit values
        [256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 134217728].forEach(v => knownValues.add(String(v)));
    }
    
    // Add boolean values for boolean-like properties
    knownValues.add('true');
    knownValues.add('false');
    
    showDropdown(input, Array.from(knownValues), (value) => {
        input.value = value;
        dropdown.style.display = 'none';
    });
}

function hideObjectValueAutocomplete(input) {
    const dropdown = input.parentElement.querySelector('.autocomplete-dropdown');
    if (dropdown) {
        dropdown.style.display = 'none';
    }
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