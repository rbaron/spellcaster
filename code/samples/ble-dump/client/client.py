import argparse
import asyncio
import logging
import pathlib
from enum import Enum

from bleak import BleakClient, BleakScanner
from bleak.backends.characteristic import BleakGATTCharacteristic

MIN_PACKET_LEN = 52

HEADER = "ax,ay,az,gx,gy,gz,name,n"

logger = logging.getLogger(__name__)

current_packet = bytearray()


# Enum class state.
class State(Enum):
    WAITING_FOR_HEADER = 0
    WAITING_FOR_DATA = 1
    PROCESSING_DATA = 1


state = State.WAITING_FOR_HEADER
n_bytes = 0
n = 0

file = None


def process_packet(packet: bytearray):
    global state
    global n_bytes
    global n

    assert state == State.PROCESSING_DATA
    # 1 second of data.
    assert n_bytes > MIN_PACKET_LEN

    # Each row contains 6 entries of 2 bytes each.
    # Each entry is a 16-bit signed integer.

    n_rows = n_bytes // (2 * 6)
    print(f"n_rows: {n_rows}")
    for i in range(n_rows):
        for j in range(6):
            pos = i * 12 + j * 2
            entry = packet[pos : pos + 2]
            value = int.from_bytes(entry, byteorder="little", signed=True)
            file.write(f"{value},")
        file.write(f"{args.gesture_name},{n}\n")

    file.flush()

    current_packet.clear()
    n += 1
    state = State.WAITING_FOR_HEADER


def notification_handler(characteristic: BleakGATTCharacteristic, data: bytearray):
    global state
    global n_bytes
    """Simple notification handler which prints the data received."""
    if state == State.WAITING_FOR_HEADER:
        assert data[0] == 0xFF
        n_bytes = data[1] << 8 | data[2]
        print(f"Expecting {n_bytes} bytes")
        state = State.WAITING_FOR_DATA
    elif state == State.WAITING_FOR_DATA:
        current_packet.extend(data)
        if len(current_packet) == n_bytes:
            print("Received packet")
            state = State.PROCESSING_DATA
            process_packet(current_packet)


async def main(args: argparse.Namespace):
    logger.info("starting scan...")

    if args.address:
        device = await BleakScanner.find_device_by_address(
            args.address, cb=dict(use_bdaddr=args.macos_use_bdaddr)
        )
        if device is None:
            logger.error("could not find device with address '%s'", args.address)
            return
    else:
        device = await BleakScanner.find_device_by_name(
            args.device_name, cb=dict(use_bdaddr=args.macos_use_bdaddr)
        )
        if device is None:
            logger.error("could not find device with name '%s'", args.name)
            return

    logger.info("connecting to device...")

    async with BleakClient(device) as client:
        logger.info("Connected")

        await client.start_notify(args.characteristic, notification_handler)
        while True:
            await asyncio.sleep(5.0)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()

    device_group = parser.add_mutually_exclusive_group(required=True)

    device_group.add_argument(
        "--device-name",
        metavar="<device_name>",
        help="the name of the bluetooth device to connect to",
    )
    device_group.add_argument(
        "--address",
        metavar="<address>",
        help="the address of the bluetooth device to connect to",
    )

    parser.add_argument(
        "--macos-use-bdaddr",
        action="store_true",
        help="when true use Bluetooth address instead of UUID on macOS",
    )

    parser.add_argument(
        "--characteristic",
        metavar="<notify uuid>",
        help="UUID of a characteristic that supports notifications",
        default="6E400003-B5A3-F393-E0A9-E50E24DCCA9E",
    )

    parser.add_argument(
        "--filepath", type=pathlib.Path, help="path to file to write to", required=True
    )

    parser.add_argument("--gesture-name", help="Gesture name", required=True)

    parser.add_argument(
        "-d",
        "--debug",
        action="store_true",
        help="sets the log level to debug",
    )

    args = parser.parse_args()

    log_level = logging.DEBUG if args.debug else logging.INFO
    logging.basicConfig(
        level=log_level,
        format="%(asctime)-15s %(name)-8s %(levelname)s: %(message)s",
    )

    # if args.filepath.exists():
    #     print(f"File {args.filepath} already exists")
    #     exit(1)

    file = args.filepath.open("w")
    file.write(HEADER + "\n")

    asyncio.run(main(args))
