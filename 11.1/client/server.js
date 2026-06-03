import * as mqttModule from './node_modules/mqtt/dist/mqtt.esm.js';
const { connect } = mqttModule.default;
import { HOST, PORT, USER, PASS, CLIENTID } from "./env_secrets.js";

export const TOPICS = {
  subscribe: {
    status:'smartbath/status',
    connected:'smartbath/status/connected'
  },
  publish: {
    start:'smartbath/command/start',
    stop:'smartbath/command/stop',
    temp:'smartbath/command/temp',
    size:'smartbath/command/size',
    silence:'smartbath/command/silence'
  }
};

let client = null;

export function mqttConnect(handlers = {}){
    const { onConnect, onDisconnect, onMessage } = handlers;
    const url = `wss://${HOST}:${PORT}/mqtt`;
    client = connect(url, {
        username: USER,
        password: PASS,
        clientId: `${CLIENTID}-${Math.random().toString(16).slice(2, 8)}`,
        clean: true,
        reconnectPeriod: 3000,
        connectTimeout: 10000,
    });

    client.on('connect', () => {
        console.log("MQTT connected");
        client.subscribe(Object.values(TOPICS.subscribe));
        onConnect?.();
    });

    client.on('reconnect', () => console.log("MQTT Attempting Reconnection"));

    client.on('error', (err) => {
        console.log(`MQTT Error: ${err.message}`);
        onDisconnect?.();
    });

    client.on('offline', () => {
        console.log("MQTT Offline");
        onDisconnect?.();
    });

    client.on('message', (topic, payload) => {
        try {
            onMessage?.(topic, payload.toString());
        } catch (err) {
            console.log('MQTT Error reading message', topic, payload.toString());
            console.log(err);
        }
    });
}

// Main publish function that handles JSON conversion and publishing to MQTT
function mqttPublish(topic, payload){
    if (!client || !client.connected){
        console.log("MQTT Broker not connected");
        return false;
    }
    client.publish(topic, JSON.stringify(payload), { qos: 1 });
    return true;
}

// Different types of publish functions 
export function mqttStartBath(targetTemp){ 
    return mqttPublish(TOPICS.publish.start, targetTemp); 
}
export function mqttStopBath(){ 
    return mqttPublish(TOPICS.publish.stop, {}); 
}
export function mqttAdjustTemp(adjTemp){ 
    return mqttPublish(TOPICS.publish.temp, adjTemp); 
}
export function mqttSetBathSize(litres){ 
    return mqttPublish(TOPICS.publish.size, litres); 
}
export function mqttSilenceWarnings(){
    return mqttPublish(TOPICS.publish.silence, {});
}