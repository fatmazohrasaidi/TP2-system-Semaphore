# 🚛🗑️ Urban Waste Collection Simulation 🖥️

This repository contains a simulation of an urban waste collection system, implemented using UNIX process synchronization and communication tools.
The goal is to manage the operations of garbage trucks (camions) and a controller (controleur) in a synchronized manner.

## 🎯 **Objective**
The system simulates the following:
- **Trucks (Camions)**: Each truck waits for assignments, collects garbage from bins, and transports it to the dump. After completing tasks, they report back to the controller and can either take on new tasks, rest, or refuel.
- **Controller (Controleur)**: Assigns tasks (dump missions, rest, refuel) to trucks based on their fuel consumption and availability.

## 🛠️ **Key Components**
- **Process Synchronization**: Using semaphores to manage the coordination between the controller and trucks.
- **Message Queues**: For communication between the controller and trucks.
- **Shared Memory**: To store the state of garbage bins and other system parameters.

## 📊 **Parameters**
- **M (Bins)**: 20
- **N (Trucks)**: 5
- **R (Buffer Size)**: 3
- **CP (Fuel Capacity)**: 300
- **Distances**: 
  - Between bins and dump: ≤ 20
  - Between bins: ≤ 20
- **Fuel Consumption**: Proportional to distance (C = 2)
- **Fuel Thresholds**: Min = 70, Max = 12 missions before refueling.


## 🚀 **How to Use**
1. Clone the repository.
2. Compile the code using a UNIX-based system.
3. Run the simulation to observe the synchronized operations of the trucks and controller.
