import lgpio

ROOM_PINS = {
    "Living Room": 17,
    "Bathroom": 27,
    "Closet": 22
}

chip = lgpio.gpiochip_open(0)

# set each LED to their respective GPIO pin
for pin in ROOM_PINS.values():
    lgpio.gpio_claim_output(chip, pin)
    lgpio.gpio_write(chip, pin, 0)

# handles turning on/off the pin. Power argument is a boolean
# if True lights on, false lights off
def toggle_pin(room, power):
    pin = ROOM_PINS.get(room)
    if pin is not None:
        lgpio.gpio_write(chip, pin, 1 if power else 0)

# when app exits, all LEDs are switched off and chip gets closed
def cleanup():
    for pin in ROOM_PINS.values():
        lgpio.gpio_write(chip, pin, 0)
    lgpio.gpiochip_close(chip)