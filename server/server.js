const express = require('express');
const session = require('express-session');
const cors = require('cors');
const mqtt = require('mqtt');
const bcrypt = require('bcryptjs');
const path = require('path');
require('dotenv').config();

const app = express();
const port = process.env.PORT || 3000;

// Middleware
app.use(cors());
app.use(express.json());
app.use(express.static(path.join(__dirname, '../'))); // Serve static files from parent directory

// Session configuration
app.use(session({
    secret: process.env.SESSION_SECRET || 'your-secret-key',
    resave: false,
    saveUninitialized: false,
    cookie: {
        secure: process.env.NODE_ENV === 'production',
        maxAge: 30 * 60 * 1000 // 30 minutes
    }
}));

// MQTT Configuration
const mqttClient = mqtt.connect(process.env.MQTT_BROKER || 'mqtt://broker.hivemq.com');

mqttClient.on('connect', () => {
    console.log('Connected to MQTT broker');
    mqttClient.subscribe('ac/control');
});

mqttClient.on('message', (topic, message) => {
    console.log(`Received message on ${topic}: ${message.toString()}`);
});

// User database (in production, use a real database)
const users = {
    'admin': {
        password: bcrypt.hashSync('admin123', 10),
        role: 'admin'
    },
    'user': {
        password: bcrypt.hashSync('user123', 10),
        role: 'user'
    },
    'CEO@bss': {
        password: bcrypt.hashSync('coral@1234', 10),
        role: 'admin'
    }
};

// Authentication middleware
const authenticate = (req, res, next) => {
    if (req.session.user) {
        next();
    } else {
        res.status(401).json({ error: 'Unauthorized' });
    }
};

// Routes
app.post('/api/login', (req, res) => {
    const { username, password } = req.body;
    const user = users[username];

    if (user && bcrypt.compareSync(password, user.password)) {
        req.session.user = {
            username,
            role: user.role
        };
        res.json({ 
            success: true, 
            user: { username, role: user.role },
            message: `Welcome, ${username}!`
        });
    } else {
        res.status(401).json({ error: 'Invalid credentials' });
    }
});

app.post('/api/logout', (req, res) => {
    req.session.destroy();
    res.json({ success: true });
});

app.get('/api/status', authenticate, (req, res) => {
    res.json({ 
        authenticated: true, 
        user: req.session.user,
        mqttConnected: mqttClient.connected
    });
});

// MQTT control endpoint
app.post('/api/control', authenticate, (req, res) => {
    const { command } = req.body;
    
    if (!command) {
        return res.status(400).json({ error: 'Command is required' });
    }

    mqttClient.publish('ac/control', command, (err) => {
        if (err) {
            console.error('MQTT publish error:', err);
            res.status(500).json({ error: 'Failed to send command' });
        } else {
            res.json({ success: true, command });
        }
    });
});

// Serve the main application
app.get('/', (req, res) => {
    res.sendFile(path.join(__dirname, '../index.html'));
});

// Error handling
app.use((err, req, res, next) => {
    console.error(err.stack);
    res.status(500).json({ error: 'Something went wrong!' });
});

// Start server
app.listen(port, () => {
    console.log(`Server running on port ${port}`);
}); 