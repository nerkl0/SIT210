from gpiozero import PWMOutputDevice

rooms = {
    "Living Room": 12,
    "Bathroom": 13,
    "Closet": 19
}

leds = {room: PWMOutputDevice(pin) for room, pin in rooms.items()}

def toggle(room, is_on):
    leds[room].value = 1 if is_on else 0

def dimmer(room, value):
    leds[room].value = value * 0.01

def cleanup():
    for led in leds.values():
        led.off()