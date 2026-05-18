from vosk import Model, KaldiRecognizer
import sounddevice as sd
import json, asyncio

model = Model("VoskModel")

PWM_STEPPER = 20

ON_WORDS = ["on", "lights", "max"]
OFF_WORDS = ["off", "dark"]
BRIGHTER_WORDS = ["brighter", "higher", "up", "increase"]
DIMMER_WORDS = ["dimmer", "lower", "down", "decrease", "dim"]
ROOM_WORDS = {
    "bathroom": ["bathroom", "bath"],
    "closet": ["closet", "wardrobe"],
    "living room": ["living room", "living", "main"],
    "fan": ["fan", "exhaust"]
}

def audio_converter(text):
    text = text.lower()
    action = None

    if any(word in text for word in ON_WORDS):
        action = 255
    elif any(word in text for word in OFF_WORDS):
        action = 0
    elif any(word in text for word in BRIGHTER_WORDS):
        action = f"+{PWM_STEPPER}"
    elif any(word in text for word in DIMMER_WORDS):
        action = f"-{PWM_STEPPER}"

    for room_name, room in ROOM_WORDS.items():
        if any(n in text for n in room):
            return action, room_name
        
    return action, None

async def run_mic(voice_command):
    loop = asyncio.get_event_loop()
    while True:
        text = await loop.run_in_executor(None, listen)
        if text:
            action, room = audio_converter(text)
            print(f"Heard: '{text}'\nRoom: {room}, Action: {action}")
            if action is not None and room:
                asyncio.ensure_future(voice_command(action, room))
            else:
                print("Could not parse command.")

def listen():
    recogniser = KaldiRecognizer(model, 16000)
    print("Listening...")
    with sd.RawInputStream(samplerate=16000, blocksize=8000, dtype='int16', channels=1) as stream:
        while True:
            data, _ = stream.read(8000)
            if recogniser.AcceptWaveform(bytes(data)):
                result = json.loads(recogniser.Result())
                text = result.get("text", "")
                if text:
                    print(f"Recognised: '{text}'")
                    return text