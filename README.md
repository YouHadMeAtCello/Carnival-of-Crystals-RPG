# Carnival of Crystals RPG 🎡💎
**CPE 2200 Term Project - Embedded Systems**

An interactive, text-based RPG running on the STM32 Nucleo-L476RG.

## 🚀 Features
* **Non-Linear Engine:** Multiple paths and mini-games handled via a custom State Machine.
* **Persistent High Scores:** Utilizes Internal Flash memory (Page 254) to save scores across resets.
* **Interrupt-Driven I/O:** Real-time character input and high-speed "mashing" mechanics.
* **ASCII Visuals:** Custom-rendered terminal art.

## 🛠️ Technical Details
* **Platform:** Nucleo-L476RG (ARM Cortex-M4)
* **Communication:** UART via Serial Terminal (115200 Baud)
* **Logic:** 18-state Finite State Machine

## 🎮 How to Play
1. Connect the Nucleo board via USB.
2. Open PuTTY (or any Serial monitor) at 115200 Baud.
3. Press the **Reset** button on the board to begin.
