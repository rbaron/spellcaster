[![spellcaster Demo](https://img.youtube.com/vi/c5Yf7bW8n6s/maxresdefault.jpg)](https://youtu.be/c5Yf7bW8n6s)

spellcaster is a home automation magic wand. Click on the thumbnail above to watch it in action.

# How does it work?
An accelerometer and gyroscope are used to capture gestures ("spells"). Cast spells are then compared against previously stored ones. An action is triggered if there's a good match.

Different actions can be triggered. For example:
- A [BTHome.io](https://bthome.io) Bluetooth Low Energy (BLE) packet can be emitted and handled in [Home Assistant](https://www.home-assistant.io/).
- A Zigbee Cluster action, similarly handled in Home Assistant automations
- A [BLE-HID](https://novelbits.io/bluetooth-hid-devices-an-intro/) to simulate a key press on a computer, macro pad-style

In the "Software" section below, there are some firmware samples for each of these actions and more.

[This blog post](https://rbaron.net/blog/2023/10/27/spellcaster-a-home-automation-magic-wand.html) has a deeper dive into how everything works.

# Project status
It sort of works, but there are caveats. Reliability and usability need improvement -- it's easy to [hold it wrong](https://www.wired.com/2010/06/iphone-4-holding-it-wrong/). See "User interface" below. The code also needs the proverbial cleaning up.

Approach this project as a fun experiment. Expect fun and experimental support.

# Hardware
![PCB](https://github.com/rbaron/spellcaster/assets/1573409/da951f41-2f28-43aa-8d76-5cb069a4d24b)

* [nRF52833](https://www.nordicsemi.com/Products/nRF52833) MCU
* [LSM6DSL](https://www.st.com/en/mems-and-sensors/lsm6dsl.html) IMU
* [Mini vibration motor](https://www.aliexpress.com/item/1005004653448729.html) for tactile feedback
* Powered by 2xAAA batteries

The [`kicad/`](./kicad/) directory contains the design and fabrication files. I used [JLCPCB](https://jlcpcb.com/) for manufacturing and assembly.

# User interface
## Casting spells
There is no on/off switch by design. To signal the *start of spell*, hold the wand horizontally flat. Once you start moving it, the gesture will start to be collected. To signal the *end of spell*, hold the wand horizontally flat again. Spells need to be between 500 milliseconds and 3 seconds in length.

## Record mode
To store a new spell in slot `S`, press the `A` button `S` times and cast a spell as described above. If everything goes well, the LED will flash five times and you will be switched to replay mode.

In this video you can see five different spells being recorded:

[![spellcaster recording spells](https://img.youtube.com/vi/6D_qe5v8ILQ/maxres1.jpg)](https://youtu.be/6D_qe5v8ILQ)

## Replay mode
This is the default mode in which spellcaster will live most of its life. Once a spell is cast, it will be compared to previously stored spells. If a good match is found, a vibration pattern is executed and an action will be triggered.

### On false positives
The matching algorithm is optimized for _something to happen_. There is a low threshold for matching spells, and often two or more will be sufficiently matched. This depends on how similar spells are, but the best matching spell will always be selected. The idea is that, when we intentionally cast a spell, the correct one will almost always be selected, instead of -- sometimes -- none. On the other hand, previously unseen spells may trigger a false positive.

This is done to mainly prevent the sorry sight of a full grown adult waving a wand in the air in front of friends and family at a dinner party and nothing happening. Don't ask. A better solution is to further tune the spell matching algorithm to be more robust against these cases, while retaining the precision when the casting is intentional. Another idea is to add an invisible capacitive touch sensor to the handle, which would also allow for much deeper sleep modes. Something to be improved.

## Deep sleep mode
If spellcaster is still for 10 seconds, it will enter the lowest power mode at around 65 uA. Most of which goes to the accelerometer, which lies semi-awake in a low power state waiting for movement. This can mostly likely be optimized further. Any movement will wake it up and into replay mode.

# Software
There are a few sample firmwares in the repository. They are all written with Nordic's [nRF Connect SDK](https://www.nordicsemi.com/Products/Development-software/nRF-Connect-SDK). Most samples are adapted from the SDK's examples.

| Directory | Description |
| --- | --- |
|[`code/sclib`](./code/sclib/)| Generic spellcaster library. Handles spell casting, storing and signal processing. Used by all other samples.|
|[`code/samples/ble-bthome`](./code/samples/ble-bthome/)|Matched spell `S` trigger a [BTHome.io](https://bthome.io) action that is equivalent to `S` button presses. This can then be used by [Home Assistant](https://homeassistant.io) automations. |
|[`code/samples/ble-hid`](./code/samples/ble-hid/)|spellcaster pairs with your computer via BLE. Matched spell `S` trigger a BLE-HID action that simulates a key press following the configurable [keymap](https://github.com/rbaron/spellcaster/blob/fc7ee0511366e4b62a08713fc59aa67fac9f286a/code/samples/ble-hid/src/main.c#L16).|
|[`code/samples/zigbee`](./code/samples/zigbee/)|Matched spells trigger Zigbee Cluster actions that can be used by [Home Assistant](https://homeassistant.io) automations. Unfortunately, the Zigbee specs don't define magic wand clusters, so the [spellcaster.py ZHA Quirk](https://github.com/rbaron/spellcaster/blob/main/code/samples/zigbee/zha/quirks/spellcaster.py) needs to be installed. With it, matched spell on slot `S` appears to be a "dim light" command with step `S`. Yikes! (And it's not even the worst hack around here). |
|[`code/samples/ble-dump`](./code/samples/ble-dump/)|Dumps raw accel & gyro spells over BLE. The [client.py](https://github.com/rbaron/spellcaster/blob/main/code/samples/ble-dump/client/client.py) connects to spellcaster and dumps the data to a file. This is how I collected the data for tuning the spell matching without cables getting in the way.|

## Building the samples
These are standard nRF Connect SDK projects with Zephyr RTOS, and the [official docs](https://nrfconnect.github.io/vscode-nrf-connect/index.html) apply. The [b-parasite project](https://github.com/rbaron/b-parasite) uses a similar setup with a generic library and different samples, and [the Wiki](https://github.com/rbaron/b-parasite/wiki/How-to-Build-the-Firmware-Samples) contains more details and alternatives.

## Flashing the samples
The samples are flashed with ARM's usual [SWD](https://developer.arm.com/documentation/ihi0031/a/The-Serial-Wire-Debug-Port--SW-DP-/Introduction-to-the-ARM-Serial-Wire-Debug--SWD--protocol). The [official docs](https://nrfconnect.github.io/vscode-nrf-connect/get_started/quick_debug.html) also apply. The only caveat is the custom, slim 6-testpoint SWD pins on the PCB. It also happens to the the exact same as described in the [b-parasite Wiki](https://github.com/rbaron/b-parasite/wiki/How-to-Flash-the-Firmware-Samples).

# Data analysis
The [`data/`](./data/) directory contains a bunch of Jupyter notebooks I used to tune the spell matching and some of the raw collected data.

# 3D printed case
![fusion-case](https://github.com/rbaron/spellcaster/assets/1573409/e60e9bde-11f5-4689-91da-1f59e97c9445)

The [`3dprint/`](./3dprint/) directory contains the STEP files for a 3D printed case. It's okay-ish, but could use some love. The 3D printed springs lose some tension after a while, and so do the clamps that hold both halves together. On the bright side, there's no glue nor screws involved. You can watch a video of the full assembly process [here](https://www.youtube.com/watch?v=kVlnX7w-8rc).


# License
The hardware and associated design files are released under the [Creative Commons CC BY-SA 4.0](https://creativecommons.org/licenses/by-sa/4.0/) license. The code is released under the [MIT license](https://opensource.org/licenses/MIT).
