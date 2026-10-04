# ESP32-S3 Two-Button LED Toggle

A minimal ESP-IDF project for the ESP32-S3: pressing either of two push-buttons toggles an LED. Both buttons are handled with GPIO interrupts that share a single interrupt service routine (ISR) and a software debounce, so no polling loop is needed.

## Features

- Interrupt-driven handling of two buttons (falling edge on GPIO4 and GPIO6)
- One ISR shared by both buttons
- Software debounce (200 ms) inside the ISR
- LED on GPIO5, starting in the OFF state
- Two different pull-up approaches, as a practical example:
  - Button 1: external 10 kΩ pull-up resistor
  - Button 2: internal pull-up of the ESP32-S3, no external resistor

## Hardware

- ESP32-S3 development board (power and ground are taken from the board)
- 2 × push-button (normally open)
- 1 × red LED
- 1 × 10 kΩ resistor (pull-up for button 1)
- 1 × 330 Ω resistor (LED current limiting)
- Breadboard and jumper wires

## Circuit

### Schematic

```
Button 1 (active low, external pull-up)

  3V3 ---[ R1 10k ]---+---- GPIO4
                      |
                   [ SW1 ]
                      |
  GND ----------------+

Button 2 (active low, internal pull-up)

  GPIO6 ---- [ SW2 ] ---- GND

Output (LED, active high)

  GPIO5 ---[ R2 330 ]---(+) LED1 (-)--- GND
                         red LED
```

### Description

- **Button 1:** the 3.3 V rail goes through R1 (10 kΩ) to a node. One side of the node goes to GPIO4, the other side goes to the push-button. The other terminal of the button is connected to ground.
- **Button 2:** GPIO6 is connected to one terminal of the push-button and the other terminal goes directly to ground. There is no resistor and **no connection to 3.3 V**: the pull-up is provided by the ESP32-S3 itself and enabled in firmware.
- **LED:** GPIO5 goes through R2 (330 Ω) to the anode (positive pin) of the red LED. The cathode (negative pin) is connected to ground.

### Pin usage

| GPIO | Direction | Function | Notes                                                                                         |
|------|-----------|----------|-----------------------------------------------------------------------------------------------|
| 4    | Input     | Button 1 | Active low: 1 when released, 0 when pressed. External pull-up (R1). Interrupt on falling edge. |
| 6    | Input     | Button 2 | Active low: 1 when released, 0 when pressed. Internal pull-up. Interrupt on falling edge.      |
| 5    | Output    | LED      | Active high: 1 = LED on.                                                                      |

### Components

| Ref.  | Component                | Value               |
|-------|--------------------------|---------------------|
| R1    | Resistor (pull-up, SW1)  | 10 kΩ               |
| R2    | Resistor (LED limiting)  | 330 Ω               |
| LED1  | LED                      | Red                 |
| SW1   | Push-button              | Normally open       |
| SW2   | Push-button              | Normally open       |

With a typical red LED forward voltage of about 1.8–2 V, the LED current is roughly 4 mA, well within the GPIO limits.

## How it works

1. At rest, GPIO4 is held at 3.3 V by R1 and GPIO6 is held at 3.3 V by the internal pull-up (logic high).
2. Pressing SW1 or SW2 connects the corresponding GPIO to ground (logic low), which triggers a falling-edge interrupt.
3. Both interrupts call the same ISR, which checks that at least 200 ms have passed since the last accepted press (debounce).
4. If so, the LED state is inverted and written to GPIO5.

Each press of either button therefore toggles the LED: on, then off, then on again. `app_main()` only configures the pins and returns; from then on everything is handled by interrupts.

## Firmware

Written in C for ESP-IDF v6.1, target `esp32s3`.

| Function               | Purpose                                                                                                     |
|------------------------|-------------------------------------------------------------------------------------------------------------|
| `configure_led()`      | Configures GPIO5 as output and turns the LED off at startup.                                                |
| `configure_buttons()`  | Configures GPIO4 (no internal pulls) and GPIO6 (internal pull-up enabled) as inputs with a falling-edge interrupt, installs the ISR service and registers the same ISR on both pins. |
| `button_isr_handler()` | ISR shared by both buttons: applies the 200 ms debounce and calls `manage_led()`. The pin number is passed as argument but not used, since both buttons do the same thing. |
| `manage_led()`         | Inverts the LED state and writes it to GPIO5.                                                               |
| `app_main()`           | Calls the two configuration functions.                                                                      |

Both `button_isr_handler()` and `manage_led()` are placed in IRAM (`IRAM_ATTR`) because they run in interrupt context, and the variables shared with the ISR are declared `volatile`.

`gpio_install_isr_service()` is called only once; each button is then registered with its own `gpio_isr_handler_add()` call.

## Build and flash

Requirements: [ESP-IDF](https://docs.espressif.com/projects/esp-idf/) v6.1 installed and configured (for example through the ESP-IDF extension for VS Code).

```
idf.py set-target esp32s3
idf.py build
idf.py -p <PORT> flash monitor
```

Replace `<PORT>` with your serial port (for example `COM3` on Windows or `/dev/ttyUSB0` on Linux).

## Project structure

```
.
├── CMakeLists.txt
├── main/
│   ├── CMakeLists.txt
│   └── main.c
├── docs/            (photos, schematic images)
└── README.md
```

## Troubleshooting

- **Board resets or the serial port disappears when a button is pressed.** A button must only connect its GPIO node to ground, with a pull-up (external resistor or internal) providing the high level. If 3.3 V is connected directly to the button (no resistor), pressing it short-circuits the power rail and the board loses power. For SW2 in particular, make sure there is no wire from 3.3 V to GPIO6.
- **CMake fails while configuring the project (`kconfgen` / `FileNotFoundError`).** Keep the project in a short path without spaces or accented characters, and outside synced folders such as OneDrive. Long paths and characters like "à" can break the ESP-IDF build scripts on Windows.
- **LED turns on and off more than once per press.** The debounce time is too short for your button. Increase `DEBOUNCE_US` in `main.c`.
- **Flash size warning at boot.** If the board has more flash than the project is configured for, set the correct size in `idf.py menuconfig` → *Serial flasher config* → *Flash size*.

## License

This project is released under the MIT License.

```
MIT License

Copyright (c) 2026 Marco Agnoli

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```