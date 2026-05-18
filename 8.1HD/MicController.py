from vosk import Model, KaldiRecognizer
import sounddevice as sd
import json, asyncio

'''
    Vosk model used for word recognition. Must download externally and store locally
    vosk-model-small-en-us-0.1.5 is enough to cover the below set words
    The argument passed in to Model is the path to Vosk model folder 
'''
model = Model("VoskModel")

PWM_INCREMENT = 20 # set increment for brighter / dimmer words


# Each of the following arrays hold alternate descriptors of the action / room words
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

'''
    Text processed from microphone is passed to identify if any of the words are recognised 
    If an ON_WORD, will set the action value to the maximum PWM value. OFF_WORDs sets to 0 - off
    If brighter or dimmer word, assigns relevant +/- PWM_INCREMENT value
    Returns room name and action if found, otherwise None, None
'''
def audio_converter(text):
    text = text.lower()
    action = None

    if any(word in text for word in ON_WORDS):
        action = 255
    elif any(word in text for word in OFF_WORDS):
        action = 0
    elif any(word in text for word in BRIGHTER_WORDS):
        action = PWM_INCREMENT
    elif any(word in text for word in DIMMER_WORDS):
        action = -PWM_INCREMENT

    for room_name, room in ROOM_WORDS.items():
        if any(n in text for n in room):
            return action, room_name
        
    return None, None

'''
    asynchronous function that takes an instance of a function from the GUI. 
    Retrieves the running event loop from main thread so that listen() can be called as a non-blocking function 
    Parses heard audio to audio_converter() to confirm if it's a recognised word. If so, 
    invokes voice_command to return to main thread with action, room arguments 
'''
async def run_mic(voice_command):
    loop = asyncio.get_event_loop()
    while True:
        text = await loop.run_in_executor(None, listen)
        if text:
            action, room = audio_converter(text)
            if action is not None and room:
                asyncio.ensure_future(voice_command(action, room))
            else:
                print("Could not parse command.")
'''
   Listens continuously to microphone, when it detects sound, converts to text
   Is a blocking function and why run_in_executor() is required in run_mic()   
'''
def listen():
    recogniser = KaldiRecognizer(model, 16000)
    with sd.RawInputStream(samplerate=16000, blocksize=8000, dtype='int16', channels=1) as stream:
        while True:
            data, _ = stream.read(8000)
            if recogniser.AcceptWaveform(bytes(data)):
                result = json.loads(recogniser.Result())
                text = result.get("text", "")
                if text:
                    return text