# Kerststal 2025

## 🎄 Fijne feestdagen! 🎄

Namens ons allemaal, fijne feestdagen!
We hopen dat jij het kijken en luisteren net zo leuk vindt als wij het bouwen vonden. Probeer vooral alle knoppen, en als je minder tijd hebt, DE RODE!

Welkom bij het Kerststal 2025 project!  
Deze kerststal is een experimenteel, creatief en vooral gezellig project. Veel plezier met bouwen en beleven!

---

## Introduction

This project controls an interactive nativity scene using an ESP32.  
You can start different scenes with buttons or via a web interface. Each scene combines motors, lights, and music for a festive experience.

---

## Webserver

- The ESP32 runs a built-in webserver.
- You can access the web interface from any device on the same network by browsing to the ESP32's IP address.
- The web interface allows you to:
  - Start scenes remotely
  - Stop the current scene
  - View the current status and play counts in real time

---

## MQTT

- The project supports MQTT for integration with home automation or remote monitoring.
- The ESP32 publishes scene activity and play counts to a configurable MQTT broker.
- You can subscribe to these topics to monitor which scene is active or how often each scene has been played.
- MQTT credentials and broker address are configured in the project settings.

---

## Buttons

- Each button starts a specific scene (e.g., Zakske, Beuk, Herdertjes).
- Press a button to activate its scene.
- The LED next to the button lights up when the scene is active.
- Buttons are connected to the ESP32 with pull-up resistors.

**Example pinout:**

| Button | GPIO | LED |
| ------ | ---- | --- |
| 1      | 32   | 25  |
| 2      | 33   | 26  |
| 3      | 34   | 27  |

---

## Motors

- Multiple motors are used (for example, for the tree and the angel).
- Motors are controlled via PWM (servo/ESC signal).
- Each motor uses its own channel and GPIO.
- Speed is set in software; "stop" sets the PWM signal to the neutral value.

**Example motor mapping:**

| Motor | GPIO | Channel |
| ----- | ---- | ------- |
| Tree  | 17   | 0       |
| Angel | 18   | 1       |

---

## Wiring

- **Buttons:** Connect one side of the button to the desired GPIO, the other to GND. Use internal pull-up.
- **LEDs:** Anode to GPIO via a resistor (~220Ω), cathode to GND.
- **Motors:** PWM signal to the correct GPIO, power directly from a stable 5V source (not via the ESP32!).
- **LED strip:** Data to GPIO (e.g., 27), power 5V, GND shared with ESP32.

**Important:**  
Always use a **common GND** for all components!

---

## Disclaimer

> This project is "vibecoded" – built for fun and creativity.  
> Use at your own risk and enjoy the Christmas vibes!

---

Merry Christmas and happy building!  
✨
