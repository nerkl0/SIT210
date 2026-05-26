// ── STATE ──
const MIN_TEMP = 30, MAX_TEMP = 42;
let state = {
    targetTemp: 38,
    currentTemp: null,
    progress: 0,
    running: false,
    appConnected: true,
    bathStatus: 'IDLE',
    notifications: [],
    unreadCount: 0,
};

// ── ELEMENTS ──
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

// ── TABS ──
function switchTab(name, btn) {
    document.querySelectorAll('.tab-content').forEach(t => t.classList.remove('active'));
    document.querySelectorAll('.nav-btn').forEach(b => b.classList.remove('active'));
    document.getElementById('tab-' + name).classList.add('active');
    btn.classList.add('active');
    if (name === 'notif') {
        state.unreadCount = 0;
        notifNew.classList.remove('visible');
    }
}

// ── NOTIFICATIONS ──
function addNotification(msg) {
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

document.getElementById('clear-btn').addEventListener('click', () => {
    state.notifications = [];
    state.unreadCount = 0;
    notifNew.classList.remove('visible');
    updateNotifications();
});

  // ── TEMPERATURE ──
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
    targetConfirm.textContent = state.targetTemp + '°C';
    tempDisplay.style.color = state.targetTemp > 42 ? 'var(--danger)':'var(--accent)';
}

  // ── START / STOP ──
startBtn.addEventListener('click', () => {
    if (!state.running) {
        state.running = true;
        state.bathStatus = 'FILLING';
        startBtn.className = 'start-btn running';
        startBtn.textContent = 'Stop Bath';
        addNotification('Bath has started filling — target ' + state.targetTemp + '°C');
        updateBathStatus();
    } else {
        stopBath('User stopped the bath');
    }
});

function stopBath(notif) {
    state.running = false;
    state.bathStatus = 'IDLE';
    state.progress = 0;
    startBtn.className = 'start-btn idle';
    startBtn.textContent = 'Start Bath';
    addNotification(notif);
    updateBathStatus();
    showProgress();
}

// ── BATH STATUS ──
function updateBathStatus() {
    const s = state.bathStatus;
    const dotClass = { IDLE: '', FILLING: 'filling', SOFT_WARNING: 'warning', HARD_WARNING: 'warning', LOST_CONNECTION: 'disconnected' };
    const label = { IDLE: 'Idle', FILLING: 'Filling', SOFT_WARNING: 'Adjusting', HARD_WARNING: 'Warning!', LOST_CONNECTION: 'Lost Signal' };
    dotBath.className = 'status-dot ' + (dotClass[s] || '');
    valBath.textContent = label[s] || s;
}

// ── APP CONNECTION STATUS ──
function setAppConnected(connected) {
    state.appConnected = connected;
    dotApp.className = 'status-dot ' + (connected ? 'connected' : 'disconnected');
    valApp.textContent = connected ? 'Connected' : 'Disconnected';
}

// ── PROGRESS ──
function showProgress() {
    const pct = Math.round(state.progress);
    progressFill.style.width = pct + '%';
    progressPct.textContent = pct + '%';
}

// ── INIT ──
setAppConnected(true);
updateBathStatus();
updateTemp();
showProgress();
updateNotifications();
