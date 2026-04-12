const rooms = [
    { name: 'Living Room', thingName: 'LivingRoom', on: false },
    { name: 'Bathroom', thingName: 'Bathroom', on: false },
    { name: 'Closet', thingName: 'Closet', on: false }
];


// Dynamic rendering of room cards. Maps each object within the rooms array to a respective card
// Assigns event handler when clicked which calls toggleLight function
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

// Toggle light receives the index of the object within the rooms array
// Calls the POST API /rooms/:name and sends the state to update the value to
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

// Value is a boolean based on whether the lights should be turned on or off.
// First updates the UI on / off, calling showContent() to re-render page.
// Then calls /all endpoint passing the boolean value as the body
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