# Contiki-NG Simulation with Cooja

This example contains Contiki-NG simulation code for network performance and energy consumption evaluation using the Cooja simulator.

## Project Overview

The project consists of two main Contiki-NG applications:

1. **UDP Client**: A node that periodically sends packets containing energy and network performance metrics.
2. **UDP Server**: A node that receives the packets and logs the metrics.

These applications use IPv6 over TSCH for communication and leverage the Energest module to measure energy consumption.

## Features

- UDP communication between client and server.
- Energy consumption monitoring (CPU, LPM, Radio TX, Radio RX).
- Latency and packet transmission statistics.
- IPv6 address handling and packet logging.

## File Structure

- `udp-client.c`: Implementation of the UDP client.
- `udp-server.c`: Implementation of the UDP server.
- `send-files-to-vm.py`: Script to transfer source files to the virtual machine.
- `fetch-files-from-vm.py`: Script to retrieve simulation XML (`.csc`) and node position files.
- `send-files-to-container.py`: Script to transfer files to the Docker container.
- `fetch-logs-from-container.py`: Script to retrieve simulation logs.

## Usage Workflow

### Using a Virtual Machine
1. Write or modify the C files in the working directory.
2. Ensure the VM is running. [See](https://github.com/JunioCesarFerreira/Cooja-Docker-VM-Setup/blob/main/vm/prepare-vm-enviroment.md).
3. Use `send-files-to-vm.py` to transfer the files to the VM.
4. Run Cooja GUI inside the VM to test the simulations.
5. Use `fetch-files-from-vm.py` to retrieve simulation results (`.csc` and `Positions.dat`).

### Using a Docker Container
1. Ensure the container is running. [See](https://github.com/JunioCesarFerreira/Cooja-Docker-VM-Setup/tree/main/ssh-docker-cooja).
2. Use `send-files-to-container.py` to transfer files to the container.
3. Connect to the container via SSH.
4. Inside the container, rename the simulation file:
   ```sh
   mv simulation.xml simulation.csc
   ```
5. Run Cooja in headless mode:
   ```sh
   java --enable-preview -Xms4g -Xmx4g -jar build/libs/cooja.jar --no-gui simulation.csc > sim.log
   ```
6. Use `fetch-logs-from-container.py` to retrieve the simulation results.

## Dependencies

- Contiki-NG
- Cooja Simulator
- Python (for automation scripts)
- Docker (if running in a containerized environment)
- Oracle VM VirtualBox (if running in a Ubuntu VM)
- WSL (if you use Windows)
- VS Code (Code edition)