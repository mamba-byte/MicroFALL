#include <iostream>
#include <string>
#include <vector>
#include <cstdlib> // Rastgele sayı üretimi için
#include <ctime>   // Zaman fonksiyonları için
#include <thread>  // Bekleme (sleep) için
#include <chrono>

// TEST_MODE true ise gerçek seri port yerine sanal veri üretir.
const bool TEST_MODE = true; 

using namespace std;

// --- 1. MOCK SERIAL CLASS (Sanal Seri Port) ---
// Gerçek sistemde buraya Windows.h veya bir Serial kütüphanesi entegre edilir.
class MockSerial {
public:
    string portName;
    int baudRate;
    bool connected;

    MockSerial() : connected(false) {}

    bool open(string port, int baud) {
        portName = port;
        baudRate = baud;
        connected = true;
        if (TEST_MODE) cout << "[TEST] " << port << " portuna " << baud << " ile baglanildi.\n";
        return true;
    }

    void close() {
        connected = false;
        if (TEST_MODE) cout << "[TEST] Baglanti kapatildi.\n";
    }

    void writeByte(unsigned char data) {
        // Gerçekte burada WriteFile olurdu.
        // Test için sadece logluyoruz (İsterseniz yorum satırı yapın)
        // cout << "[Giden Byte]: " << (int)data << endl; 
    }

    unsigned char readByte() {
        // Gerçekte burada ReadFile olurdu.
        // Test için rastgele bir sayı döndürüyoruz (0-99 arası)
        return (unsigned char)(rand() % 100);
    }
};

// --- 2.3 API CLASSES (UML FIGURE 17) ---

// Base Class
class HomeAutomationSystemConnection {
protected:
    MockSerial serialPort;
    int comPortNumber; // Basitlik için int tutuyoruz, string "COMx" de olabilir
    int baudRate;

public:
    HomeAutomationSystemConnection() {
        comPortNumber = 1;
        baudRate = 9600;
    }

    void setComPort(int port) {
        comPortNumber = port;
    }

    void setBaudRate(int rate) {
        baudRate = rate;
    }

    bool open() {
        string pName = "COM" + to_string(comPortNumber);
        return serialPort.open(pName, baudRate);
    }

    bool close() {
        serialPort.close();
        return true;
    }

    virtual void update() = 0; // Pure virtual, alt sınıflar dolduracak
    
    // Yardımcı getter
    int getPortNum() { return comPortNumber; }
    int getBaud() { return baudRate; }
};

// Board #1 Class
class AirConditionerSystemConnection : public HomeAutomationSystemConnection {
private:
    float desiredTemperature;
    float ambientTemperature;
    int fanSpeed;

public:
    AirConditionerSystemConnection() {
        desiredTemperature = 0.0;
        ambientTemperature = 0.0;
        fanSpeed = 0;
    }

    // Dokuman Sayfa 16'daki Tabloya gore verileri ceker [cite: 675]
    void update() override {
        if (!serialPort.connected) return;

        // 1. Ortam Sicakligi (Low ve High Byte)
        serialPort.writeByte(0x03); // İstek gönder
        int amb_low = serialPort.readByte();
        serialPort.writeByte(0x04);
        int amb_high = serialPort.readByte();
        ambientTemperature = amb_high + (amb_low / 10.0f); // Örnek birleştirme

        // 2. Fan Hizi
        serialPort.writeByte(0x05);
        fanSpeed = serialPort.readByte(); // Doğrudan rps

        // 3. Istenen Sicaklik (Okuma)
        serialPort.writeByte(0x01);
        int des_low = serialPort.readByte();
        serialPort.writeByte(0x02);
        int des_high = serialPort.readByte();
        desiredTemperature = des_high + (des_low / 10.0f);
    }

    // Dokuman Sayfa 16 - Set Desired Temp [cite: 675]
    bool setDesiredTemp(float temp) {
        if (!serialPort.connected) return false;

        int high = (int)temp;
        int low = (int)((temp - high) * 10);

        // Format: 10xxxxxx (Low) ve 11xxxxxx (High)
        unsigned char cmd_low = 0b10000000 | (low & 0x3F);
        unsigned char cmd_high = 0b11000000 | (high & 0x3F);

        serialPort.writeByte(cmd_low);
        // Çok hızlı göndermemek için ufak bir bekleme gerekebilir
        this_thread::sleep_for(chrono::milliseconds(50)); 
        serialPort.writeByte(cmd_high);

        desiredTemperature = temp; // Lokal değişkeni de güncelle
        return true;
    }

    float getAmbientTemp() { return ambientTemperature; }
    int getFanSpeed() { return fanSpeed; }
    float getDesiredTemp() { return desiredTemperature; }
};

// Board #2 Class
class CurtainControlSystemConnection : public HomeAutomationSystemConnection {
private:
    float curtainStatus;
    float outdoorTemperature;
    float outdoorPressure;
    double lightIntensity;

public:
    CurtainControlSystemConnection() {
        curtainStatus = 0.0;
        outdoorTemperature = 0.0;
        outdoorPressure = 0.0;
        lightIntensity = 0.0;
    }

    // Dokuman Sayfa 19'daki Tabloya gore verileri ceker [cite: 719]
    void update() override {
        if (!serialPort.connected) return;

        // 1. Dis Sicaklik
        serialPort.writeByte(0x03);
        int out_low = serialPort.readByte();
        serialPort.writeByte(0x04);
        int out_high = serialPort.readByte();
        outdoorTemperature = out_high + (out_low / 10.0f);

        // 2. Perde Durumu
        // Burada sadece high byte örneği yapıyoruz, dokümanda fractional da var
        serialPort.writeByte(0x02); 
        curtainStatus = (float)serialPort.readByte(); 

        // 3. Basinc
        serialPort.writeByte(0x06);
        outdoorPressure = (float)serialPort.readByte() * 10; // Örnek ölçekleme

        // 4. Isik Siddeti
        serialPort.writeByte(0x08);
        lightIntensity = (double)serialPort.readByte() * 10; 
    }

    // Dokuman Sayfa 19 - Set Curtain Status [cite: 719]
    bool setCurtainStatus(float status) {
        if (!serialPort.connected) return false;

        int val = (int)status;
        
        // Format: 10xxxxxx ve 11xxxxxx
        unsigned char cmd_low = 0b10000000 | (0 & 0x3F); // Ondalık kısım 0 varsayıldı
        unsigned char cmd_high = 0b11000000 | (val & 0x3F);

        serialPort.writeByte(cmd_low);
        this_thread::sleep_for(chrono::milliseconds(50));
        serialPort.writeByte(cmd_high);

        curtainStatus = status;
        return true;
    }

    float getOutdoorTemp() { return outdoorTemperature; }
    float getOutdoorPress() { return outdoorPressure; }
    double getLightIntensity() { return lightIntensity; }
    float getCurtainStatus() { return curtainStatus; }
};

// --- 2.4 APPLICATION MENUS (FIGURE 18) ---

void clearScreen() {
    // Windows için "cls", Linux/Mac için "clear"
    system("cls"); 
}

void airConditionerMenu(AirConditionerSystemConnection &ac) {
    int choice = 0;
    while (true) {
        ac.update(); // Verileri yenile
        clearScreen();
        cout << "--- AIR CONDITIONER ---\n";
        cout << "Home Ambient Temperature: " << ac.getAmbientTemp() << " C\n";
        cout << "Home Desired Temperature: " << ac.getDesiredTemp() << " C\n";
        cout << "Fan Speed: " << ac.getFanSpeed() << " rps\n";
        cout << "-----------------------\n";
        cout << "Connection Port: COM" << ac.getPortNum() << "\n";
        cout << "Connection Baudrate: " << ac.getBaud() << "\n";
        cout << "-----------------------\n";
        cout << "MENU\n";
        cout << "1. Enter the desired temperature\n";
        cout << "2. Return\n";
        cout << "Choice: ";
        cin >> choice;

        if (choice == 1) {
            float newTemp;
            cout << "Enter Desired Temp: ";
            cin >> newTemp;
            ac.setDesiredTemp(newTemp);
            cout << "Veri gonderiliyor...\n";
            this_thread::sleep_for(chrono::seconds(1));
        } else if (choice == 2) {
            break;
        }
    }
}

void curtainMenu(CurtainControlSystemConnection &cc) {
    int choice = 0;
    while (true) {
        cc.update(); // Verileri yenile
        clearScreen();
        cout << "--- CURTAIN CONTROL ---\n";
        cout << "Outdoor Temperature: " << cc.getOutdoorTemp() << " C\n";
        cout << "Outdoor Pressure: " << cc.getOutdoorPress() << " hPa\n";
        cout << "Curtain Status: " << cc.getCurtainStatus() << " %\n";
        cout << "Light Intensity: " << cc.getLightIntensity() << " Lux\n";
        cout << "-----------------------\n";
        cout << "Connection Port: COM" << cc.getPortNum() << "\n";
        cout << "Connection Baudrate: " << cc.getBaud() << "\n";
        cout << "-----------------------\n";
        cout << "MENU\n";
        cout << "1. Enter the desired curtain status\n";
        cout << "2. Return\n";
        cout << "Choice: ";
        cin >> choice;

        if (choice == 1) {
            float newStatus;
            cout << "Enter Desired Curtain (%): ";
            cin >> newStatus;
            cc.setCurtainStatus(newStatus);
            cout << "Veri gonderiliyor...\n";
            this_thread::sleep_for(chrono::seconds(1));
        } else if (choice == 2) {
            break;
        }
    }
}

int main() {
    srand(time(0)); // Rastgelelik icin seed

    AirConditionerSystemConnection acSystem;
    CurtainControlSystemConnection curtainSystem;

    // Ayarlar
    acSystem.setComPort(3);
    curtainSystem.setComPort(4);

    // Baglantilari Ac
    acSystem.open();
    curtainSystem.open();

    int choice = 0;
    while (true) {
        clearScreen();
        cout << "=== MAIN MENU ===\n";
        cout << "1. Air Conditioner\n";
        cout << "2. Curtain Control\n";
        cout << "3. Exit\n";
        cout << "Choice: ";
        cin >> choice;

        if (choice == 1) {
            airConditionerMenu(acSystem);
        } else if (choice == 2) {
            curtainMenu(curtainSystem);
        } else if (choice == 3) {
            cout << "Exiting...\n";
            acSystem.close();
            curtainSystem.close();
            break;
        }
    }

    return 0;
}