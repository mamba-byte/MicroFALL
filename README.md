# Home Automation Term Project 2025-2026

## Project Overview
This project is a **Home Automation System** designed for the "Introduction to Microcomputers" course. It consists of two PIC16F877A microcontrollers controlling various peripherals and a PC client application communicating via UAR.

## Architecture
The system is divided into two main subsystems:

### 1. Board #1: Air Conditioner System
* **MCU:** PIC16F877A
* **Peripherals:**
    * **Temperature System:** LM35 Sensor, Heater, Cooler, Tachometer.
    * **User Interface:** 4x4 Keypad and Multiplexed 7-Segment Display.
* **Functionality:** Maintains desired temperature by toggling heater/cooler and displays status.

### 2. Board #2: Curtain Control System
* **MCU:** PIC16F877A
* **Peripherals:**
    * **Actuator:** Stepper Motor (Unipolar) for curtain movement.
    * **Sensors:** LDR (Light), BMP180 (Pressure/Temp), Potentiometer.
    * **Display:** LCD HD44780.
* **Functionality:** Controls curtain openness (0-100%) based on light levels or user input

### 3. PC Application
* **Language:** C++
* **Communication:** UART (Serial) at 9600 baud.
* **Features:** Provides a menu-based interface to monitor sensors and set control values remotely.

## How to Run
1.  **Simulation:** Open `PICSimLab` and load the `.hex` files compiled from the `.s` assembly sources.
2.  **Connection:** Ensure the virtual UART ports are connected (e.g., COM1 <-> COM2).
3.  **PC App:** Compile `main.cpp` and run the executable to interact with the boards.
