
#include <unistd.h>
#include <cstring>
#include <sys/time.h>

#include "log.h"

#include "I2CSi4713.h"

#include "util/I2CUtils.h"
#include "util/GPIOUtils.h"

// Commands
#define SI4710_CMD_POWER_UP         0x01
#define SI4710_CMD_GET_REV          0x10
#define SI4710_CMD_POWER_DOWN       0x11
#define SI4710_CMD_SET_PROPERTY     0x12
#define SI4710_CMD_GET_PROPERTY     0x13
#define SI4710_CMD_GET_INT_STATUS   0x14
#define SI4710_CMD_PATCH_ARGS       0x15
#define SI4710_CMD_PATCH_DATA       0x16
#define SI4710_CMD_TX_TUNE_FREQ     0x30
#define SI4710_CMD_TX_TUNE_POWER    0x31
#define SI4710_CMD_TX_TUNE_MEASURE  0x32
#define SI4710_CMD_TX_TUNE_STATUS   0x33
#define SI4710_CMD_TX_ASQ_STATUS    0x34
#define SI4710_CMD_TX_RDS_BUFF      0x35
#define SI4710_CMD_TX_RDS_PS        0x36
#define SI4710_CMD_TX_AGC_OVERRIDE  0x48
#define SI4710_CMD_GPO_CTL          0x80
#define SI4710_CMD_GPO_SET          0x81


#define SI4713_PROP_REFCLK_FREQ   0x0201

#ifdef PLATFORM_BBB
#define I2CBUS 2
#else
#define I2CBUS 1
#endif

I2CSi4713::I2CSi4713(const std::string &gpioPin) {
    resetPin = PinCapabilities::getPinByName(gpioPin).ptr();
    if (resetPin == nullptr) {
        resetPin = PinCapabilities::getPinByGPIO(std::stoi(gpioPin)).ptr();
    }
    resetPin->configPin("gpio", "out");
    resetPin->setValue(1);
    usleep(200000);
    resetPin->setValue(0);
    usleep(200000);
    resetPin->setValue(1);
    usleep(300000);
    i2c = new I2CUtils(I2CBUS, 0x63);
    if (i2c->isOk()) {
        sendSi4711Command(SI4710_CMD_POWER_UP, {0x12, 0x50});
        usleep(500000);
        setProperty(SI4713_PROP_REFCLK_FREQ, 32768);
    } else {
        delete i2c;
        i2c = nullptr;
    }
}
I2CSi4713::~I2CSi4713() {
    
    if (i2c) {
        delete i2c;
    }
    
}


bool I2CSi4713::isOk() {
    return i2c != nullptr;
}
void I2CSi4713::powerUp() {
}
void I2CSi4713::powerDown() {
}
void I2CSi4713::reset() {
}


std::string I2CSi4713::getRev() {
    std::vector<uint8_t> response;
    response.resize(9);
    sendSi4711Command(SI4710_CMD_GET_REV, {0}, response);
    
    int pn = response[1];
    int fw = response[2] << 8 | response[3];
    int patch = response[4] << 8 | response[5];
    int cmp = response[6] << 8 | response[7];
    int chiprev = response[8];
    
    char buf[250];
    sprintf(buf, "Si4713 - pn: %X  fw: %X   patch: %X   cmp: %X    chiprev: %X", pn, fw, patch, cmp, chiprev);
    return buf;
}

std::string I2CSi4713::getASQ() {
    std::vector<uint8_t> out;
    out.resize(8);
    sendSi4711Command(SI4710_CMD_TX_ASQ_STATUS, {0x1}, out);
    std::string r = "ASQ Flags: ";
    r += std::to_string(out[1])  + " " +  std::to_string(out[2]) + " " +  std::to_string(out[3]);
    r += "  - InLevel:";
    int lev = out[4];
    if (lev > 127) {
        lev -= 256;
    }
    r += std::to_string(lev);
    r += " dBfs";
    return r;
}
std::string I2CSi4713::getTuneStatus() {
    std::vector<uint8_t> out;
    out.resize(8);
    sendSi4711Command(SI4710_CMD_TX_TUNE_STATUS, {0x1}, out);
    int currFreq = out[2] << 8 | out[3];
    int currdBuV = out[5];
    int currAntCap = out[6];
    
    float f = currFreq / 100.0f;
    
    std::string r = "Freq:" + std::to_string(f)
        + " MHz - Power:" + std::to_string(currdBuV)
        + "dBuV - ANTcap:" + std::to_string(currAntCap);
    return r;
}
bool I2CSi4713::sendSi4711Command(uint8_t cmd, const std::vector<uint8_t> &data, bool ignoreFailures) {
    //printf("Sending command %X    datasize: %d\n", cmd, data.size());
    int i = i2c->writeBlockData(cmd, &data[0], data.size());
    usleep(10000);
    if (ignoreFailures || i >= 0) {
        return true;
    }
    return false;
}

bool I2CSi4713::sendSi4711Command(uint8_t cmd, const std::vector<uint8_t> &data, std::vector<uint8_t> &out, bool ignoreFailures) {
    //printf("Sending command %X    datasize: %d     toRead:  %d\n", cmd, data.size(), out.size());
    int i = i2c->writeBlockData(cmd, &data[0], data.size());
    if (!ignoreFailures && i < 0) {
        return false;
    }
    usleep(10000);
    if (out.size()) {
        usleep(10000);
        i2c->readI2CBlockData(0x00, &out[0], out.size());
        //printf("    read  %d\n", i);
    }
    return false;
}


bool I2CSi4713::setProperty(uint16_t prop, uint16_t val) {
    //printf("Set property: %X  val: %X\n", prop, val);
    unsigned char aucBuf[20];
    memset(aucBuf, 0x00, 20);
    aucBuf[0] = 0x00;
    aucBuf[1] = prop >> 8;
    aucBuf[2] = prop;
    aucBuf[3] = val >> 8;
    aucBuf[4] = val;
    int i = i2c->writeBlockData(SI4710_CMD_SET_PROPERTY, aucBuf, 5) >= 0;
    if (i == -1) {
        printf("Failed property: %X  %X\n", prop, val);
    }
    usleep(1000);
    return i;
}
