import * as mqttModule from './node_modules/mqtt/dist/mqtt.esm.js';
const { connect } = mqttModule.default;
import { HOST, PORT, USER, PASS, CLIENTID } from "./env_secrets.js";
import { setAppConnected } from "./app.js";

const TOPICS = {
  subscribe: {
    state:'smartbath/status/state',
    temp: 'smartbath/status/temp',
    progress:'smartbath/status/progress',
  },
  publish: {
    start:'smartbath/command/start',
    stop: 'smartbath/command/stop',
  }
};

let client = null;

export function mqttConnect(){
    const url = `wss://${HOST}:${PORT}/mqtt`;
    client = connect(url, {
        username: USER,
        password: PASS,
        clientId: CLIENTID,
        clean: true,
        reconnectPeriod: 3000,
        connectTimeout: 10000,
    });

    client.on('connect', () => {
        console.log("MQTT connected");
        setAppConnected(true);
        client.subscribe(Object.values(TOPICS.subscribe))
    });

    client.on('reconnect', () => {
        console.log("MQTT Attempting Reconnection");
        setAppConnected(false);
    });

    client.on('error', (err)=>{
        console.log(`MQTT Error: ${err.message}`);
        setAppConnected(false);
    });

    client.on('offline', () => {
        console.log("MQTT Offline");
        setAppConnected(false);
    });

    client.on('message', (topic, payload)=>{
        try{ 
            const data = JSON.parse(payload.toString());
            handlePayload(topic, data);
        } catch (err) {
            console.log('MQTT Error reading messaing', topic, payload.toString());
        }
    });
}

function handlePayload(topic, data){
    switch(topic){
        case TOPICS.subscribe.state:
            updateBathStatus(data.state); 
            state.running  = (data.state === 'FILLING' || data.state === 'SOFT_WARNING' || data.state === 'HARD_WARNING');
            break; 
        case TOPICS.subscribe.temp: 
            state.currentTemp = data.temp;
            currentTemp.textContent = data.temp.toFixed(1) + '°C';
            break; 
        case TOPICS.subscribe.progress:
            state.progress = data.progress; 
            showProgress(); 
            break;
    }
}

function mqttPublish(topic, payload){
    if (!client || !client.connected){
        console.log("MQTT Broker not connected");
        return false; 
    }
    client.publish(topic, JSON.stringify(payload), { qos: 1});
    return true; 
}

function mqttStartBath(targetTemp){
    return mqttPublish(TOPICS.publish.start, {target: targetTemp});
}
function mqttStopBath(){
    return mqttPublish(TOPICS.publish.stop, {});
}