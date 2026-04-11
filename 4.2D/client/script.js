const rooms = [
    { name: 'Living Room', thingName: 'LivingRoom', on: false },
    { name: 'Bathroom', thingName: 'Bathroom', on: false },
    { name: 'Closet', thingName: 'Closet', on: false }
];


function showContent() {
    document.getElementById('rooms').innerHTML = rooms.map((r, i) => `
      <div class="card ${r.on ? 'on' : ''}" onclick="toggleLight(${i})">
        <div class="card-inner">
            <div class="led ${r.on ? 'on' : ''}"></div>
            <div class="room">${r.name}</div>
            <div class="status">${r.on ? 'On' : 'Off'}</div>
        </div>
      </div>
    `).join('');
}

async function toggleLight(i) {
    rooms[i].on = !rooms[i].on;
    showContent();
    try {
        await fetch(`/api/rooms/${rooms[i].thingName}`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ value: rooms[i].on })
        });
    } catch (e) {
        console.warn('Failed to update:', e);
    }
}

async function setAll(value) {
    rooms.forEach(r => r.on = value);
    showContent();
    await fetch('/api/all', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ value })
    });
}

showContent();