from gpiozero import PWMOutputDevice

rooms = {
    "Living Room": 12,
    "Bathroom": 13,
    "Closet": 19
}

# assigns each value in rooms to it's respective GPIO PWM enabled pin
leds = {room: PWMOutputDevice(pin) for room, pin in rooms.items()}

# power toggle for LEDs
def toggle(room, is_on):
    leds[room].value = 1 if is_on else 0

# uses PWM module .value input to set Duty Cycle of LED
def dimmer(room, value):
    leds[room].value = value * 0.01

def cleanup():
    for led in leds.values():
        led.off()