import { mqttConnect, mqttStartBath, mqttStopBath, mqttAdjustTemp, mqttSetBathSize, TOPICS } from "./server.js"

const MIN_TEMP = 30, MAX_TEMP = 42;
const MIN_SIZE = 2, MAX_SIZE = 30, SIZE_STEP = 2;
let lastNotifiedState = null;
export let state = {
    targetTemp: 38,
    bathSize: 10,
    currentTemp: null,
    progress: 0,
    running: false,
    appConnected: true,
    bathStatus: 'IDLE',
    notifications: [],
    unreadCount: 0,
};

// ELEMENTS
const startBtn = document.getElementById('start-btn');
const tempDisplay = document.getElementById('temp-target-display');
const targetConfirm = document.getElementById('target-confirm');
const currentTemp = document.getElementById('current-temp');
const progressFill = document.getElementById('progress-fill');
const progressPct = document.getElementById('progress-percent');
const dotApp = document.getElementById('dot-app');
const valApp = document.getElementById('val-app');
const dotBath = document.getElementById('dot-bath');
const valBath = document.getElementById('val-bath');
const notifList = document.getElementById('notif-list');
const notifEmpty = document.getElementById('notif-empty');
const notifNew = document.getElementById('notif-new');
const sizeDisplay = document.getElementById('size-display');
const sizeConfirm = document.getElementById('size-confirm');

// TABS
export function switchTab(name, btn) {
    document.querySelectorAll('.tab-content').forEach(t => t.classList.remove('active'));
    document.querySelectorAll('.nav-btn').forEach(b => b.classList.remove('active'));
    document.getElementById(`tab-${name}`).classList.add('active');
    btn.classList.add('active');
    if (name === 'notif') {
        state.unreadCount = 0;
        notifNew.classList.remove('visible');
    }
}

// NOTIFICATIONS
export function addNotification(msg) {
    const n = { msg, time: new Date()};
    state.notifications.push(n);
    state.unreadCount++;
    notifNew.classList.add('visible');
    updateNotifications();
}

function updateNotifications() {
    const items = state.notifications;
    notifEmpty.style.display = items.length ? 'none' : 'block';
    const existing = notifList.querySelectorAll('.notif-item');
    existing.forEach(el => el.remove());
    items.forEach(n => {
        const el = document.createElement('div');
        el.className = 'notif-item';
        const timeStr = n.time.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit'});
        el.innerHTML = `
        <div class="notif-body">
            <div class="notif-msg">${n.msg}</div>
            <div class="notif-time">${timeStr}</div>
        </div>`;
        notifList.appendChild(el);
    });
}

function convertNotification(st) {
    const temp = state.currentTemp;
    const tempStr = (temp != null) ? `${temp.toFixed(1)}°C` : 'unknown';
    const target = state.targetTemp;
    switch (st) {
        case 'IDLE':
            return `Bath is ready.
                    \nCurrent temperature: ${tempStr}`;
        case 'FILLING':
            return `Bath is filling.
                \nTarget ${target}°C, current ${tempStr}`;
        
        case 'SOFT_WARNING':
            if (temp == null)
                return `Adjusting temperature.\n
                    Target ${target}°C`;
            return temp < target
                ? `Temperature too low, adjusting.\n
                    Current ${tempStr}, target ${target}°C`
                : `Temperature is too high, adjusting.\n
                    Current ${tempStr}, target ${target}°C`;
        
        case 'HARD_WARNING':
            return `!! WARNING !!\n
                Temperature too high (${tempStr})!\n
                Do not enter the bath!`;
        
        case 'LOST_CONNECTION':
            return `Connection could not be established with bath.\n
                Filling has stopped`;
        case 'SENSOR_FAULT':
            return `Temperature Sensor fault. Filling has stopped.\n
                Last known temperature: ${tempStr}`;
        default:
            return `Bath status: ${st}`;
    }
}

function handleMessage(topic, payload) {
    switch (topic) {
        case TOPICS.subscribe.connected:
            setAppConnected(payload === 'True');
            break;
        case TOPICS.subscribe.status: {
            let data;
            try {
                data = JSON.parse(payload);
            } catch {
                console.log('Bad status payload:', payload);
                break;
            }

            if (typeof data.temp === 'number') 
                setCurrentTemp(data.temp);
            
            if (typeof data.progress === 'number') {
                state.progress = data.progress;
                showProgress();
            }

            const st = data.state;
            const wasRunning = state.running;
            state.running = (st === 'FILLING' || st === 'SOFT_WARNING' || st === 'HARD_WARNING');
            state.bathStatus = st;

            const notif = convertNotification(st);
            const stateChanged = (st !== lastNotifiedState);

            if (!state.running && wasRunning) {
                stopBath(notif);
            } else {
                if (stateChanged) addNotification(notif);
                updateBathStatus();
            }
            lastNotifiedState = st;
            break;
        }
    }
}

// Listener events
document.querySelectorAll('.nav-btn').forEach(btn => {
    btn.addEventListener('click', () => {
        switchTab(btn.dataset.tab, btn);
    });
});

document.getElementById('clear-btn').addEventListener('click', () => {
    state.notifications = [];
    state.unreadCount = 0;
    notifNew.classList.remove('visible');
    updateNotifications();
});

document.getElementById('btn-size-up').addEventListener('click', () => {
    if (state.bathSize < MAX_SIZE) {
        state.bathSize += SIZE_STEP; 
        updateSize(); 
    }
});
document.getElementById('btn-size-down').addEventListener('click', () => {
    if (state.bathSize > MIN_SIZE) { 
        state.bathSize -= SIZE_STEP; 
        updateSize(); 
    }
});

function updateSize() {
    sizeDisplay.textContent = state.bathSize;
}

//     TEMPERATURE
document.getElementById('btn-up').addEventListener('click', () => {
    if (state.targetTemp < MAX_TEMP) {
        state.targetTemp++;
        updateTemp();
    }
});
document.getElementById('btn-down').addEventListener('click', () => {
    if (state.targetTemp > MIN_TEMP) {
        state.targetTemp--;
        updateTemp();
    }
});

function updateTemp(){
    tempDisplay.textContent = state.targetTemp;
    targetConfirm.textContent = `${state.targetTemp}°C`;
    tempDisplay.style.color = state.targetTemp > 42 ? 'var(--danger)':'var(--accent)';
    if (state.running){
        mqttAdjustTemp(state.targetTemp);
        addNotification(`Adjusted target temperature to ${state.targetTemp}°C`);
    }
}

//     START BUTTON
startBtn.addEventListener('click', () => {
    if (!state.running) {
        const start_pub = mqttStartBath(state.targetTemp);
        if (!start_pub) {
            addNotification('Can not communicate with Bath, please check connection');
            return;
        }
        mqttSetBathSize(state.bathSize);
        startBath(`Commencing filling the tub. Target temp ${state.targetTemp}°C, size ${state.bathSize}L`);
    } else {
        mqttStopBath();
        stopBath('User requested bath filling to stop');
    }
});

export function startBath(notif){
    state.running = true;
    state.bathStatus = 'FILLING';
    lastNotifiedState = 'FILLING';
    startBtn.className = 'start-btn running';
    startBtn.textContent = 'Stop Bath';
    addNotification(notif);
    updateBathStatus();
    showProgress();
}
export function stopBath(notif) {
    state.running = false;
    state.bathStatus = 'IDLE';
    state.progress = 0;
    lastNotifiedState = 'IDLE';
    startBtn.className = 'start-btn idle';
    startBtn.textContent = 'Start Bath';
    addNotification(notif);
    updateBathStatus();
    showProgress();
}

//    BATH STATUS
export function updateBathStatus() {
    const s = state.bathStatus;
    const dotClass = { IDLE: '', FILLING: 'filling', SOFT_WARNING: 'warning', HARD_WARNING: 'warning', LOST_CONNECTION: 'disconnected' };
    const label = { IDLE: 'Idle', FILLING: 'Filling', SOFT_WARNING: 'Adjusting', HARD_WARNING: 'Warning!', LOST_CONNECTION: 'Lost Signal' };
    dotBath.className = 'status-dot ' + (dotClass[s] || '');
    valBath.textContent = label[s];
} 

//   APP CONNECTION STATUS  
export function setAppConnected(connected) {
    state.appConnected = connected;
    dotApp.className = 'status-dot ' + (connected ? 'connected' : 'disconnected');
    valApp.textContent = connected ? 'Connected' : 'Disconnected';
}

//   PROGRESS  
export function showProgress() {
    const pct = Math.round(state.progress);
    progressFill.style.width = `${pct}%`;
    progressPct.textContent = `${pct}%`;
}

export function setCurrentTemp(t) {
    if (typeof t !== 'number' || Number.isNaN(t)) return;
    state.currentTemp = t;
    currentTemp.textContent = `${t.toFixed(1)}°C`;
}

updateBathStatus();
updateTemp();
showProgress();
updateNotifications();
mqttConnect({
    onConnect: () => setAppConnected(true),
    onDisconnect: () => setAppConnected(false),
    onMessage: handleMessage,
});