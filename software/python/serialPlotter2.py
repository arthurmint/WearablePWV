import serial
import serial.tools.list_ports
import time
import os
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import numpy as np

# --- Configuration ---
allowed_devices = ['USB VID:PID=303A:1001 SER=58:8C:81:A8:40:7C', 'USB VID:PID=303A:1001 SER=58:8C:81:A8:40:7C LOCATION=1-1.1']
baudrate = 9600
max_elements = 50 
threshold = 1.1e9 

# SET YOUR FIXED Y-AXIS LIMITS HERE
y_min = -2.5e9
y_max = 2.5e9
# ---------------------

def findPorts():
    ports = serial.tools.list_ports.comports()
    found = []
    print("Searching for ESP32 Family Devices...")
    for port, desc, hwid in sorted(ports):
        if desc == "USB JTAG/serial debug unit":
            print(f"Found Device: {port}")
            found.append(port)
    return found

def get_replay_data(filename):
    replay_buffer = []
    if not os.path.exists(filename):
        print(f"Error: File {filename} not found.")
        return None
    with open(filename, 'r') as f:
        for line in f:
            try:
                parts = line.strip().split(',')
                replay_buffer.append((float(parts[0]), float(parts[1]), int(parts[2])))
            except: continue
    return replay_buffer

if __name__ == '__main__':
    mode = input("Select Mode: [1] Live Stream  [2] Replay File: ").strip()
    
    active_ports = []
    files = {}
    serials = {}
    replay_buffers = {}
    replay_indices = {}
    data_store = {}
    ts0 = time.time()

    if mode == '1':
        active_ports = findPorts()
        if not active_ports:
            print("No devices found."); exit()
        
        base_name = input("Enter base filename for data: ")
        for port in active_ports:
            clean_port = port.replace("/", "_").replace("\\", "_")
            fname = f"{base_name}_{clean_port}.csv"
            files[port] = open(fname, 'w')
            serials[port] = serial.Serial(port, baudrate, timeout=0.1)
    else:
        base_name = input("Enter base filename used during recording: ")
        num_replay = int(input("How many devices to replay? "))
        for i in range(num_replay):
            p_name = input(f"Enter port name for device {i+1}: ")
            active_ports.append(p_name)
            clean_port = p_name.replace("/", "_").replace("\\", "_")
            replay_buffers[p_name] = get_replay_data(f"{base_name}_{clean_port}.csv")
            replay_indices[p_name] = 0

    for port in active_ports:
        data_store[port] = {
            'times': [], 'values': [], 
            'peak_t': [], 'peak_v': [], 
            'last_val': 0
        }

    # --- Plot Setup ---
    num_devs = len(active_ports)
    fig, axes = plt.subplots(num_devs, 1, squeeze=False)
    plot_objects = {}

    for i, port in enumerate(active_ports):
        ax = axes[i, 0]
        ln, = ax.plot([], [], 'r-', label=f"Signal ({port})")
        pk, = ax.plot([], [], 'go', markersize=8, label="Peak")
        
        if port == "/dev/cu.usbmodem101":
            ax.set_title(f"Device: Module B on {port}")
        elif port == "/dev/cu.usbmodem1101":
            ax.set_title(f"Device: Module A on {port}")
        ax.legend(loc="upper right")
        
        # 1. Apply fixed Y limits
        ax.set_ylim(y_min, y_max)
        # 2. Disable Y autoscaling
        ax.set_autoscaley_on(False) 
        
        plot_objects[port] = {'line': ln, 'peaks': pk, 'ax': ax}

    running = True

    def update(frame):
        global running
        artists = []

        for port in active_ports:
            ds = data_store[port]
            objs = plot_objects[port]
            ts, val = None, None
            is_peak = 0

            if mode == '1':
                ser = serials[port]
                if ser.in_waiting > 0:
                    try:
                        raw = ser.readline().decode('utf-8', errors='ignore').strip()
                        ts = time.time() - ts0
                        val = float(raw)
                        if ds['last_val'] < threshold and val >= threshold:
                            is_peak = 1
                        files[port].write(f"{ts},{val},{is_peak}\n")
                    except: continue
            else:
                idx = replay_indices[port]
                buf = replay_buffers[port]
                if buf and idx < len(buf):
                    ts, val, is_peak = buf[idx]
                    replay_indices[port] += 1
                elif idx >= len(buf):
                    running = False

            if ts is not None and val is not None:
                ds['times'].append(ts)
                ds['values'].append(val)
                if is_peak:
                    ds['peak_t'].append(ts)
                    ds['peak_v'].append(val)
                
                ds['last_val'] = val

                if len(ds['times']) > max_elements:
                    ds['times'].pop(0)
                    ds['values'].pop(0)
                while ds['peak_t'] and ds['peak_t'][0] < ds['times'][0]:
                    ds['peak_t'].pop(0)
                    ds['peak_v'].pop(0)

                objs['line'].set_data(ds['times'], ds['values'])
                objs['peaks'].set_data(ds['peak_t'], ds['peak_v'])

                # 3. Only rescale X (time), leave Y alone
                objs['ax'].relim()
                objs['ax'].autoscale_view(scaley=False, scalex=True)
                if ds['times']:
                    objs['ax'].set_xlim(ds['times'][0], ds['times'][-1])

            artists.extend([objs['line'], objs['peaks']])
        return artists

    ani = FuncAnimation(fig, update, interval=10, blit=False, cache_frame_data=False)
    
    try:
        plt.tight_layout()
        plt.show()
    finally:
        for s in serials.values(): s.close()
        for f in files.values(): f.close()
        print("Cleanup complete.")