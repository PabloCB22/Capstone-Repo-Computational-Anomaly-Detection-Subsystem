# Capstone-Repo-Computational-Anomaly-Detection-Subsystem
repo for my EE undergrade senior capstone project (real time cyber threat detector)

Purpose of this code is real time analysis of compotational data (CPU usage, CPU temp, mem%)
to monitor for potential Flood/DDOS attacks on edge-decives within a C2 kubernetes cluster 

- Detector uses a statistical analysis (modified Z-score) to detect any anomalies as data comes through
- Detector samples data to adjust parameters for anomaly behavior through time 
- Detector uses a ROS2_Humble Subscriber to recive data (20Hz)
- Detector averages data points to account for telemetry noise from edge device
- Detector coded entierly in c++ and multithreaded to be as fast as possible
  
