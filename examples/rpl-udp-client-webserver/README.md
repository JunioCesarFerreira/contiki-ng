# RPL UDP Client Webserver

This Contiki-NG application simulates a network node that performs three main functions:

1. **RPL Node Communication:**  
   The `node_process` handles UDP communication. It periodically sends a UDP packet containing the network uptime (obtained via TSCH) to a root node when the network is reachable. When a UDP packet is received in the `udp_rx_callback`, it calculates packet latency based on timestamps and updates a global sensor latency value.

2. **Energy Monitoring:**  
   The `energest_example_process` periodically calculates energy consumption for different components (CPU active, low-power mode, radio TX, and radio RX) using the Energest module. Energy is computed by converting accumulated ticks into seconds and multiplying by predefined power consumption constants. The energy metrics are printed to the console for monitoring.

3. **Web Server:**  
   The `webserver_nogui_process` starts a simple HTTP server using the `httpd-simple` module. It generates JSON responses (via the `generate_routes` function) that include simulated sensor data (e.g., temperature and humidity), energy metrics, and latency measurements. This allows remote monitoring of the node’s status through a web interface.

**Usage Instructions:**

- **Running on the Cooja VM:**  
  The simulation is intended to be run using the provided Cooja Virtual Machine. You can find the VM at [link](https://github.com/JunioCesarFerreira/Cooja-Docker-VM-Setup/blob/main/vm/prepare-vm-enviroment.md). This VM comes pre-configured with Contiki-NG and Cooja, making it easier to execute and test simulations.

- **Transferring Files:**  
  To send your application files (such as your code or simulation configuration) to the VM, you can use the `send-files.py` script. This script simplifies the process of copying files from your host machine to the VM.
  
  In `contiki-ng/examples` run:
  ```bash
  py ./rpl-udp-client-webserver/send-files.py
  ```

  Before running, create the appropriate directories on the destination.

- **Executing Simulations:**  
  Once the files are transferred to the VM, you can start Cooja and run the simulation directly on the VM. 

This setup allows you to quickly deploy, test, and monitor your Contiki-NG simulations using the Cooja VM, with convenient file transfer provided by `send-files.py`.

