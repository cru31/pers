const path = require('path');
const fs = require('fs');

// Custom module loader for express
let express;
try {
    // Try to load express normally first
    express = require('express');
} catch (e) {
    // If not found, use the path from environment variable
    const nodeModulesPath = process.env.NODE_MODULES_PATH;
    if (nodeModulesPath) {
        const expressPath = path.join(nodeModulesPath.replace(/"/g, ''), 'express');
        express = require(expressPath);
    } else {
        throw new Error('Cannot find express module. Please set NODE_MODULES_PATH environment variable.');
    }
}

const app = express();
const PORT = 5000;

// Get result file path from command line argument or use default
const resultPath = process.argv[2];
const dataPath = resultPath || path.join(__dirname, '../../../../build/bin/Debug/test_results.json');
const sessionId = resultPath ? path.basename(path.dirname(resultPath)) : 'current';

console.log('Starting server with session ID:', sessionId);
console.log('Data path:', dataPath);

// Serve static files
app.use(express.static(path.join(__dirname, 'public')));

// API endpoint to get test results
app.get('/api/results', (req, res) => {
    try {
        if (fs.existsSync(dataPath)) {
            const data = fs.readFileSync(dataPath, 'utf8');
            res.json(JSON.parse(data));
        } else {
            res.status(404).json({ error: 'Test results not found' });
        }
    } catch (error) {
        console.error('Error reading test results:', error);
        res.status(500).json({ error: 'Failed to read test results' });
    }
});

// API endpoint to get session info
app.get('/api/session', (req, res) => {
    const absolutePath = path.resolve(dataPath);
    res.json({ 
        sessionId: sessionId,
        dataPath: absolutePath
    });
});

// Serve index.html for root
app.get('/', (req, res) => {
    res.sendFile(path.join(__dirname, 'public', 'index.html'));
});

app.listen(PORT, () => {
    console.log(`Test results viewer running at http://localhost:${PORT}`);
    console.log('Press Ctrl+C to stop the server');
});