# Multi-Objective RPL Metrics Collection with Contiki-NG

This project provides a Contiki-NG simulation framework for collecting execution and network metrics in RPL-based Wireless Sensor Networks (WSNs). The goal is to enable realistic data collection to support multi-objective optimization studies, including energy consumption, latency, throughput, and communication reliability.

## Overview

The project is composed of two types of motes:

- **UDP Root (Sink)**: Acts as the RPL root and periodically sends probe packets to all known nodes. It collects and logs the metrics returned by each node.
- **UDP Node (Sensor)**: Joins the RPL network and responds to root probes by sending a structured metrics report containing energy, transmission, and reception data.

## File Structure

- `root.c`: Root mote logic. Sends packets, tracks nodes, receives and logs metrics.
- `node.c`: Node logic. Measures internal metrics and responds to root queries.
- `metrics-packet.h`: Contains the `node_metrics_packet_t` and `server_packet_t` structures used for metric communication.
- `project-conf.h`: Enables Contiki-NG’s energy tracking via `ENERGEST_CONF_ON`.
- `Makefile`: Build configuration for simulation in Cooja or native targets.

## Metrics Collected

Each node reports the following metrics:

- CPU energy consumption (mJ)
- LPM energy consumption (mJ)
- Radio TX energy (mJ)
- Radio RX energy (mJ)
- Total packets sent and received
- Total bytes transmitted and received
- Timestamps for latency calculation

## How It Works

1. The root node starts as the RPL DAG root and initializes a UDP server.
2. Nodes join the RPL network and periodically listen for server packets.
3. On receiving a server probe, each node:
   - Collects current energy and network usage.
   - Sends back a `node_metrics_packet_t` with the data.
4. The root receives, logs, and computes latency and other metrics.

## Simulation

This project is fully compatible with [Cooja](https://github.com/contiki-ng/cooja), Contiki-NG’s network simulator. It supports both simulation (Cooja) and native builds.

### To build and run in Cooja:
1. Open Cooja.
2. Import this project as a new simulation.
3. Add one root and multiple nodes.
4. Run the simulation and observe terminal output for metrics.
