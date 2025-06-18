// Configuration
const config = {
    api: {
        baseUrl: 'http://localhost:3000/api'
    },
    sessionTimeout: 30 * 60 * 1000 // 30 minutes
};

// Session timer
let sessionTimer = null;

// DOM Elements
const loginScreen = document.getElementById('loginScreen');
const remoteInterface = document.getElementById('remoteInterface');
const loginForm = document.getElementById('loginForm');
const logoutBtn = document.getElementById('logoutBtn');
const currentStatus = document.getElementById('currentStatus');
const currentTemp = document.getElementById('currentTemp');

// Check for existing session
async function checkSession() {
    try {
        const response = await fetch(`${config.api.baseUrl}/status`, {
            credentials: 'include'
        });
        
        if (response.ok) {
            const data = await response.json();
            if (data.authenticated) {
                showRemoteInterface();
                startSessionTimer();
            }
        }
    } catch (error) {
        console.error('Session check failed:', error);
    }
}

// Start session timer
function startSessionTimer() {
    if (sessionTimer) {
        clearTimeout(sessionTimer);
    }
    sessionTimer = setTimeout(() => {
        logout();
    }, config.sessionTimeout);
}

// Handle login
loginForm.addEventListener('submit', async (e) => {
    e.preventDefault();
    const username = document.getElementById('username').value;
    const password = document.getElementById('password').value;

    try {
        const response = await fetch(`${config.api.baseUrl}/login`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            credentials: 'include',
            body: JSON.stringify({ username, password })
        });

        const data = await response.json();

        if (response.ok) {
            showRemoteInterface();
            startSessionTimer();
            // Show welcome message
            alert(data.message || 'Welcome!');
        } else {
            alert(data.error || 'Login failed. Please check your credentials.');
        }
    } catch (error) {
        console.error('Login error:', error);
        alert('Login failed. Please try again.');
    }
});

// Handle logout
logoutBtn.addEventListener('click', logout);

function logout() {
    fetch(`${config.api.baseUrl}/logout`, {
        method: 'POST',
        credentials: 'include'
    }).finally(() => {
        loginScreen.classList.remove('hidden');
        remoteInterface.classList.add('hidden');
        if (sessionTimer) {
            clearTimeout(sessionTimer);
        }
    });
}

// Send command through server
async function sendCommand(command) {
    try {
        const response = await fetch(`${config.api.baseUrl}/control`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            credentials: 'include',
            body: JSON.stringify({ command })
        });

        if (response.ok) {
            const data = await response.json();
            updateStatus(command);
        } else {
            console.error('Failed to send command');
        }
    } catch (error) {
        console.error('Command error:', error);
    }
}

// Update status display
function updateStatus(command) {
    if (command === 'ON') {
        currentStatus.textContent = 'ON';
    } else if (command === 'OFF') {
        currentStatus.textContent = 'OFF';
        currentTemp.textContent = '--°C';
    } else if (command.startsWith('TEMP:')) {
        const temp = command.split(':')[1];
        currentTemp.textContent = `${temp}°C`;
        currentStatus.textContent = 'ON';
    }
}

// Add event listeners for power buttons
document.querySelectorAll('.power-btn').forEach(button => {
    button.addEventListener('click', () => {
        const command = button.dataset.command.toUpperCase();
        sendCommand(command);
    });
});

// Add event listeners for temperature buttons
document.querySelectorAll('.temp-btn').forEach(button => {
    button.addEventListener('click', () => {
        const temp = button.dataset.temp;
        sendCommand(`TEMP:${temp}`);
    });
});

function showRemoteInterface() {
    loginScreen.classList.add('hidden');
    remoteInterface.classList.remove('hidden');
}

// Check session on page load
checkSession(); 