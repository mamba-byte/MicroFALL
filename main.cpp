#include <iostream>
#include <string>
#include <windows.h> // For Serial Communication
#include <vector>

using namespace std;

// ===========================================================================
// [R2.3-1] Base Class: HomeAutomationSystemConnection
// ===========================================================================
class HomeAutomationSystemConnection {
protected:
    int comPort;
    int baudRate;
    HANDLE hSerial;
    bool connected;

public:
    HomeAutomationSystemConnection() : comPort(0), baudRate(9600), connected(false), hSerial(INVALID_HANDLE_VALUE) {}

    void setComPort(int port) { this->comPort = port; }
    void setBaudRate(int rate) { this->baudRate = rate; }

    bool openConnection() {
        string portName = "\\\\.\\COM" + to_string(comPort);
        hSerial = CreateFileA(portName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

        if (hSerial == INVALID_HANDLE_VALUE) return false;

        DCB dcbSerialParams = {0};
        dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
        if (!GetCommState(hSerial, &dcbSerialParams)) return false;

        dcbSerialParams.BaudRate = baudRate;
        dcbSerialParams.ByteSize = 8;
        dcbSerialParams.StopBits = ONESTOPBIT;
        dcbSerialParams.Parity = NOPARITY;

        if (!SetCommState(hSerial, &dcbSerialParams)) return false;

        connected = true;
        return true;
    }

    bool closeConnection() {
        if (connected) {
            CloseHandle(hSerial);
            connected = false;
            return true;
        }
        return false;
    }

    // Helper to send byte
    void sendByte(unsigned char data) {
        if (!connected) return;
        DWORD bytesWritten;
        WriteFile(hSerial, &data, 1, &bytesWritten, NULL);
    }
    
    // Helper to receive byte
    unsigned char readByte() {
        if (!connected) return 0;
        unsigned char buffer[1] = {0};
        DWORD bytesRead;
        ReadFile(hSerial, buffer, 1, &bytesRead, NULL);
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
        // [cite: 675] Send commands to PIC to get data
        // Protocol Example: Send 0x03 (Get Amb Low), Read Byte, Send 0x04 (Get Amb High), Read Byte
        // Implementation simplified for simulation
        sendByte(0x03); 
        int frac = readByte();
        sendByte(0x04);
        int integer = readByte();
        ambientTemperature = integer + (frac / 10.0f);
        
        sendByte(0x05); // Get Fan Speed
        fanSpeed = readByte();
    }

    bool setDesiredTemp(float temp) {
        desiredTemperature = temp;
        // [cite: 675] Logic to send Set command (11xxxxxx for int, 10xxxxxx for frac)
        int integer = (int)temp;
        int frac = (int)((temp - integer) * 10);
        
        unsigned char cmdInt = 0xC0 | (integer & 0x3F); // 11xxxxxx
        sendByte(cmdInt);
        
        unsigned char cmdFrac = 0x80 | (frac & 0x3F);   // 10xxxxxx
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
    float outdoorTemp;
    float outdoorPress;
    double lightIntensity;

public:
    void update() override {
        // [cite: 719] Send UART commands to fetch data
        sendByte(0x01); // Get Curtain Low
        // ... (Reading implementation similar to AC class)
    }

    bool setCurtainStatus(float status) {
        curtainStatus = status;
        // [cite: 719] Send Set Command
        int val = (int)status;
        unsigned char cmd = 0xC0 | (val & 0x3F); // Simplified protocol
        sendByte(cmd);
        return true;
    }
    
    float getCurtainStatus() { return curtainStatus; }
};

// ===========================================================================
// [R2.4-1] Application & Menus
// ===========================================================================
void clearScreen() { system("cls"); }

int main() {
    AirConditionerSystemConnection ac;
    CurtainControlSystemConnection curtain;

    // Setup Connections (Assumed COM1 and COM2 for simulation pair)
    ac.setComPort(1); 
    ac.openConnection();
    
    curtain.setComPort(2);
    curtain.openConnection();

    int choice = 0;
    while (choice != 3) {
        clearScreen();
        //  Main Menu
        cout << "MAIN MENU" << endl;
        cout << "1. Air Conditioner" << endl;
        cout << "2. Curtain Control" << endl;
        cout << "3. Exit" << endl;
        cout << "Choice: ";
        cin >> choice;

        if (choice == 1) {
            int subChoice = 0;
            while (subChoice != 2) {
                ac.update(); // Fetch latest data
                clearScreen();
                // [cite: 773] AC Info Screen
                cout << "Home Ambient Temperature: " << ac.getAmbientTemp() << " C" << endl;
                cout << "Home Desired Temperature: " << ac.getDesiredTemp() << " C" << endl;
                cout << "Fan Speed: " << ac.getFanSpeed() << " rps" << endl;
                cout << "-------------------------" << endl;
                
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