# Capstone-Repo-Computational-Anomaly-Detection-Subsystem
Repo for my EE undergraduate senior capstone project (real-time cyber threat detector)

Purpose of this code is real-time analysis of computational data (CPU usage, CPU temp, mem%)
to monitor for potential flood/DDoS attacks on edge devices within a C2 Kubernetes cluster.

- Detector uses statistical analysis (modified Z-score) to detect anomalies as data comes through
- Detector samples data to adjust parameters for anomaly behavior over time
- Detector uses a ROS2 Humble subscriber to receive data (20 Hz)
- Detector averages data points to account for telemetry noise from edge devices
- Detector is coded entirely in C++ and multithreaded to be as fast as possible
