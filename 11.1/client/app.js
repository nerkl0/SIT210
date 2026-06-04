import { mqttConnect, mqttStartBath, mqttStopBath, mqttAdjustTemp, mqttSetBathSize, mqttSilenceWarnings, TOPICS } from "./server.js"

/*  DOM ELEMENTS */
const startBtn = document.getElementById('start-btn');
const tempDisplay = document.getElementById('temp-target-display');
const targetConfirm = document.getElementById('target-confirm');
const currentTemp = document.getElementById('current-temp');
const progressFill = document.getElementById('progress-fill');
const progressPct = document.getElementById('progress-percent');
const appIndicator = document.getElementById('app-ind');
const valApp = document.getElementById('val-app');
const bathIndicator = document.getElementById('bath-ind');
const valBath = document.getElementById('val-bath');
const notifList = document.getElementById('notif-list');
const notifEmpty = document.getElementById('notif-empty');
const notifNew = document.getElementById('notif-new');
const sizeDisplay = document.getElementById('size-display');
const addWaterBtn = document.getElementById('add-water-btn');
const silenceBtn = document.getElementById('silence-btn');

const bath = {
    MIN_TEMP: 30,
    MAX_TEMP: 42,
    MIN_SIZE: 2,
    MAX_SIZE: 30,
    SIZE_STEP: 2
}

let lastNotifiedState = null; // variable to ensure only a single notification per state is added to notifications
const START_CONFIRM_TIMEOUT = 10000; // Timeout if the firmware does not confirm filling after start pressed 
let startTimeoutConf = null; // keeps track of timeout count, callable as a variable in clearTimeout

// Separation of different states where bath is confirmed to be running/not running
const RUNNING_STATES = ['FILLING', 'SOFT_WARNING', 'HARD_WARNING'];
const WARNING_STATES = ['HARD_WARNING', 'SENSOR_FAULT'];
const FAULT_STATES = ['SENSOR_FAULT', 'LOST_CONNECTION'];

const MSG_COMMS_FAIL = 'Can not communicate with Bath, please check connection';

// Bath state defaults
let state = {
    targetTemp: 38,
    bathSize: 10,
    currentTemp: null,
    progress: 0,
    running: false,
    appConnected: false,
    bathStatus: 'IDLE',
    addMore: false,
    silenced: false,
    pendingStart: false,
    pendingNotif: null,
    notifications: [],
};

const BATH_STATUS = {
    IDLE:{ label: 'Idle', cls: '' },
    FILLING: { label: 'Filling', cls: 'filling' },
    SOFT_WARNING:{ label: 'Adjusting', cls: 'warning' },
    HARD_WARNING: { label:'Warning!', cls: 'warning' },
    LOST_CONNECTION: { label: 'Lost Signal', cls: 'disconnected' },
    SENSOR_FAULT:{ label: 'Sensor Fault', cls: 'warning' }
};

// STYLE strings to manipulate css properties
const STYLE = {
    startBtn: {
        idle: 'start-btn idle',
        running: 'start-btn running',
    },
    indicator: 'status-ind',
    indicatorState: {
        IDLE: '',
        FILLING: 'filling',
        SOFT_WARNING: 'warning',
        HARD_WARNING: 'warning',
        LOST_CONNECTION: 'disconnected',
        SENSOR_FAULT: 'warning'
    },
};


/*  =====  HELPER FUNCTIONS  =====  */

// Builds a full className for the bath/app indicator from STYLE
const indicatorClass = (classModifier) => `${STYLE.indicator} ${classModifier || ''}`.trim();

//  Function expressions for state checks
const isActive = () => state.running || state.pendingStart; // pending included so that the start button stays inactive
const isRunningState = (st) => RUNNING_STATES.includes(st);
const isWarningState = (st) => WARNING_STATES.includes(st);
const isFaultState = (st) => FAULT_STATES.includes(st);
const commsError = () => addNotification(MSG_COMMS_FAIL);


/*  =====  NOTIFICATIONS  =====  */

// Notifications generated for each bath state
const NOTIF_TEMPLATES = {
    IDLE:(tmpStr) => `Bath is ready.\nCurrent temperature: ${tmpStr}`,
    FILLING: (tmpStr, target) => `Bath is filling.\nTarget ${target}°C, current ${tmpStr}`,
    SOFT_WARNING: (tmpStr, target, temp) => {
        if (temp == null) return `Adjusting temperature.\nTarget ${target}°C`;
        return temp < target
            ? `Temperature too low, adjusting.\nCurrent ${tmpStr}, target ${target}°C`
            : `Temperature is too high, adjusting.\nCurrent ${tmpStr}, target ${target}°C`;
    },
    HARD_WARNING: (tmpStr) => `!! WARNING !!\nTemperature too high (${tmpStr})!\nDo not enter the bath!`,
    LOST_CONNECTION: () => `Connection could not be established with bath.\nFilling has stopped`,
    SENSOR_FAULT: (tmpStr) => `Temperature Sensor fault. Filling has stopped.\nLast known temperature: ${tmpStr}`,
};

// Creates a new notification, updates the indicator on notifications tab
function addNotification(msg) {
    const n = { msg, time: new Date()};
    state.notifications.push(n);
    notifNew.classList.add('visible');
    notifEmpty.style.display = 'none';
    notifList.appendChild(createNotifElement(n));
}

// Build a notification element, appending the time it was sent and add to notifications body
function createNotifElement(n) {
    const el = document.createElement('div');
    el.className = 'notif-item';
    const body = document.createElement('div');
    body.className = 'notif-body';
    const msg = document.createElement('div');
    msg.className = 'notif-msg';
    msg.textContent = n.msg;
    const time = document.createElement('div');
    time.className = 'notif-time';
    time.textContent = n.time.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
    body.append(msg, time);
    el.append(body);
    return el;
}

// Helper function to convert current state into a notification
function convertToNotification(st) {
    const temp = state.currentTemp;
    const tempStr = (temp != null) ? `${temp.toFixed(1)}°C` : 'unknown';
    const fn = NOTIF_TEMPLATES[st];
    return fn ? fn(tempStr, state.targetTemp, temp) : `Bath status: ${st}`;
}

// Rebuild the notification body. Callable mainly for clear notifications but handles building initial body on load
function updateNotifications() {
    const items = state.notifications;
    notifEmpty.style.display = items.length ? 'none' : 'block';
    notifList.querySelectorAll('.notif-item').forEach(el => el.remove());
    items.forEach(n => notifList.appendChild(createNotifElement(n)));
}



/*  =====  MQTT HANDLING FUNCTIONS  =====  */

// Parse and validate a status payload. Returns the parsed object, or null if the payload is unusable (logs reason to console).
function parseStatusPayload(payload) {
    let data;
    try {
        data = JSON.parse(payload);
    } catch {
        console.log('Bad status payload:', payload);
        return null;
    }
    if (typeof data.state !== 'string') {
        console.log('Status payload missing state:', payload);
        return null;
    }
    return data;
}

/* 
   State becomes pending after Start button is hit. The UI is held in a pending state which does not update progress/bath status etc
   until the firmware responds with a state other than IDLE. Start button stays in a clicked state. 
   If the state is running, the pending UI is cleared and updates notifications, else it returns false
   Returns true if the caller decides to stop the bath within this state.
   startTimeoutConf is configured and resolved within uiUpdateStart()
*/
function resolvePendingStart(st) {
    // Ignore a stale IDLE so the UI doesn't fall back to idle before filling actually begins.
    if (state.pendingStart && st === 'IDLE') {
        lastNotifiedState = st;
        return true;
    }
    if (state.pendingStart && isRunningState(st)) {
        clearTimeout(startTimeoutConf);
        state.pendingStart = false;
        if (state.pendingNotif) {
            addNotification(state.pendingNotif);
            state.pendingNotif = null;
        }
    }
    return false;
}

// Update the fill-progress bar from the progress topic only if bath is running 
// Ignores progress for "add more" button 
function applyProgress(data, st) {
    const progressActive = isRunningState(st);
    if (typeof data.progress === 'number' && !state.addMore && progressActive) {
        state.progress = data.progress;
        showProgress();
    }
}

// Handle hard fault states (sensor fault / lost connection): stop the bath,
// reset state and lock the UI. Returns true when the fault state is resolved
function handleFaultState(st, notif, stateChanged) {
    if (!isFaultState(st)) 
        return false;

    if (stateChanged) 
        addNotification(notif);
    state.running = false;
    state.pendingStart = false;
    state.addMore = false;
    state.progress = 0;
    
    clearTimeout(startTimeoutConf);
    resetStartButton();
    startBtn.disabled = true;

    showProgress();
    updateBathStatus();
    
    lastNotifiedState = st;
    return true;
}

// Handle for normal status transitions: detect stop, enable the start button, send notifications
// Linked to appConnected. If appConnected is false, disable buttons else should be enabled
function handleRunningTransition(st, notif, wasRunning, stateChanged) {
    // Send notification on first detection of IDLE transition 
    if (!state.running && wasRunning) { 
        uiUpdateStop(notif);
        return;
    }

    if (st === 'IDLE' || st === 'FILLING') 
        startBtn.disabled = !state.appConnected;
    
    if (stateChanged && st !== 'IDLE') 
        addNotification(notif);

    updateBathStatus();
}

// Process the incoming message from firmware. Calls parseStatusPayload returning 
// if data is corrupted or missing. Updates status of each component on the UI
function processJSONMessage(payload) {
    const data = parseStatusPayload(payload);
    if (!data) return;

    if (typeof data.temp === 'number') 
        setCurrentTemp(data.temp);

    const st = data.state;
    if (resolvePendingStart(st)) 
        return;

    applyProgress(data, st);

    const wasRunning = state.running;
    state.running = isRunningState(st);
    state.bathStatus = st;

    const notif = convertToNotification(st);
    const stateChanged = (st !== lastNotifiedState);

    refreshSilenceState(st, stateChanged);

    if (handleFaultState(st, notif, stateChanged)) 
        return;

    handleRunningTransition(st, notif, wasRunning, stateChanged);
    lastNotifiedState = st;
}

// Splits topics, if the payload topic is 'connected' calls setAppConnected
// else for status calls processJSONMessage
function routeTopics(topic, payload) {
    switch (topic) {
        case TOPICS.subscribe.connected:
            setAppConnected(payload === 'True');
            break;
        case TOPICS.subscribe.status:
            processJSONMessage(payload);
            break;
    }
}

/*  =====  UI COMPONENTS  =====  */

// The buzzer should only ever be silenced for any one particular state. This function 
// resets back to default "FALSE" after the state moves out of a warning state or if there
// is no longer any warning
function refreshSilenceState(st, stateChanged) {
    const warning = isWarningState(st);
    if ((stateChanged && warning) || !warning) 
        state.silenced = false;
}

// CURRENT TEMP ELEMENT
// Sets current temp of the bath fixed to 1 decimal place
function setCurrentTemp(t) {
    if (typeof t !== 'number' || Number.isNaN(t)) 
        return;
    state.currentTemp = t;
    currentTemp.textContent = `${t.toFixed(1)}°C`;
}

// Sets connection status comparing against previous status to provide a notification
// on first transition out of a state
function setAppConnected(connected) {
    const wasConnected = state.appConnected;
    state.appConnected = connected;
    appIndicator.className = indicatorClass(connected ? 'connected' : 'disconnected');
    valApp.textContent = connected ? 'Connected' : 'Disconnected';
    startBtn.disabled = !connected;
    updateBathStatus();
    if (connected && !wasConnected)
        addNotification('Connection with bath successful');
    console.log('start disabled =', startBtn.disabled);
}

// PROGRESS ELEMENT
// Increments the progress bar as status is updated. Ignores if the topic is addMore
function showProgress() {
    if (state.addMore) {
        progressFill.style.width = '0%';
        progressPct.textContent = '—';
        return;
    }
    const pct = Math.round(state.progress);
    progressFill.style.width = `${pct}%`;
    progressPct.textContent = `${pct}%`;
}

// NAVIGATION
// Switches between Home tab and Notifications tab
function switchTab(name, btn) {
    document.querySelectorAll('.tab-content').forEach(t => t.classList.remove('active'));
    document.querySelectorAll('.nav-btn').forEach(b => b.classList.remove('active'));
    document.getElementById(`tab-${name}`).classList.add('active');
    btn.classList.add('active');
    
    // on switch to notifications, remove indicator
    if (name === 'notif') {
        notifNew.classList.remove('visible');
    }
}

// BATH STATUS
// Update UI components for bath status.
function updateBathStatus() {
    if (!state.appConnected) {
        bathIndicator.className = indicatorClass('');
        valBath.textContent = '–';
        silenceBtn.classList.remove('visible');
        return;
    }
    const { label, cls } = BATH_STATUS[state.bathStatus];
    bathIndicator.className = indicatorClass(cls);
    valBath.textContent = label;

    // On initial hard warning, show silence button. Otherwise if clicked or not hard warning, hide button
    const showSilence = isWarningState(state.bathStatus) && !state.silenced;
    silenceBtn.classList.toggle('visible', showSilence);
}


/*  =====  LISTENER EVENTS & CORRESPONDING FUNCTIONS =====  */

// Navigation between Home and Notification -> Switch tab
document.querySelectorAll('.nav-btn').forEach(btn => {
    btn.addEventListener('click', () => {
        switchTab(btn.dataset.tab, btn);
    });
});

// Clear messages on notifications page
document.getElementById('clear-btn').addEventListener('click', () => {
    state.notifications = [];
    notifNew.classList.remove('visible');
    updateNotifications();
});

// BATH SIZE
// The next three listener events/functions handle the Bath Size element
// Increment/Decrement the literage when the +/- button is pressed.
// Increases/Decreases by SIZE_STEP, then refreshes UI
document.getElementById('btn-size-up').addEventListener('click', () => {
    if (state.bathSize < bath.MAX_SIZE) {
        state.bathSize += bath.SIZE_STEP; 
        updateSize(); 
    }
});
document.getElementById('btn-size-down').addEventListener('click', () => {
    if (state.bathSize > bath.MIN_SIZE) { 
        state.bathSize -= bath.SIZE_STEP; 
        updateSize(); 
    }
});
function updateSize() {
    sizeDisplay.textContent = state.bathSize;
}

// TEMPERATURE
// The next three listener events/functions handle the Bath Temperature adj element
// Increase/Decrease by 1. 
document.getElementById('btn-up').addEventListener('click', () => {
    if (state.targetTemp < bath.MAX_TEMP) {
        state.targetTemp++;
        updateTemp();
    }
});
document.getElementById('btn-down').addEventListener('click', () => {
    if (state.targetTemp > bath.MIN_TEMP) {
        state.targetTemp--;
        updateTemp();
    }
});
// If temperature is 41, text colour changes to warning. If 42 colours of text changes to red
function updateTemp(){
    tempDisplay.textContent = state.targetTemp;
    targetConfirm.textContent = `${state.targetTemp}°C`;
    tempDisplay.style.color = state.targetTemp >= 42 ? 'var(--danger)' :
                            state.targetTemp > 40  ? 'var(--warn)':'var(--accent)';
    if (state.running){
        if (mqttAdjustTemp(state.targetTemp))
            addNotification(`Adjusted target temperature to ${state.targetTemp}°C`);
        else
            commsError();
    }
}

// SILENCE WARNINGS
// Publishes a silence request to the firmware to stop the buzzer sounding
// The silence warning button only appears in HARD_WARNING or SENSOR_FAULT state
// Button disappears after it's clicked. 
silenceBtn.addEventListener('click', () => {
    if (mqttSilenceWarnings()) {
        state.silenced = true;
        silenceBtn.classList.remove('visible');
        addNotification('Warning buzzer silenced');
    } else {
        commsError();
    }
});

// START/STOP & ADD MORE WATER BUTTON 
// When the start button is clicked and state isn't active, flip to active
// else flip to inactive state. Calls uiUpdateStart/Stop providing a notification for UI
startBtn.addEventListener('click', () => {
    if (!isActive()) {
        requestStart(() => {
            mqttSetBathSize(state.bathSize);
            uiUpdateStart(`Commencing filling the tub. Target temp ${state.targetTemp}°C, size ${state.bathSize}L`);
        });
    } else {
        requestStop('User requested bath filling to stop');
    }
});
// Add More Water sets bath size effectively to unlimited. User needs to control start/stop.
addWaterBtn.addEventListener('click', () => {
    if (!isActive()) {
        requestStart(() => {
            mqttSetBathSize(9999);
            state.addMore = true;
            uiUpdateStart(`Adding more water to the bath.\nTarget temp ${state.targetTemp}°C`);
        });
    } else {
        requestStop('User stopped adding water');
    }
});
// Shared start/stop flow for the Start and Add-water buttons.
function requestStart(onPublished) {
    const start_pub = mqttStartBath(state.targetTemp);
    if (!start_pub) {
        commsError();
        return;
    }
    onPublished();
}
function requestStop(notif) {
    if (!mqttStopBath()) 
        commsError();
    uiUpdateStop(notif);
}
// Reset start button to idle (Start Bath) appearance.
function resetStartButton() {
    startBtn.className = STYLE.startBtn.idle;
    startBtn.textContent = 'Start Bath';
}
// Updates UI components to reflect the bath state for stop and start
// Configures the timeout for response from the firmware after click.
// On initial click, UI state flips to pending which keeps the start button pressed
// but doesn't yet update status components. If a response is not received from 
// the firmware state reverts to default settings and notification sent
function uiUpdateStart(notif){
    state.pendingStart = true;
    state.pendingNotif = notif;
    startBtn.className = STYLE.startBtn.running;
    startBtn.textContent = 'Stop Bath';

    // Revert if the firmware never confirms filling.
    clearTimeout(startTimeoutConf);
    startTimeoutConf = setTimeout(() => {
        if (!state.pendingStart) return;
        state.pendingStart = false;
        state.pendingNotif = null;
        state.addMore = false;
        resetStartButton();
        addNotification('No response from Bath, please check connection and try again');
    }, START_CONFIRM_TIMEOUT);
}
function uiUpdateStop(notif) {
    clearTimeout(startTimeoutConf);

    state.running = false;
    state.addMore = false;
    state.silenced = false;
    state.pendingStart = false;
    state.pendingNotif = null;
    state.bathStatus = 'IDLE';
    state.progress = 0;
    lastNotifiedState = 'IDLE';

    resetStartButton();
    addNotification(notif);
    updateBathStatus();
    showProgress();
}



// Render each once on load
updateBathStatus();
updateTemp();
showProgress();
updateNotifications();
mqttConnect({
    onConnect: () => setAppConnected(true),
    onDisconnect: () => setAppConnected(false),
    onMessage: routeTopics,
});