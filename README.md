# Data Link Layer Protocols Simulation

This project simulates data link layer protocols for communication between two nodes connected via a noisy channel. The implementation demonstrates the **Selective Repeat protocol** with noise simulation, **Byte Stuffing** for framing, and **CRC** for error detection.

---

## Table of Contents

- [📖 Overview](#overview)  
- [✨ Features](#features)  
- [📂 Input Files](#input-files)  
- [📝 System Outputs](#system-outputs)  
- [🧠 Key Algorithms](#key-algorithms)  
- [🚀 Getting Started](#getting-started) 

---

## Overview

This project demonstrates a reliable communication simulation over a noisy channel. The simulation handles:

- Errors such as modification, loss, duplication, and delay.
- Framing with byte stuffing.
- Error detection using Cyclic Redundancy Check (CRC).

The system consists of:
- **Two nodes** for communication (Node0 and Node1).
- **One coordinator** for initialization.

---

## Features

- **Selective Repeat Protocol**: Implements acknowledgment and retransmission based on sequence numbers.
- **Noise Simulation**: Simulates errors like modification, loss, duplication, and delay.
- **Configurable Parameters**: Set window size, timeout interval, and other parameters via an `.ini` file.
- **Detailed Logging**: Outputs message processing details in a log file.

---

## Input Files

1. **Input Files for Nodes** (`input0.txt` and `input1.txt`):
   - Contains messages prefixed with a 4-bit error code (e.g., `1010 Data Link`).
   - Error codes represent potential errors during transmission.

2. **Coordinator File** (`coordinator.txt`):
   - Specifies the starting node and the start time.

---

## System Outputs

The system generates:
- A log file (`output.txt`) containing details of message transmissions, including errors and acknowledgments.
- Console output reflecting the log file content for real-time monitoring.

---

## Key Algorithms

1. **Selective Repeat Protocol**:
   - Manages retransmissions for errored frames with timeout mechanisms.
   - Handles out-of-order frame delivery.

2. **Byte Stuffing**:
   - Adds framing characters (`$` for start/end, `/` for escape).

3. **CRC for Error Detection**:
   - Ensures data integrity during transmission.

---

## Getting Started

### Prerequisites
- OMNeT++
- GCC
- Java 11

