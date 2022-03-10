#ifndef __I2CSI4713__
#define __I2CSI4713__

#include "Si4713.h"

class I2CUtils;
class PinCapabilities;

class I2CSi4713 : public Si4713 {
public:
    I2CSi4713(const std::string &gpioPin);
    virtual ~I2CSi4713();
    
    
    virtual bool isOk() override;
    virtual std::string getRev() override;
    virtual void powerUp() override;
    virtual void powerDown() override;
    virtual void reset() override;
    virtual std::string getASQ() override;
    virtual std::string getTuneStatus() override;
    
    
protected:
    virtual bool sendSi4711Command(uint8_t cmd, const std::vector<uint8_t> &data, bool ignoreFailures = false) override;
    virtual bool sendSi4711Command(uint8_t cmd, const std::vector<uint8_t> &data, std::vector<uint8_t> &out, bool ignoreFailures = false) override;
    virtual bool setProperty(uint16_t prop, uint16_t val) override;
    
private:
    I2CUtils *i2c = nullptr;
    const PinCapabilities *resetPin = nullptr;
};

#endif
