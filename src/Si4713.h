#ifndef __SI4713__
#define __SI4713__

#include <stdint.h>
#include <vector>
#include <string>

class Si4713Connector;

class Si4713 {
public:
    Si4713(uint16_t _usVID, uint16_t _usPID); //usb

    virtual ~Si4713();
    
    bool isOk();
    bool init();
    
    std::string getRev();
    void powerUp();
    void powerDown();
    void reset();
    
    void setEUPreemphasis() {isEUPremphasis = true;}
    void setFrequency(int frequency); // freq * 100,  so 8790 for 87.9
    void setTXPower(int power, double antCap);
    
    std::string getASQ();
    std::string getTuneStatus();

    //RDS stuff
    void setPTY(int i) { pty = i;}
    void beginRDS();
    void setRDSStation(const std::vector<std::string> &station);
    void setRDSBuffer(const std::string &rds);
    void sendTimestamp();
    
    //USB audio
    void enableAudio();
    void disableAudio();
private:
    void Init();
    
    void sendRtPlusInfo(int content1, int content1_pos, int content1_len,
                        int content2, int content2_pos, int content2_len);
    
    bool sendDeviceCommand(uint8_t cmd, bool ignoreFailures = false);
    bool sendDeviceCommand(uint8_t cmd, std::vector<uint8_t> &out, bool ignoreFailures = false);
    bool sendSi4711Command(uint8_t cmd, const std::vector<uint8_t> &data, bool ignoreFailures = false);
    bool sendSi4711Command(uint8_t cmd, const std::vector<uint8_t> &data, std::vector<uint8_t> &out, bool ignoreFailures = false);
    bool setProperty(uint16_t prop, uint16_t val);
    bool getProperty(uint16_t prop, uint16_t &val);

    
    Si4713Connector * connector = nullptr;
    
    bool isEUPremphasis = false;
    int pty = 2;
    std::vector<std::string> lastStation;
    std::string lastRDS;
};

#endif
