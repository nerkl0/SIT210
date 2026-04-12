import 'dotenv/config';
import express from 'express';
import { ArduinoIoTCloud } from 'arduino-iot-js';

const app = express();
const PORT = 3000;
app.use(express.json());
app.use(express.static('client'));

const { CLIENT_ID, CLIENT_SECRET, THING_ID } = process.env;

let connected = false;

// Connection handler to Arduino cloud
async function connectToArduino() {
    await ArduinoIoTCloud.connect({
        clientId: CLIENT_ID,
        clientSecret: CLIENT_SECRET,
        onDisconnect: (message) => {
            console.error('Disconnected:', message);
            connected = false;
        }
    });
    connected = true;
    console.log('Connected to Arduino IoT Cloud');
}

connectToArduino();

// API Endpoint for individual Thing handling. Passes name of Thing as argument to Cloud sendProperty(), along with the state udpate
app.post('/api/rooms/:name', async (req, res) => {
    const { name } = req.params;
    const { value } = req.body;
    try {
        await ArduinoIoTCloud.sendProperty(THING_ID, name, value);
        res.json({ ok: true, name, value });
    } catch (e) {
        res.status(500).json({ error: e.message });
    }
});

// API Endpoint for all. Changes the value parameter for sendProperty based on the payload the endpont receives:
// if argument true publish AllOn else publish AllOff
app.post('/api/all', async (req, res) => {
    const { value } = req.body;
    try {
        await ArduinoIoTCloud.sendProperty(THING_ID, value ? 'AllOn' : 'AllOff', true);
        res.json({ ok: true });
    } catch (e) {
        res.status(500).json({ error: e.message });
    }
});

app.listen(PORT, () => console.log(`Server running at ${PORT}`));