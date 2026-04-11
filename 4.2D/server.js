import 'dotenv/config';
import express from 'express';
import { ArduinoIoTCloud } from 'arduino-iot-js';

const app = express();
const PORT = 3000;
app.use(express.json());
app.use(express.static('client'));

const { CLIENT_ID, CLIENT_SECRET, THING_ID } = process.env;

let connected = false;

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