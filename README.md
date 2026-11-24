# LPC2148 MiniOS: corrected 4-task application

## Status
This is full application-level source for ARM Compiler 5 / Keil µVision. It is **not certified error-free, not compiled in Keil, and not tested on an LPC2148 board**. It requires the **official LPC2148 startup assembly** from your device support installation, correct clock/interrupt setup, and matching wiring. Do not use the previously provided illustrative startup file.

## Wiring assumptions
- P0.0 = UART0 TX, P0.1 = UART0 RX; 3.3V TTL USB-UART and common ground; 9600 8N1.
- P0.10 = active-high LED via resistor.
- P0.28 = AD0.1 connected to LM35 output, voltage never above 3.3V; analog reference assumed 3.3V.
- P0.11 = active-high buzzer **driver input**. Use transistor/driver and suitable supply, not a high-current buzzer connected directly to the GPIO.
- The LM35 may need a suitable supply above 3.3V depending on part; ensure output stays within ADC limits and grounds are common.

## Keil steps
1. Create a new Keil µVision project targeting NXP LPC2148.
2. Select ARM Compiler 5 (ARMCC). ARM Compiler 6 is not supported by this source unchanged.
3. Add `main.c` and the **official** LPC2148 startup assembly from the device pack / legacy device database.
4. In startup, configure PLL for CCLK=60 MHz and VPBDIV=1 for PCLK=60 MHz. Actual board oscillator must be known; startup must initialize C runtime and route VIC IRQs correctly and enable IRQs.
5. In Options for Target > Output, select Create HEX File. Build and inspect errors/warnings.
6. Verify clock and IRQ behavior in a debugger/simulator before flashing. Program the HEX using board-supported ISP/JTAG.

## Four tasks
1. UART polling every 10ms: `help`, `status`, `temp`, `suspend N`, `resume N`; refuses `suspend 1` to avoid locking out UART control.
2. LM35 reading every 500ms via AD0.1; threshold 35.0C turns P0.11 on. Uses bounded ADC polling, so no infinite ADC wait.
3. LED toggles every 500ms (1-second full cycle).
4. Diagnostics every 2000ms.

## Caveats
- UART transmit is blocking; at 9600 baud, printing diagnostics/status may delay lower-priority tasks. No hard real-time deadline guarantee.
- The ADC reading assumes VREF=3.3V and an LM35 with 10mV/C slope; sensor accuracy and analog calibration require physical testing.
- Task 2 has no negative-temperature handling. A standard LM35 without negative supply is suitable for positive temperatures.
- Scheduler is cooperative, not preemptive; no independent per-task stacks.
- Tick rollover comparisons assume task periods less than 2^31ms.
- The actual VIC vector/IRQ startup arrangement must match the official Keil startup. If it does not, timer interrupts will not run.
- No LCD: it was not among the four tasks specified in the latest request.
