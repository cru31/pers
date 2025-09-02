const express = require('express');
const path = require('path');
const fs = require('fs');

const app = express();
const PORT = 5000;

// Get session ID from command line argument
const sessionId = process.argv[2] || 'default';
const dataPath = path.join(__dirname, sessionId, 'data', 'result.json');

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
    res.json({ sessionId: sessionId });
});

// Serve index.html for root
app.get('/', (req, res) => {
    res.sendFile(path.join(__dirname, 'public', 'index.html'));
});

app.listen(PORT, () => {
    console.log(`Test results viewer running at http://localhost:${PORT}`);
    console.log('Press Ctrl+C to stop the server');
});