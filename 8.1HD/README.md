# Voice Activated Lighting System

## Description
This application allows a user to control the power of lights and fans via a user friendly application or via Voice Command.

## Contents
1. [Dependencies](#dependencies)
2. [Hardware](#hardware)
3. [Installation](#installation)
4. [Configure Microphone](#configure-microphone)
5. [Running the Program](#running-the-program)
6. [Usage](#usage)
7. [Known Limitations](#known-limitations)
8. [Troubleshooting](#troubleshooting)

## Dependencies 
**RapsberryPi** 
- `PyQt6==6.11.0`
- `qasync==0.28.0`
- `bleak==3.0.2`
- `vosk==0.3.45`
- `sounddevice==0.5.5`

**Arduino** 
- `ArduinoBLE`
- `BH1750`

## Hardware
Has been designed to work optimally with the Raspberry Pi 5 and Arduino Nano 33 IoT. A wire diagram of the setup can be found [here](will_include_link) 

## Installation
**UI Application (Raspberry Pi)**
1. [Download](https://www.python.org/downloads/) `python >= 3.0.0` and install
2. Make sure pip is installed 
3. Set up virtual environment & install dependencies
    ```
    python -m venv [env name]
    
    [For Linux/Mac]
    source venv/bin/activate
    
    [For Windows]
    venv\Scripts\activate

    pip install -r requirements.txt
    ```
**Backend (Arduino)**
1. If you have the Arduino CLI run the below, otherwise navigate to library tab in ArduinoIde and search `ArduinoBLE`, `BH1750` 
 ```
 arduino-cli lib install "ArduinoBLE"
 arduino-cli lib install "BH1750"
 ```
2. Install program onto the Arduino

## Configure Microphone
1. Download and extract a [Vosk model](https://alphacephei.com/vosk/models)
    - vosk-model-small-en-us-0.15 is sufficient for the current words used in this application
2. Place the model within the root folder of the application
3. In `MicController.py` update Model argument with the file path to model
    - `model = Model("Path/To/Model")`


> **Note:** If you have multiple input devices, you will need to find the index of the microphone you wish to use. Run the snippet below to retrieve the index, then update the `channel` parameter of `sd.RawInputStream()` on line 73 in `MicController.py`
> ```python
> import sounddevice as sd
> for i, dev in enumerate(sd.query_devices()):
>     if dev['max_input_channels'] > 0:
>         print(i, dev['name'])
> ```

## Running the Program
1. Ensure the Arduino is powered and running
2. Activate your virtual environment (see Installation)
3. Run the application
    ```
        python main.py
    ```

## Usage

### GUI
The application window displays each room and the fan. Each room has a toggle button to switch the light on or off. The Living Room light has a slider to adjust brightness.

### Voice Commands
Voice commands require a recognised **action word** and a **room word** spoken together.

**Rooms**

| Room | Accepted Words |
|------|---------------|
| Living Room | "living room", "living", "main" |
| Bathroom | "bathroom", "bath" |
| Closet | "closet", "wardrobe" |
| Fan | "fan", "exhaust" |

**Actions**

| Action | Accepted Words |
|--------|---------------|
| Turn on (full brightness) | "on", "lights", "max" |
| Turn off | "off", "dark" |
| Brighter | "brighter", "higher", "up", "increase" |
| Dimmer | "dimmer", "lower", "down", "decrease", "dim" |

**Examples**
- *"bathroom on"* — turns the bathroom light on at full brightness
- *"living room dimmer"* — decreases the living room light brightness
- *"fan on"* — activates the exhaust fan
- *"wardrobe off"* — turns the closet light off

## Known Limitations
- Currently only configured to support microphones with USB input
- Fan can only be turned on or off. Modulating it's speed is not currently possible
- If application is closed while the lights or fan is on, each must be turned off manually
- Currently no support for "All Off" or "All On" voice command 

## Troubleshooting
**Microphone can become unresponsive after reconnecting**  
Occasionally after hitting the reconnect button, the microphone may stop responding to voice commands. If this occurs, close and reopen the application.