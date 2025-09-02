# Web Viewer Backend Implementation

## Overview

The backend server is a Node.js/Express application that serves the web viewer interface and provides REST API endpoints for test result data. It manages sessions, handles file operations, and coordinates with the C++ test runner.

## Architecture

### Server Components

```
Express Server (server.js)
├── Middleware
│   ├── CORS
│   ├── JSON Parser
│   ├── Static Files
│   └── Error Handler
├── Routes
│   ├── /api/results
│   ├── /api/session
│   └── /api/status
├── Session Management
│   ├── Session Creation
│   ├── Data Storage
│   └── Cleanup
└── Process Management
    ├── Port Management
    ├── Server Lifecycle
    └── Signal Handling
```

## Implementation

### Server Setup

```javascript
const express = require('express');
const path = require('path');
const fs = require('fs').promises;
const { spawn } = require('child_process');

const app = express();
const PORT = process.env.PORT || 5000;

// Middleware
app.use(express.json());
app.use(express.static(path.join(__dirname, 'public')));

// CORS for development
app.use((req, res, next) => {
    res.header('Access-Control-Allow-Origin', '*');
    res.header('Access-Control-Allow-Headers', 
               'Origin, X-Requested-With, Content-Type, Accept');
    next();
});
```

### Session Management

```javascript
class SessionManager {
    constructor(baseDir) {
        this.baseDir = baseDir;
        this.currentSession = null;
    }
    
    async createSession(sessionId) {
        const sessionPath = path.join(this.baseDir, sessionId);
        const dataPath = path.join(sessionPath, 'data');
        
        // Create directory structure
        await fs.mkdir(dataPath, { recursive: true });
        
        this.currentSession = {
            id: sessionId,
            path: sessionPath,
            dataPath: dataPath,
            createdAt: new Date(),
            resultFile: path.join(dataPath, 'result.json')
        };
        
        return this.currentSession;
    }
    
    async getSessionData() {
        if (!this.currentSession) {
            throw new Error('No active session');
        }
        
        const data = await fs.readFile(
            this.currentSession.resultFile, 'utf8'
        );
        return JSON.parse(data);
    }
    
    async cleanup(olderThan = 24 * 60 * 60 * 1000) {
        const sessions = await fs.readdir(this.baseDir);
        const now = Date.now();
        
        for (const session of sessions) {
            const sessionPath = path.join(this.baseDir, session);
            const stats = await fs.stat(sessionPath);
            
            if (now - stats.mtimeMs > olderThan) {
                await fs.rm(sessionPath, { recursive: true });
                console.log(`Cleaned up old session: ${session}`);
            }
        }
    }
}

const sessionManager = new SessionManager(
    path.join(__dirname, 'sessions')
);
```

## API Endpoints

### GET /api/results

Returns test results for the current session:

```javascript
app.get('/api/results', async (req, res) => {
    try {
        const data = await sessionManager.getSessionData();
        res.json(data);
    } catch (error) {
        console.error('Error reading results:', error);
        res.status(500).json({ 
            error: 'Failed to load results',
            message: error.message 
        });
    }
});
```

Response format:
```json
{
    "metadata": {
        "version": "1.0.0",
        "total_tests": 24,
        "passed": 9,
        "failed": 0,
        "not_applicable": 15,
        "not_yet_implemented": 9,
        "pass_rate": 37.5,
        "total_time_ms": 964.42
    },
    "results": [
        {
            "id": "001",
            "category": "Critical Path",
            "test_type": "WebGPU Instance Creation",
            "passed": true,
            "execution_time_ms": 215.05,
            "expected_result": "Valid instance created",
            "actual_result": "Valid instance created",
            "log_messages": [...]
        }
    ]
}
```

### GET /api/session

Returns current session information:

```javascript
app.get('/api/session', (req, res) => {
    if (!sessionManager.currentSession) {
        return res.status(404).json({ 
            error: 'No active session' 
        });
    }
    
    res.json({
        sessionId: sessionManager.currentSession.id,
        createdAt: sessionManager.currentSession.createdAt,
        path: sessionManager.currentSession.path
    });
});
```

### GET /api/status

Health check endpoint:

```javascript
app.get('/api/status', (req, res) => {
    res.json({
        status: 'running',
        uptime: process.uptime(),
        memory: process.memoryUsage(),
        session: sessionManager.currentSession ? 'active' : 'none'
    });
});
```

### POST /api/session

Create a new session:

```javascript
app.post('/api/session', async (req, res) => {
    try {
        const sessionId = req.body.sessionId || Date.now().toString();
        const session = await sessionManager.createSession(sessionId);
        
        res.json({
            success: true,
            session: {
                id: session.id,
                path: session.path
            }
        });
    } catch (error) {
        res.status(500).json({ 
            error: 'Failed to create session',
            message: error.message 
        });
    }
});
```

## Process Management

### Port Availability Check

```javascript
async function isPortAvailable(port) {
    return new Promise((resolve) => {
        const server = require('net').createServer();
        
        server.once('error', (err) => {
            if (err.code === 'EADDRINUSE') {
                resolve(false);
            } else {
                resolve(true);
            }
        });
        
        server.once('listening', () => {
            server.close();
            resolve(true);
        });
        
        server.listen(port);
    });
}
```

### Kill Existing Server

```javascript
async function killExistingServer(port) {
    if (process.platform === 'win32') {
        // Windows: Find and kill process using the port
        const { exec } = require('child_process');
        
        return new Promise((resolve) => {
            exec(`netstat -ano | findstr :${port}`, (err, stdout) => {
                if (err || !stdout) {
                    resolve(); // No process found
                    return;
                }
                
                const lines = stdout.trim().split('\n');
                const pids = new Set();
                
                lines.forEach(line => {
                    const parts = line.trim().split(/\s+/);
                    const pid = parts[parts.length - 1];
                    if (pid && pid !== '0') {
                        pids.add(pid);
                    }
                });
                
                pids.forEach(pid => {
                    console.log(`Killing process ${pid} on port ${port}`);
                    exec(`taskkill /PID ${pid} /F`, () => {});
                });
                
                setTimeout(resolve, 1000); // Wait for processes to die
            });
        });
    } else {
        // Unix-like systems
        const { exec } = require('child_process');
        
        return new Promise((resolve) => {
            exec(`lsof -ti:${port} | xargs kill -9`, () => {
                resolve();
            });
        });
    }
}
```

### Server Lifecycle Management

```javascript
class ServerManager {
    constructor(app, port) {
        this.app = app;
        this.port = port;
        this.server = null;
    }
    
    async start() {
        // Check if port is in use
        const available = await isPortAvailable(this.port);
        
        if (!available) {
            console.log(`Port ${this.port} is in use, killing existing server...`);
            await killExistingServer(this.port);
            
            // Wait a moment for port to be released
            await new Promise(resolve => setTimeout(resolve, 2000));
        }
        
        return new Promise((resolve, reject) => {
            this.server = this.app.listen(this.port, () => {
                console.log(`Server running on http://localhost:${this.port}`);
                resolve(this.server);
            });
            
            this.server.on('error', reject);
        });
    }
    
    stop() {
        return new Promise((resolve) => {
            if (this.server) {
                this.server.close(() => {
                    console.log('Server stopped');
                    resolve();
                });
            } else {
                resolve();
            }
        });
    }
}
```

## Integration with Test Runner

### Launch from C++

```cpp
void launchWebViewer(const std::string& sessionId, 
                     const std::string& resultPath) {
    // Copy result to session directory
    std::string sessionDir = "web/sessions/" + sessionId + "/data";
    std::filesystem::create_directories(sessionDir);
    std::filesystem::copy_file(
        resultPath,
        sessionDir + "/result.json",
        std::filesystem::copy_options::overwrite_existing
    );
    
    // Launch server
    std::string command = "cd web && npm start -- --session=" + sessionId;
    
#ifdef _WIN32
    // Windows: Use start to open in new window
    command = "start cmd /c \"" + command + "\"";
#else
    // Unix: Use terminal emulator
    command = "gnome-terminal -- " + command;
#endif
    
    system(command.c_str());
    
    // Open browser
    std::this_thread::sleep_for(std::chrono::seconds(2));
#ifdef _WIN32
    system("start http://localhost:5000");
#else
    system("xdg-open http://localhost:5000");
#endif
}
```

### Command Line Arguments

```javascript
// Parse command line arguments
const args = process.argv.slice(2);
const sessionId = args.find(arg => arg.startsWith('--session='))
                      ?.split('=')[1] || Date.now().toString();

// Initialize with session
(async () => {
    try {
        await sessionManager.createSession(sessionId);
        const serverManager = new ServerManager(app, PORT);
        await serverManager.start();
    } catch (error) {
        console.error('Failed to start server:', error);
        process.exit(1);
    }
})();
```

## Error Handling

### Global Error Handler

```javascript
// 404 handler
app.use((req, res) => {
    res.status(404).json({ 
        error: 'Not found',
        path: req.path 
    });
});

// Global error handler
app.use((err, req, res, next) => {
    console.error('Server error:', err.stack);
    
    res.status(err.status || 500).json({
        error: err.message || 'Internal server error',
        stack: process.env.NODE_ENV === 'development' ? err.stack : undefined
    });
});
```

### Graceful Shutdown

```javascript
// Handle shutdown signals
const signals = ['SIGTERM', 'SIGINT', 'SIGUSR2'];

signals.forEach(signal => {
    process.on(signal, async () => {
        console.log(`Received ${signal}, shutting down gracefully...`);
        
        try {
            // Close server
            await serverManager.stop();
            
            // Cleanup sessions
            await sessionManager.cleanup();
            
            console.log('Shutdown complete');
            process.exit(0);
        } catch (error) {
            console.error('Error during shutdown:', error);
            process.exit(1);
        }
    });
});

// Handle uncaught exceptions
process.on('uncaughtException', (error) => {
    console.error('Uncaught exception:', error);
    process.exit(1);
});

process.on('unhandledRejection', (reason, promise) => {
    console.error('Unhandled rejection at:', promise, 'reason:', reason);
    process.exit(1);
});
```

## File Operations

### Async File Handling

```javascript
class FileManager {
    async readJSON(filepath) {
        try {
            const data = await fs.readFile(filepath, 'utf8');
            return JSON.parse(data);
        } catch (error) {
            if (error.code === 'ENOENT') {
                throw new Error(`File not found: ${filepath}`);
            } else if (error.name === 'SyntaxError') {
                throw new Error(`Invalid JSON in file: ${filepath}`);
            }
            throw error;
        }
    }
    
    async writeJSON(filepath, data) {
        const json = JSON.stringify(data, null, 2);
        await fs.writeFile(filepath, json, 'utf8');
    }
    
    async ensureDir(dirpath) {
        await fs.mkdir(dirpath, { recursive: true });
    }
    
    async copyFile(src, dest) {
        await this.ensureDir(path.dirname(dest));
        await fs.copyFile(src, dest);
    }
}
```

## Monitoring and Logging

### Request Logging

```javascript
// Morgan for HTTP request logging
const morgan = require('morgan');

// Custom format
morgan.token('session', (req) => 
    sessionManager.currentSession?.id || 'none'
);

app.use(morgan(':method :url :status :response-time ms - session: :session'));
```

### Application Logging

```javascript
class Logger {
    constructor(level = 'info') {
        this.levels = ['error', 'warn', 'info', 'debug'];
        this.level = level;
    }
    
    log(level, message, ...args) {
        if (this.levels.indexOf(level) <= this.levels.indexOf(this.level)) {
            const timestamp = new Date().toISOString();
            console.log(`[${timestamp}] [${level.toUpperCase()}] ${message}`, ...args);
        }
    }
    
    error(message, ...args) { this.log('error', message, ...args); }
    warn(message, ...args) { this.log('warn', message, ...args); }
    info(message, ...args) { this.log('info', message, ...args); }
    debug(message, ...args) { this.log('debug', message, ...args); }
}

const logger = new Logger(process.env.LOG_LEVEL || 'info');
```

## Security

### Input Validation

```javascript
// Sanitize session ID
function sanitizeSessionId(sessionId) {
    // Only allow alphanumeric and basic punctuation
    return sessionId.replace(/[^a-zA-Z0-9_-]/g, '');
}

// Validate file paths
function isPathSafe(filepath) {
    const normalized = path.normalize(filepath);
    const resolved = path.resolve(filepath);
    
    // Prevent directory traversal
    if (normalized.includes('..')) {
        return false;
    }
    
    // Ensure path is within allowed directory
    const allowedDir = path.resolve(__dirname, 'sessions');
    return resolved.startsWith(allowedDir);
}
```

### Rate Limiting

```javascript
const rateLimit = require('express-rate-limit');

const limiter = rateLimit({
    windowMs: 15 * 60 * 1000, // 15 minutes
    max: 100, // limit each IP to 100 requests per windowMs
    message: 'Too many requests from this IP'
});

app.use('/api/', limiter);
```

## Performance

### Caching

```javascript
class Cache {
    constructor(ttl = 60000) { // 1 minute default
        this.cache = new Map();
        this.ttl = ttl;
    }
    
    set(key, value) {
        this.cache.set(key, {
            value,
            expires: Date.now() + this.ttl
        });
    }
    
    get(key) {
        const item = this.cache.get(key);
        
        if (!item) return null;
        
        if (Date.now() > item.expires) {
            this.cache.delete(key);
            return null;
        }
        
        return item.value;
    }
    
    clear() {
        this.cache.clear();
    }
}

const resultCache = new Cache(5 * 60 * 1000); // 5 minutes

// Use cache in endpoint
app.get('/api/results', async (req, res) => {
    const cached = resultCache.get('results');
    
    if (cached) {
        return res.json(cached);
    }
    
    const data = await sessionManager.getSessionData();
    resultCache.set('results', data);
    res.json(data);
});
```

### Compression

```javascript
const compression = require('compression');

app.use(compression({
    level: 6,
    threshold: 1024, // Only compress responses > 1KB
    filter: (req, res) => {
        // Compress JSON and text
        const type = res.getHeader('Content-Type');
        return /json|text|javascript/.test(type);
    }
}));
```

## Deployment

### Production Configuration

```javascript
// config/production.js
module.exports = {
    port: process.env.PORT || 3000,
    sessionDir: process.env.SESSION_DIR || '/var/lib/test-viewer/sessions',
    logLevel: process.env.LOG_LEVEL || 'warn',
    cors: {
        origin: process.env.CORS_ORIGIN || false,
        credentials: true
    },
    ssl: {
        enabled: process.env.SSL_ENABLED === 'true',
        key: process.env.SSL_KEY_PATH,
        cert: process.env.SSL_CERT_PATH
    }
};
```

### Docker Support

```dockerfile
FROM node:18-alpine

WORKDIR /app

COPY package*.json ./
RUN npm ci --only=production

COPY . .

EXPOSE 5000

CMD ["node", "server.js"]
```

## Testing

### Unit Tests

```javascript
// test/server.test.js
const request = require('supertest');
const app = require('../server');

describe('API Endpoints', () => {
    test('GET /api/status returns 200', async () => {
        const response = await request(app).get('/api/status');
        expect(response.status).toBe(200);
        expect(response.body.status).toBe('running');
    });
    
    test('GET /api/results returns 404 without session', async () => {
        const response = await request(app).get('/api/results');
        expect(response.status).toBe(404);
    });
});
```