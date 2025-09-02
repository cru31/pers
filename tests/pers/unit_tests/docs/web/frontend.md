# Web Viewer Frontend Implementation

## Overview

The web viewer frontend is a single-page application (SPA) built with vanilla JavaScript, providing an interactive interface for viewing and analyzing test results. It features real-time filtering, expandable details, and VSCode integration.

## Architecture

### Component Structure

```
Frontend Application
├── HTML Structure (index.html)
│   ├── Header Section
│   ├── Summary Cards
│   ├── Tab Navigation
│   └── Tab Contents
│       ├── Test Results Table
│       └── Full Logs View
├── Styling (styles.css)
│   ├── Layout Grid
│   ├── Component Styles
│   ├── Status Colors
│   └── Responsive Design
└── JavaScript Logic (app.js)
    ├── Data Management
    ├── UI Rendering
    ├── Event Handlers
    └── API Communication
```

## User Interface Design

### Layout

```
┌─────────────────────────────────────────────────────────────┐
│                         Header                               │
│  ┌─────────────┐  ┌──────────────┐  ┌──────────────────┐  │
│  │ Session Info│  │ Load JSON Btn │  │ Timestamp        │  │
│  └─────────────┘  └──────────────┘  └──────────────────┘  │
├─────────────────────────────────────────────────────────────┤
│                     Summary Cards                            │
│  ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐   │
│  │Total │ │Pass  │ │Fail  │ │ N/A  │ │ NYI  │ │Rate  │   │
│  └──────┘ └──────┘ └──────┘ └──────┘ └──────┘ └──────┘   │
├─────────────────────────────────────────────────────────────┤
│  [ Test Results ]  [ Full Logs ]                            │
├─────────────────────────────────────────────────────────────┤
│  Search: [_________] Category: [▼] Status: [▼]             │
├─────────────────────────────────────────────────────────────┤
│  ┌──────────────────────────────────────────────────────┐  │
│  │                   Test Results Table                  │  │
│  │  ID | Category | Type | Status | Time | Expected |.. │  │
│  │  ──────────────────────────────────────────────────  │  │
│  │  001| Critical | ... | PASS NYI| 10ms | Valid... |   │  │
│  │  ▼ Expandable details with logs                      │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

## Features

### 1. Status Cards

Interactive summary cards with click-to-filter functionality:

```javascript
// Card HTML structure
<div class="card passed clickable" data-filter="passed">
    <h3>Passed</h3>
    <div class="card-value" id="passed-tests">9</div>
</div>

// Click handler
card.addEventListener('click', () => {
    const filterValue = card.dataset.filter;
    document.getElementById('status-filter').value = filterValue;
    filterResults();
});
```

### 2. Multi-Status Badges

Tests can display multiple status badges:

```javascript
// Determine badges for a test
let badges = [];

// Primary status
if (result.actual_result?.includes('N/A')) {
    badges.push({ status: 'na', text: 'N/A' });
} else if (result.passed) {
    badges.push({ status: 'passed', text: 'PASS' });
} else {
    badges.push({ status: 'failed', text: 'FAIL' });
}

// Secondary status (NYI)
if (result.log_messages?.some(log => log.includes('[TODO_OR_DIE]'))) {
    badges.push({ status: 'nyi', text: 'NYI' });
}

// Render badges
const badgeHtml = badges.map(badge => 
    `<span class="status-badge status-${badge.status}">${badge.text}</span>`
).join(' ');
```

### 3. Expandable Test Details

Click to expand test rows for detailed information:

```javascript
function toggleDetails(index) {
    const detailRow = document.querySelector(`tr[data-detail-index="${index}"]`);
    const mainRow = document.querySelector(`tr[data-index="${index}"]`);
    
    if (detailRow.style.display === 'table-row') {
        detailRow.style.display = 'none';
        mainRow.classList.remove('expanded');
    } else {
        detailRow.style.display = 'table-row';
        mainRow.classList.add('expanded');
    }
}
```

### 4. Log Message Formatting

Color-coded log messages with source links:

```javascript
function formatLogMessages(logs) {
    return logs.map(log => {
        // Parse source location for links
        const sourceMatch = log.match(/\(([^@]+)\s*@\s*(.+):(\d+)\)$/);
        
        if (sourceMatch) {
            const [_, func, file, line] = sourceMatch;
            const linkHtml = `(<a href="#" class="source-path-link" 
                data-path="${file}" data-line="${line}">
                ${func} @ ${file}:${line}</a>)`;
            log = log.replace(/\([^)]+\)$/, linkHtml);
        }
        
        // Apply color coding by log level
        const levelMatch = log.match(/^\[(TRACE|DEBUG|INFO|WARNING|ERROR|CRITICAL)\]/);
        if (levelMatch) {
            const level = levelMatch[1];
            const logClass = getLogClass(level);
            log = log.replace(/^\[[\w_]+\]/, 
                `<span class="log-level-inline ${logClass}">[${level}]</span>`);
        }
        
        return `<div class="log-line">${log}</div>`;
    }).join('');
}
```

## Filtering System

### Search and Filter Implementation

```javascript
function filterResults() {
    const searchTerm = document.getElementById('search').value.toLowerCase();
    const categoryFilter = document.getElementById('category-filter').value;
    const statusFilter = document.getElementById('status-filter').value;
    
    filteredResults = allResults.filter(result => {
        // Text search across all fields
        if (searchTerm) {
            const searchable = [
                result.id, result.category, result.test_type,
                result.input, result.expected_result, result.actual_result
            ].join(' ').toLowerCase();
            
            if (!searchable.includes(searchTerm)) return false;
        }
        
        // Category filter
        if (categoryFilter && result.category !== categoryFilter) {
            return false;
        }
        
        // Status filter with multi-status support
        if (statusFilter) {
            if (statusFilter === 'na') {
                return result.actual_result?.includes('N/A');
            } else if (statusFilter === 'nyi') {
                return result.log_messages?.some(log => 
                    log.includes('[TODO_OR_DIE]'));
            } else if (statusFilter === 'passed') {
                return result.passed && 
                       !result.actual_result?.includes('N/A');
            } else if (statusFilter === 'failed') {
                return !result.passed && 
                       !result.actual_result?.includes('N/A');
            }
        }
        
        return true;
    });
    
    displayResults();
}
```

## Full Logs View

### Log Entry Display

```javascript
function createFullLogsView() {
    // Collect all logs with test context
    const allLogs = [];
    allResults.forEach(result => {
        result.log_messages?.forEach(log => {
            allLogs.push({
                testId: result.id,
                testType: result.test_type,
                category: result.category,
                passed: result.passed,
                log: log,
                result: result
            });
        });
    });
    
    // Render each log entry
    allLogs.forEach(entry => {
        const logDiv = createLogEntry(entry);
        const sourceDiv = createSourceInfo(entry);
        const detailsDiv = createTestDetails(entry);
        
        // Click to expand
        logDiv.addEventListener('click', () => {
            toggleLogDetails(sourceDiv, detailsDiv, logDiv);
        });
        
        container.appendChild(logDiv);
        container.appendChild(sourceDiv);
        container.appendChild(detailsDiv);
    });
}
```

### Log Parsing

```javascript
function parseLogEntry(log) {
    const result = {
        level: '',
        category: '',
        message: '',
        function: '',
        file: '',
        line: ''
    };
    
    // Extract log level
    const levelMatch = log.match(/^\[([\w_]+)\]/);
    if (levelMatch) {
        result.level = levelMatch[1];
    }
    
    // Parse new format: "Category: Message (function @ file:line)"
    const match = log.match(/^\[[\w_]+\]\s*([^:]+):\s+(.*)\s+\(([^@]+)\s*@\s*(.+):(\d+)\)$/);
    if (match) {
        result.category = match[1];
        result.message = match[2];
        result.function = match[3];
        result.file = match[4];
        result.line = match[5];
    }
    
    return result;
}
```

## VSCode Integration

### File Opening Protocol

```javascript
function openInVSCode(path, line) {
    // Convert path to VSCode URI format
    const vscodePath = path.replace(/\\/g, '/');
    const vscodeUri = `vscode://file/${vscodePath}:${line || 1}:1`;
    
    // Attempt to open in VSCode
    window.location.href = vscodeUri;
}

// Event delegation for all source links
document.addEventListener('click', (e) => {
    if (e.target.classList.contains('source-path-link')) {
        e.preventDefault();
        const path = e.target.dataset.path;
        const line = e.target.dataset.line;
        openInVSCode(path, line);
    }
});
```

## Local File Loading

### FileReader Implementation

```javascript
function loadJsonFile(file) {
    const reader = new FileReader();
    
    reader.onload = (e) => {
        try {
            const data = JSON.parse(e.target.result);
            
            // Update application state
            allResults = data.results || [];
            updateSummary(data.metadata);
            populateCategoryFilter();
            filteredResults = [...allResults];
            
            // Refresh UI
            displayResults();
            createFullLogsView();
            
            // Update status display
            document.getElementById('session-info').textContent = 
                `Local File: ${file.name}`;
            
        } catch (error) {
            showError(`Error parsing JSON: ${error.message}`);
        }
    };
    
    reader.readAsText(file);
}

// File input handler
document.getElementById('json-file-input').addEventListener('change', (e) => {
    const file = e.target.files[0];
    if (file) loadJsonFile(file);
});
```

## State Management

### Application State

```javascript
// Global state
let allResults = [];        // All test results
let filteredResults = [];   // Filtered subset
let expandedRows = new Set(); // Expanded detail rows
let activeFilters = {
    search: '',
    category: '',
    status: ''
};

// State persistence (optional)
function saveState() {
    localStorage.setItem('testViewerState', JSON.stringify({
        filters: activeFilters,
        expanded: Array.from(expandedRows)
    }));
}

function restoreState() {
    const saved = localStorage.getItem('testViewerState');
    if (saved) {
        const state = JSON.parse(saved);
        activeFilters = state.filters;
        expandedRows = new Set(state.expanded);
        applyFilters();
    }
}
```

## Performance Optimization

### Virtual Scrolling (for large datasets)

```javascript
class VirtualScroller {
    constructor(container, itemHeight, renderItem) {
        this.container = container;
        this.itemHeight = itemHeight;
        this.renderItem = renderItem;
        this.items = [];
        this.visibleRange = { start: 0, end: 0 };
    }
    
    setItems(items) {
        this.items = items;
        this.container.style.height = 
            `${items.length * this.itemHeight}px`;
        this.render();
    }
    
    render() {
        const scrollTop = this.container.scrollTop;
        const containerHeight = this.container.clientHeight;
        
        const start = Math.floor(scrollTop / this.itemHeight);
        const end = Math.ceil((scrollTop + containerHeight) / this.itemHeight);
        
        // Only render visible items
        for (let i = start; i < end && i < this.items.length; i++) {
            this.renderItem(this.items[i], i);
        }
    }
}
```

### Debounced Search

```javascript
function debounce(func, delay) {
    let timeoutId;
    return function(...args) {
        clearTimeout(timeoutId);
        timeoutId = setTimeout(() => func.apply(this, args), delay);
    };
}

const debouncedFilter = debounce(filterResults, 300);
document.getElementById('search').addEventListener('input', debouncedFilter);
```

## Styling

### CSS Variables for Theming

```css
:root {
    /* Status colors */
    --color-pass: #48bb78;
    --color-fail: #f56565;
    --color-na: #a0aec0;
    --color-nyi: #ed8936;
    --color-skip: #9f7aea;
    
    /* Log level colors */
    --log-trace: #718096;
    --log-debug: #4299e1;
    --log-info: #48bb78;
    --log-warning: #f6ad55;
    --log-error: #fc8181;
    --log-critical: #f56565;
    
    /* UI colors */
    --bg-primary: #1a202c;
    --bg-secondary: #2d3748;
    --text-primary: #f7fafc;
    --text-secondary: #cbd5e0;
}
```

### Responsive Design

```css
/* Mobile-first approach */
.container {
    padding: 1rem;
    max-width: 100%;
}

/* Tablet and up */
@media (min-width: 768px) {
    .summary-cards {
        display: grid;
        grid-template-columns: repeat(3, 1fr);
        gap: 1rem;
    }
}

/* Desktop */
@media (min-width: 1024px) {
    .summary-cards {
        grid-template-columns: repeat(6, 1fr);
    }
    
    .container {
        max-width: 1200px;
        margin: 0 auto;
    }
}
```

## Accessibility

### ARIA Labels

```html
<button aria-label="Expand test details" 
        aria-expanded="false"
        onclick="toggleDetails(0)">
    ▶
</button>

<div role="status" aria-live="polite">
    <span id="filter-status">Showing 24 of 100 tests</span>
</div>
```

### Keyboard Navigation

```javascript
// Enable keyboard navigation
document.addEventListener('keydown', (e) => {
    const focused = document.activeElement;
    
    if (focused.classList.contains('test-row')) {
        if (e.key === 'Enter' || e.key === ' ') {
            e.preventDefault();
            focused.click();
        } else if (e.key === 'ArrowDown') {
            focused.nextElementSibling?.focus();
        } else if (e.key === 'ArrowUp') {
            focused.previousElementSibling?.focus();
        }
    }
});
```

## Error Handling

### User-Friendly Error Display

```javascript
function showError(message, details = null) {
    const errorDiv = document.getElementById('error');
    errorDiv.innerHTML = `
        <div class="error-message">
            <strong>Error:</strong> ${escapeHtml(message)}
            ${details ? `<details>
                <summary>Details</summary>
                <pre>${escapeHtml(details)}</pre>
            </details>` : ''}
        </div>
    `;
    errorDiv.style.display = 'block';
    
    // Auto-hide after 10 seconds
    setTimeout(() => {
        errorDiv.style.display = 'none';
    }, 10000);
}
```

## Usage

### Basic Usage

1. **Automatic Launch**: Test runner launches viewer automatically
   ```bash
   ./pers_json_tests.exe --jsviewer test_cases.json
   ```

2. **Manual File Load**: Click "Load JSON" to load local results

3. **Filtering**: Use search box or click status cards

4. **Details**: Click any row to expand test details

5. **VSCode**: Click file paths to open in VSCode

### Advanced Features

- **Multi-select**: Ctrl+click to select multiple tests
- **Export**: Right-click to export filtered results
- **Comparison**: Load multiple files to compare results
- **Bookmarks**: Save filter combinations for quick access