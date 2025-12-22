# Home Automation Term Project 2025-2026

## Project Overview
[cite_start]This project is a **Home Automation System** designed for the "Introduction to Microcomputers" course[cite: 2, 30]. [cite_start]It consists of two PIC16F877A microcontrollers controlling various peripherals and a PC client application communicating via UART[cite: 36].

## Architecture
The system is divided into two main subsystems:

### 1. Board #1: Air Conditioner System
* **MCU:** PIC16F877A
* **Peripherals:**
    * [cite_start]**Temperature System:** LM35 Sensor, Heater, Cooler, Tachometer[cite: 113].
    * [cite_start]**User Interface:** 4x4 Keypad and Multiplexed 7-Segment Display[cite: 151, 266].
* [cite_start]**Functionality:** Maintains desired temperature by toggling heater/cooler and displays status[cite: 647].

### 2. Board #2: Curtain Control System
* **MCU:** PIC16F877A
* **Peripherals:**
    * [cite_start]**Actuator:** Stepper Motor (Unipolar) for curtain movement[cite: 681].
    * [cite_start]**Sensors:** LDR (Light), BMP180 (Pressure/Temp), Potentiometer[cite: 497, 524].
    * [cite_start]**Display:** LCD HD44780[cite: 340].
* [cite_start]**Functionality:** Controls curtain openness (0-100%) based on light levels or user input[cite: 696].

### 3. PC Application
* **Language:** C++
* [cite_start]**Communication:** UART (Serial) at 9600 baud[cite: 311].
* [cite_start]**Features:** Provides a menu-based interface to monitor sensors and set control values remotely[cite: 761].

## How to Run
1.  **Simulation:** Open `PICSimLab` and load the `.hex` files compiled from the `.s` assembly sources.
2.  **Connection:** Ensure the virtual UART ports are connected (e.g., COM1 <-> COM2).
3.  **PC App:** Compile `main.cpp` and run the executable to interact with the boards.