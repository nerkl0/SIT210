from bleak import BleakClient, BleakScanner
import asyncio

DEVICE_NAME = "Lindas Lighting Rig"
ARD_CHARACTERISTIC_UUID = "5fcc86b2-1b9a-4316-8845-98c6ec20a1bc"

client = None
on_lost_connection = None

async def connect():
    global client

    print(f"Scanning for {DEVICE_NAME}...")
    device = await BleakScanner.find_device_by_name(DEVICE_NAME, timeout=10.0)
    
    if device is None:
        print("Device not found")
        return False
    
    try:
        client = BleakClient(device.address, disconnected_callback=lost_connection, timeout=10.0)
        await client.connect()
        print("Bluetooth connected successfully")
        return client.is_connected
    except Exception as e:
        print(f"Connection failed: {e}")
        return False

async def disconnect():
    global client
    if client and client.is_connected:
        await client.disconnect()

    print("Bluetooth disconnected")
    client = None

async def send(req, val):
    if client and client.is_connected:
        try:
            request = f"{req}:{val}"
            await client.write_gatt_char(ARD_CHARACTERISTIC_UUID, request.encode(), response=True)
            print(f"\n\nBluetooth request sent: {request}\n\n")
        except Exception as e: 
            print(f"\n\nError sending {req}: {e}\n\n")


def lost_connection(client):
    if on_lost_connection:
        asyncio.get_event_loop().call_soon_threadsafe(on_lost_connection)
   