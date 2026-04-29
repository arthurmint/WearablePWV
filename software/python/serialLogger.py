import serial
import serial.tools.list_ports
import time


ports = serial.tools.list_ports.comports()

## Add allowed hwid here
allowed_devices = ['USB VID:PID=303A:1001 SER=58:8C:81:A8:40:7C']

baudrate = 9600

def findPort():
    print("Searching for ESP32 Family Devices...")
    for port, desc, hwid in sorted(ports):
        print("{}: {} [{}]".format(port, desc, hwid))

        if hwid in allowed_devices:
            print("ESP32-C3 Found at {}".format(port))
            return port
    print("No device found")
    return 0




if __name__ == '__main__':
    ## Handle serial read
    port = serial.Serial(findPort(), baudrate)

    ## Handle data log
    filename = str(input("Enter filename: ")) + ".csv"
    print("Writing to {}".format(filename))
    file = open(filename, 'w')

    ## Handle plot
    ts0 = time.time()

    while True:
        try: 
            ts = time.time() - ts0
            line = str(port.readline())

            file.write(str(ts))
            file.write(",")
            file.write(line[2:][:-5])
            file.write("\n")

            time.sleep(0.01)

            print("Timestamp: [{}] Sig: {}".format(ts, line[2:][:-5]))
        except KeyboardInterrupt:
            break
    
    print("Data file saved to {}".format(filename))









