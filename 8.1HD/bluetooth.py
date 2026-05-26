from bleak import BleakClient, BleakScanner
import asyncio

DEVICE_NAME = "Lindas Lighting Rig"
ARD_CHARACTERISTIC_UUID = "5fcc86b2-1b9a-4316-8845-98c6ec20a1bc" # Matches characteristic ID from Arduino Bluetooth connection 

client = None # global client to hold state for Bluetooth method
on_lost_connection = None # holds context for Bleak Client on disconnect 

'''
    Handles connecting to the set bluetooth device
    The Client registers a disconnect callback lost_connection() which is callable on disconnection 
    Returns success state of bluetooth connection 
'''
async def connect():
    global client
    device = await BleakScanner.find_device_by_name(DEVICE_NAME, timeout=10.0)
    
    if device is None: # set bluetooth device not found
        return False
    
    try:
        client = BleakClient(device.address, disconnected_callback=lost_connection, timeout=10.0)
        await client.connect()
        return client.is_connected
    except Exception as e:
        return False

'''
    Handles disconnect of the bluetooth. Resets client for new connection 
'''
async def disconnect():
    global client
    if client and client.is_connected:
        await client.disconnect()
    client = None

'''
    Sends bluetooth data. By creating a string of req as the room, val as the PWM value
    Calls Bleak write_gatt_char() and awaits write response after sending 
    Logs sending errors to terminal     
'''
async def send(req, val):
    if client and client.is_connected:
        try:
            request = f"{req}:{val}"
            await client.write_gatt_char(ARD_CHARACTERISTIC_UUID, request.encode(), response=True)
        except Exception as e: 
            print(f"Error sending {req}: {e}")

'''
    Set as a callback when initialising the bleak client
    When connection to the bluetooth drops, lost_connection retrieves the running loop.
    call_soon_threadsafe() ensures the loop executes in its own thread 
'''
def lost_connection(client):
    if on_lost_connection:
        asyncio.get_event_loop().call_soon_threadsafe(on_lost_connection) 