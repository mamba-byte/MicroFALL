#include <iostream>
#include <string>
#include <vector>
#include <cstdio>

// macOS / POSIX specific headers
#include <fcntl.h>      // For file control definitions
#include <errno.h>      // For error number definitions
#include <termios.h>    // For POSIX terminal control definitions
#include <unistd.h>     // For UNIX standard definitions (read/write/close)
#include <sys/ioctl.h>

using namespace std;

// ===========================================================================
// [R2.3-1] Base Class: HomeAutomationSystemConnection
// ===========================================================================
class HomeAutomationSystemConnection {
protected:
    string portName;
    int baudRate;
    int serial_fd; // File descriptor for serial port
    bool connected;

public:
    HomeAutomationSystemConnection() : baudRate(9600), connected(false), serial_fd(-1) {}

    // On macOS, ports look like "/dev/tty.usbserial-XXXX" or "/dev/tty.SLAB_USBtoUART"
    void setPortPath(string port) { this->portName = port; }
    
    // Kept for compatibility with PDF diagram, but handles integer-based baud conversion internally
    void setBaudRate(int rate) { this->baudRate = rate; }

    bool openConnection() {
        // Open the serial port (Read/Write, No controlling terminal, No delay)
        serial_fd = open(portName.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
        
        if (serial_fd == -1) {
            perror("Unable to open port");
            return false;
        }

        // Configure Port
        struct termios options;
        tcgetattr(serial_fd, &options); // Get current options

        // Set Baud Rate (PDF requires 9600 [cite: 310])
        speed_t speed;
        switch(baudRate) {
            case 9600: speed = B9600; break;
            case 19200: speed = B19200; break;
            case 115200: speed = B115200; break;
            default: speed = B9600;
        }
        cfsetispeed(&options, speed);
        cfsetospeed(&options, speed);

        // Set 8N1 (8 Data bits, No Parity, 1 Stop bit) [cite: 310]
        options.c_cflag &= ~PARENB; // No Parity
        options.c_cflag &= ~CSTOPB; // 1 Stop Bit
        options.c_cflag &= ~CSIZE;  // Mask character size bits
        options.c_cflag |= CS8;     // 8 Data Bits

        // Disable hardware flow control
        options.c_cflag &= ~CRTSCTS;

        // Enable receiver and set local mode
        options.c_cflag |= (CLOCAL | CREAD);

        // Apply settings
        tcsetattr(serial_fd, TCSANOW, &options);

        // Restore blocking behavior (read will wait for data)
        fcntl(serial_fd, F_SETFL, 0);

        connected = true;
        return true;
    }

    bool closeConnection() {
        if (connected && serial_fd != -1) {
            close(serial_fd);
            connected = false;
            serial_fd = -1;
            return true;
        }
        return false;
    }

    // Send a single byte
    void sendByte(unsigned char data) {
        if (!connected) return;
        write(serial_fd, &data, 1);
    }
    
    // Receive a single byte
    unsigned char readByte() {
        if (!connected) return 0;
        unsigned char buffer[1] = {0};
        int n = read(serial_fd, buffer, 1);
        if (n < 0) return 0; // Error
        return buffer[0];
    }
    
    virtual void update() = 0; // Pure virtual
};

// ===========================================================================
// Derived Class 1: AirConditionerSystemConnection [cite: 735]
// ===========================================================================
class AirConditionerSystemConnection : public HomeAutomationSystemConnection {
private:
    float desiredTemperature;
    float ambientTemperature;
    int fanSpeed;

public:
    AirConditionerSystemConnection() : desiredTemperature(0), ambientTemperature(0), fanSpeed(0) {}

    void update() override {
        // [cite: 675] Example Protocol implementation
        // Request Ambient Low Byte (0x03)
        sendByte(0x03); 
        int frac = readByte();
        
        // Request Ambient High Byte (0x04)
        sendByte(0x04);
        int integer = readByte();
        
        ambientTemperature = integer + (frac / 10.0f);
        
        // Request Fan Speed (0x05)
        sendByte(0x05); 
        fanSpeed = readByte();
    }

    bool setDesiredTemp(float temp) {
        desiredTemperature = temp;
        // Logic for[cite: 675]: 11xxxxxx (Int), 10xxxxxx (Frac)
        int integer = (int)temp;
        int frac = (int)((temp - integer) * 10);
        
        unsigned char cmdInt = 0xC0 | (integer & 0x3F); 
        sendByte(cmdInt);
        
        unsigned char cmdFrac = 0x80 | (frac & 0x3F);   
        sendByte(cmdFrac);
        return true;
    }

    float getAmbientTemp() { return ambientTemperature; }
    float getDesiredTemp() { return desiredTemperature; }
    int getFanSpeed() { return fanSpeed; }
};

// ===========================================================================
// Derived Class 2: CurtainControlSystemConnection [cite: 744]
// ===========================================================================
class CurtainControlSystemConnection : public HomeAutomationSystemConnection {
private:
    float curtainStatus;
    // Additional vars omitted for brevity as per PDF structure
    
public:
    void update() override {
        // [cite: 719] Request Curtain Status (simplified)
        sendByte(0x01); // Get Low Byte
        int frac = readByte();
        sendByte(0x02); // Get High Byte
        int integer = readByte();
        curtainStatus = integer + (frac / 10.0f);
    }

    bool setCurtainStatus(float status) {
        // [cite: 719] Set Curtain Status
        int val = (int)status;
        unsigned char cmd = 0xC0 | (val & 0x3F); 
        sendByte(cmd);
        return true;
    }
    
    float getCurtainStatus() { return curtainStatus; }
};

// ===========================================================================
// Application & Menus [cite: 768]
// ===========================================================================
void clearScreen() { 
    // ANSI escape code to clear screen on macOS terminal
    cout << "\033[2J\033[1;1H"; 
}

int main() {
    AirConditionerSystemConnection ac;
    CurtainControlSystemConnection curtain;

    // IMPORTANT: On macOS, you must find your specific port name.
    // Run "ls /dev/tty.*" in terminal to find them.
    // They usually look like "/dev/tty.usbserial-0001" or similar.
    string port1, port2;
    
    cout << "Enter Port for Air Conditioner (e.g., /dev/tty.usbserial-A): ";
    cin >> port1;
    cout << "Enter Port for Curtain Control (e.g., /dev/tty.usbserial-B): ";
    cin >> port2;

    ac.setPortPath(port1); 
    if (!ac.openConnection()) {
        cout << "Failed to open AC port!" << endl;
        return 1;
    }
    
    curtain.setPortPath(port2);
    if (!curtain.openConnection()) {
        cout << "Failed to open Curtain port!" << endl;
        return 1;
    }

    int choice = 0;
    while (choice != 3) {
        clearScreen();
        // [cite: 769] Main Menu
        cout << "MAIN MENU" << endl;
        cout << "1. Air Conditioner" << endl;
        cout << "2. Curtain Control" << endl;
        cout << "3. Exit" << endl;
        cout << "Choice: ";
        cin >> choice;

        if (choice == 1) {
            int subChoice = 0;
            while (subChoice != 2) {
                ac.update(); 
                clearScreen();
                // [cite: 773] AC Info Screen
                cout << "Home Ambient Temperature: " << ac.getAmbientTemp() << " C" << endl;
                cout << "Home Desired Temperature: " << ac.getDesiredTemp() << " C" << endl;
                cout << "Fan Speed: " << ac.getFanSpeed() << " rps" << endl;
                cout << "-------------------------" << endl;
                
                // [cite: 775] Sub Menu
                cout << "MENU" << endl;
                cout << "1. Enter the desired temperature" << endl;
                cout << "2. Return" << endl;
                cin >> subChoice;

                if (subChoice == 1) {
                    float t;
                    cout << "Enter Desired Temp: ";
                    cin >> t;
                    ac.setDesiredTemp(t);
                }
            }
        } else if (choice == 2) {
            int subChoice = 0;
            while (subChoice != 2) {
                curtain.update();
                clearScreen();
                // [cite: 782] Curtain Info Screen
                cout << "Curtain Status: " << curtain.getCurtainStatus() << " %" << endl;
                cout << "-------------------------" << endl;
                
                // [cite: 784] Sub Menu
                cout << "MENU" << endl;
                cout << "1. Enter the desired curtain status" << endl;
                cout << "2. Return" << endl;
                cin >> subChoice;

                if (subChoice == 1) {
                    float s;
                    cout << "Enter Desired Curtain: ";
                    cin >> s;
                    curtain.setCurtainStatus(s);
                }
            }
        }
    }

    ac.closeConnection();
    curtain.closeConnection();
    return 0;
}