import serial
import serial.tools.list_ports
import time
import os
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import numpy as np

# --- Configuration ---
allowed_devices = ['USB VID:PID=303A:1001 SER=58:8C:81:A8:40:7C', 'USB VID:PID=303A:1001 SER=58:8C:81:A8:1E:C0', 'USB VID:PID=303A:1001 SER=58:8C:81:A8:1E:C0 LOCATION=0-1']
baudrate = 9600
max_elements = 50 
threshold = 1.1e9 # Adjust based on your sensor range

def findPorts():
    ports = serial.tools.list_ports.comports()
    found = []
    print("Searching for ESP32 Family Devices...")
    for port, desc, hwid in sorted(ports):
        print(f"Device found at: {desc}")
        if desc == "USB JTAG/serial debug unit":
            print(f"Found Allowed Device: {hwid}")
            found.append(port)
    return found

if __name__ == '__main__':
    active_ports = findPorts()
    if not active_ports:
        print("No devices found. Exiting.")
        exit()

    serials = {}
    data_store = {}
    ts0 = time.time()

    # Setup for each found device
    for port in active_ports:
        serials[port] = serial.Serial(port, baudrate, timeout=0.1)
        data_store[port] = {
            'times': [],
            'values': [],
            'peak_t': [],
            'peak_v': [],
            'last_val': 0
        }

    # --- Plot Setup ---
    num_devs = len(active_ports)
    fig, axes = plt.subplots(num_devs, 1, sharex=True, squeeze=False)
    plot_objects = {}

    for i, port in enumerate(active_ports):
        ax = axes[i, 0]

        ln, = ax.plot([], [], 'r-', label=f"Signal ({port})")
        av, = ax.plot([], [], 'b--', alpha=0.5, label="Baseline")
        pk, = ax.plot([], [], 'go', markersize=8, label="Peak")
        
        if port == "/dev/cu.usbmodem101":
            ax.set_title(f"Device: Module B on {port}")
        elif port == "/dev/cu.usbmodem1101":
            ax.set_title(f"Device: Module A on {port}")
        ax.legend(loc="upper right")
       
        plot_objects[port] = {'line': ln, 'avg': av, 'peaks': pk, 'ax': ax}

    running = True

    def update(frame):
        global running
        artists = []

        for port in active_ports:
            ser = serials[port]
            ds = data_store[port]
            objs = plot_objects[port]

            try:
                if ser.in_waiting > 0:
                    raw = ser.readline().decode('utf-8', errors='ignore').strip()
                    ts = time.time() - ts0
                    val = float(raw)

                    ds['times'].append(ts)
                    ds['values'].append(val)

                    # Peak Detection (Rising Edge)
                    if ds['last_val'] < threshold and val >= threshold:
                        ds['peak_t'].append(ts)
                        ds['peak_v'].append(val)
                    
                    ds['last_val'] = val

                    # Maintain Buffer
                    if len(ds['times']) > max_elements:
                        ds['times'].pop(0)
                        ds['values'].pop(0)
                    
                    while ds['peak_t'] and ds['peak_t'][0] < ds['times'][0]:
                        ds['peak_t'].pop(0)
                        ds['peak_v'].pop(0)

                    # Update Artist Data
                    objs['line'].set_data(ds['times'], ds['values'])
                    objs['avg'].set_data(ds['times'], [np.mean(ds['values'])] * len(ds['times']))
                    objs['peaks'].set_data(ds['peak_t'], ds['peak_v'])

                    # Scaling
                    
                    objs['ax'].set_ylim(-2.5e+9, 2.5e+9)

                    if ds['times']:
                        objs['ax'].set_xlim(ds['times'][0], ds['times'][-1])

            except Exception as e:
                pass # Handle malformed serial strings
            
            artists.extend([objs['line'], objs['avg'], objs['peaks']])

        return artists

    ani = FuncAnimation(fig, update, interval=10, blit=False, cache_frame_data=False)
    
    try:
        plt.tight_layout()
        plt.show()
    finally:
        for ser in serials.values():
            ser.close()
        print("Ports closed.")