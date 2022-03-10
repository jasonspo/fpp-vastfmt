#ifndef __VASTFMT__
#define __VASTFMT__

#include "Si4713.h"

struct hid_device_;

class VASTFMT : public Si4713 {
public:
    VASTFMT();
    virtual ~VASTFMT();
    
    
    virtual bool isOk() override;
    virtual std::string getRev() override;
    virtual void powerUp() override;
    virtual void powerDown() override;
    virtual void reset() override;
    virtual std::string getASQ() override;
    virtual std::string getTuneStatus() override;
    
    
    void enableAudio();
    void disableAudio();
protected:
    virtual bool sendSi4711Command(uint8_t cmd, const std::vector<uint8_t> &data, std::vector<uint8_t> &out, bool ignoreFailures = false) override;
    virtual bool setProperty(uint16_t prop, uint16_t val) override;

    bool getProperty(uint16_t prop, uint16_t &val);
    bool sendDeviceCommand(uint8_t cmd, bool ignoreFailures = false);
    bool sendDeviceCommand(uint8_t cmd, std::vector<uint8_t> &dataOut, bool ignoreFailures = false);

    
private:
    hid_device_ *phd = nullptr;
};


#endif
