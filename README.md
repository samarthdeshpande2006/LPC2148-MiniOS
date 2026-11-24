# LPC2148 Mini Embedded Operating System (MiniOS)

A lightweight embedded task scheduler developed using **Embedded C** for the **ARM7TDMI-S LPC2148 microcontroller**.

## Overview

MiniOS is a custom-built embedded task scheduler designed to demonstrate fundamental Real-Time Operating System (RTOS) concepts without relying on an existing RTOS.

The system manages multiple independent tasks using timer-driven scheduling, task states and priority-based execution.

## Key Features

- Timer0-based system tick generation
- Cooperative multitasking
- Task Control Block (TCB)
- Priority-based task scheduling
- Task states: READY, RUNNING, WAITING and SUSPENDED
- Task suspend and resume functionality
- Non-blocking task execution
- UART-based diagnostics
- CPU utilization estimation
- GPIO, ADC and LCD peripheral integration

## Hardware & Software

**Hardware**
- NXP LPC2148 Development Board
- ARM7TDMI-S Processor
- LEDs and Push Buttons
- LM35 Temperature Sensor
- 16×2 LCD Display
- Buzzer

**Software**
- Embedded C
- Keil µVision
- Flash Magic
- Proteus (Optional)
- PuTTY / Tera Term

## Project Roadmap

| Week | Milestone |
|---|---|
| 01 | Hardware and GPIO/UART setup |
| 02 | Timer0 and system tick |
| 03 | Cooperative scheduler |
| 04 | TCB and task states |
| 05 | Priority-based scheduling |
| 06 | Peripheral integration |
| 07 | CPU monitoring and testing |
| 08 | Final integration and documentation |

## Future Enhancements

- Preemptive scheduling
- ARM context switching
- Round-robin scheduling
- Inter-task communication
- Semaphores and synchronization
- Dynamic task creation and deletion

## Project Information

- **Platform:** NXP LPC2148
- **Architecture:** ARM7TDMI-S
- **Programming Language:** Embedded C
- **Project Duration:** 8 Weeks
- **Project Type:** Embedded Systems

## Development Status

🚧 Under Development

---

*Exploring the fundamentals of embedded operating systems, task scheduling and ARM7 architecture.*


