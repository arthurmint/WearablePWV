import serial
import serial.tools.list_ports
import time
import os
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation

# --- Configuration ---
allowed_devices = ['USB VID:PID=303A:1001 SER=58:8C:81:A8:40:7C']
baudrate = 9600
max_elements = 50 

def findPort():
    ports = serial.tools.list_ports.comports()
    print("Searching for ESP32 Family Devices...")
    for port, desc, hwid in sorted(ports):
        print("{}: {} [{}]".format(port, desc, hwid))
        if hwid in allowed_devices:
            print("ESP32-C3 Found at {}".format(port))
            return port
    print("No device found")
    return None

def get_replay_data(filename):
    """Loads CSV data into a list for replaying."""
    replay_buffer = []
    if not os.path.exists(filename):
        print(f"Error: File {filename} not found.")
        return None
    
    with open(filename, 'r') as f:
        for line in f:
            try:
                t_str, v_str = line.strip().split(',')
                replay_buffer.append((float(t_str), float(v_str)))
            except ValueError:
                continue
    return replay_buffer

if __name__ == '__main__':
    mode = input("Select Mode: [1] Live Stream  [2] Replay File: ").strip()
    
    ser = None
    file = None
    replay_data = None
    replay_index = 0
    ts0 = time.time()

    if mode == '1':
        # --- Live Mode Setup ---
        found_port = findPort()
        if not found_port:
            exit()
        ser = serial.Serial(found_port, baudrate, timeout=1)
        filename = input("Enter filename to save data: ") + ".csv"
        file = open(filename, 'w')
        print(f"Streaming live... Saving to {filename}")
    else:
        # --- Replay Mode Setup ---
        filename = input("Enter filename to replay (without .csv): ") + ".csv"
        replay_data = get_replay_data(filename)
        if not replay_data:
            exit()
        print(f"Replaying data from {filename}...")

    # Shared Data buffers
    times = []
    values = []
    running = True

    # Set up the plot
    fig, ax = plt.subplots()
    line_plot, = ax.plot([], [], 'r-')
    ax.set_title("PPG Data Plotter on {}".format(found_port) if mode == '1' else f"Replay: {filename}")
    ax.set_xlabel("Time (s)")
    ax.set_ylabel("Value")

    def update(frame):
        global running, replay_index
        if not running:
            return line_plot,

        try:
            ts, val = None, None

            if mode == '1':
                # Logic for Live Serial
                if ser.is_open and ser.in_waiting > 0:
                    raw_line = ser.readline().decode('utf-8', errors='ignore').strip()
                    ts = time.time() - ts0
                    try:
                        val = float(raw_line)
                        file.write(f"{ts},{val}\n")
                        file.flush()
                    except ValueError:
                        return line_plot,
            else:
                # Logic for Replay
                if replay_index < len(replay_data):
                    ts, val = replay_data[replay_index]
                    replay_index += 1
                    # Small sleep to simulate time passing if the file is large
                    time.sleep(0.01) 
                else:
                    running = False
                    print("Replay finished.")
                    return line_plot,

            if ts is not None and val is not None:
                times.append(ts)
                values.append(val)

                if len(times) > max_elements:
                    times.pop(0)
                    values.pop(0)

                line_plot.set_data(times, values)
                ax.relim()
                ax.autoscale_view()

        except Exception as e:
            print(f"Error: {e}")
            running = False
            
        return line_plot,

    ani = FuncAnimation(fig, update, interval=10, blit=False, cache_frame_data=False)

    try:
        plt.show()
    except KeyboardInterrupt:
        print("\nShutdown signaled...")
    finally:
        running = False
        if ani.event_source:
            ani.event_source.stop()
        if file and not file.closed:
            file.close()
        if ser and ser.is_open:
            ser.close()
        print("Cleanup complete.")
