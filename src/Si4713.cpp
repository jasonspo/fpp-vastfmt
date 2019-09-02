#include <cstring>
#include <sys/time.h>

#include "Si4713.h"
#include "util/I2CUtils.h"

#include "hidapi.h"
#include "log.h"
#include "bitstream.h"


#define PCRequestError      0x80
#define PCTransfer          0x02
#define RequestDone         0x80

//status bits
#define STATUS_BIT_STCINT  0x01
#define STATUS_BIT_ASQINT  0x02
#define STATUS_BIT_RDSINT  0x04
#define STATUS_BIT_RSQINT  0x08
#define STATUS_BIT_ERR     0x40
#define STATUS_BIT_CTS     0x80

// properties
#define SI4713_PROP_GPO_IEN  0x0001
#define SI4713_PROP_DIGITAL_INPUT_FORMAT  0x0101
#define SI4713_PROP_DIGITAL_INPUT_SAMPLE_RATE  0x0103
#define SI4713_PROP_REFCLK_FREQ  0x0201
#define SI4713_PROP_REFCLK_PRESCALE  0x0202
#define SI4713_PROP_TX_COMPONENT_ENABLE  0x2100
#define SI4713_PROP_TX_AUDIO_DEVIATION  0x2101
#define SI4713_PROP_TX_PILOT_DEVIATION  0x2102
#define SI4713_PROP_TX_RDS_DEVIATION  0x2103
#define SI4713_PROP_TX_LINE_LEVEL_INPUT_LEVEL  0x2104
#define SI4713_PROP_TX_LINE_INPUT_MUTE  0x2105
#define SI4713_PROP_TX_PREEMPHASIS  0x2106
#define SI4713_PROP_TX_PILOT_FREQUENCY  0x2107
#define SI4713_PROP_TX_ACOMP_ENABLE  0x2200
#define SI4713_PROP_TX_ACOMP_THRESHOLD  0x2201
#define SI4713_PROP_TX_ATTACK_TIME  0x2202
#define SI4713_PROP_TX_RELEASE_TIME  0x2203
#define SI4713_PROP_TX_ACOMP_GAIN  0x2204
#define SI4713_PROP_TX_LIMITER_RELEASE_TIME  0x2205
#define SI4713_PROP_TX_ASQ_INTERRUPT_SOURCE  0x2300
#define SI4713_PROP_TX_ASQ_LEVEL_LOW  0x2301
#define SI4713_PROP_TX_ASQ_DURATION_LOW  0x2302
#define SI4713_PROP_TX_ASQ_LEVEL_HIGH  0x2303
#define SI4713_PROP_TX_ASQ_DURATION_HIGH  0x2304
#define SI4713_PROP_TX_RDS_INTERRUPT_SOURCE 0x2C00
#define SI4713_PROP_TX_RDS_PI  0x2C01
#define SI4713_PROP_TX_RDS_PS_MIX  0x2C02
#define SI4713_PROP_TX_RDS_PS_MISC  0x2C03
#define SI4713_PROP_TX_RDS_PS_REPEAT_COUNT  0x2C04
#define SI4713_PROP_TX_RDS_MESSAGE_COUNT  0x2C05
#define SI4713_PROP_TX_RDS_PS_AF  0x2C06
#define SI4713_PROP_TX_RDS_FIFO_SIZE  0x2C07



//==================================================================
// FM Transmit Commands
//==================================================================
// TX_TUNE_FREQ
#define TX_TUNE_FREQ 0x30

// TX_TUNE_POWER
#define TX_TUNE_POWER 0x31

// TX_TUNE_MEASURE
#define TX_TUNE_MEASURE 0x32

// TX_TUNE_STATUS
#define TX_TUNE_STATUS           0x33
#define TX_TUNE_STATUS_IN_INTACK 0x01

// TX_ASQ_STATUS
#define TX_ASQ_STATUS             0x34
#define TX_ASQ_STATUS_IN_INTACK   0x01
#define TX_ASQ_STATUS_OUT_IALL    0x01
#define TX_ASQ_STATUS_OUT_IALH    0x02
#define TX_ASQ_STATUS_OUT_OVERMOD 0x04

// TX_RDS_BUFF
#define TX_RDS_BUFF               0x35
#define TX_RDS_BUFF_IN_INTACK     0x01
#define TX_RDS_BUFF_IN_MTBUFF     0x02
#define TX_RDS_BUFF_IN_LDBUFF     0x04
#define TX_RDS_BUFF_IN_FIFO       0x80
#define TX_RDS_BUFF_OUT_RDSPSXMIT 0x10
#define TX_RDS_BUFF_OUT_CBUFXMIT  0x08
#define TX_RDS_BUFF_OUT_FIFOXMIT  0x04
#define TX_RDS_BUFF_OUT_CBUFWRAP  0x02
#define TX_RDS_BUFF_OUT_FIFOMT    0x01

// TX_RDS_PS
#define TX_RDS_PS 0x36


enum {
    RequestNone = 0,
    RequestCpuId,
    RequestSi4711Reset,             //reset
    RequestSi4711Access,            //low level access
    RequestSi4711GetProp,           //medium level get prop
    RequestSi4711SetProp,           //medium level set prop
    RequestSi4711PowerStatus,
    RequestSi4711PowerUp,           //high level power up
    RequestSi4711PowerDown,         //high level power down
    RequestSi4711AudioEnable,       //this MUST BE!!! called after setting config to enable audio.
    RequestSi4711AudioDisable,
    RequestEepromSectionRead,
    RequestEepromSectionWrite,
    RequestSi4711AsqStatus,
    RequestSi4711TuneStatus,
    RequestUnknown
};
static const char *si471x_requests[] = {
    "NONE",
    "CPU ID",
    "Frontend Reset",
    "Frontend Access",
    "Get Property",
    "Set Property",
    "Frontend Power Status",
    "Frontend Power Up",
    "Frontend Power Down",
    "Frontend Enable Audio",
    "Frontend Disable Audio",
    "EEPROM Read",
    "EEPROM Write",
    "unknown"
};
#define Si471xRequestStr(x) (x < RequestUnknown ? si471x_requests[x] : si471x_requests[RequestUnknown])

enum {
    SI4711_OK = 0,
    SI4711_TIMEOUT,
    SI4711_COMM_ERR,
    SI4711_BAD_ARG,
    SI4711_NOCTS,
    SI4711_ERROR_UNKNOWN
};
static const char *si471x_statuses[] = {
    "OK",
    "I2C Timeout",
    "I2C Communication Error",
    "Bad Argument",
    "No CTS!",
    "unknown"
};
#define Si471xStatusStr(x) (x < SI4711_ERROR_UNKNOWN ? si471x_statuses[x] : si471x_statuses[SI4711_ERROR_UNKNOWN])



class Si4713Connector {
public:
    Si4713Connector() {}
    virtual ~Si4713Connector() {}

    virtual bool isOk() = 0;
    
    virtual bool sendDeviceCommand(uint8_t cmd, std::vector<uint8_t> &dataOut, bool ignoreFailures) = 0;
    virtual bool sendSi4711Command(uint8_t cmd, const std::vector<uint8_t> &dataIn, std::vector<uint8_t> &dataOut, bool ignoreFailures) = 0;
    
    virtual bool setProperty(uint16_t prop, uint16_t val) = 0;
    virtual bool getProperty(uint16_t prop, uint16_t &val) = 0;

};

class USBSi4713Connector : public Si4713Connector {
public:
    USBSi4713Connector(uint16_t _usVID, uint16_t _usPID) {
        struct hid_device_info *phdi = nullptr;
        phdi = hid_enumerate(_usVID,_usPID);
        if (phdi == nullptr) {
            return;
        }
        phd = hid_open_path(phdi->path);
        hid_free_enumeration(phdi);
        phdi=nullptr;
    }
    virtual ~USBSi4713Connector() {
        if (phd) {
            hid_close(phd);
        }
    }
    virtual bool isOk() override { return phd != nullptr; }
    virtual bool sendDeviceCommand(uint8_t cmd, std::vector<uint8_t> &dataOut, bool ignoreFailures) {
        unsigned char aucBufIn[43];
        unsigned char aucBufOut[43];
        memset(aucBufOut, 0x00, 43); // Clear out the response buffer
        memset(aucBufIn, 0xCC, 43); // Clear out the response buffer

        /* Send a BL Query Command */
        aucBufOut[0] = 0; // Report ID, ignored
        aucBufOut[1] = PCTransfer;
        aucBufOut[2] = cmd;
        
        hid_write(phd, aucBufOut, 43);
        int r = hid_read(phd, aucBufIn, 43);

        if (r < 2) {
            LogWarn(VB_PLUGIN, "Si4713/USB: not enough data\n");
            return false;
        }
        if (!ignoreFailures && aucBufIn[0] & PCRequestError) {
            LogWarn(VB_PLUGIN, "Si4713/USB: request error\n");
            return false;
        }
        if (!ignoreFailures && !(aucBufIn[0] & PCTransfer)) {
            LogWarn(VB_PLUGIN, "Si4713/USB: Transfer error.\n");
            return false;
        }
        if (ignoreFailures || aucBufIn[1] == (cmd|RequestDone)) {
            dataOut.resize(r - 2);
            memcpy(&dataOut[0], &aucBufIn[2], r - 2);
            return true;
        } else {
            LogWarn(VB_PLUGIN, "Si4713/USB: Request not done.\n");
        }
        return false;
    }
    virtual bool sendSi4711Command(uint8_t cmd, const std::vector<uint8_t> &dataIn, std::vector<uint8_t> &dataOut, bool ignoreFailures) override {
        unsigned char aucBufIn[43];
        unsigned char aucBufOut[43];
        memset(aucBufOut, 0x00, 43); // Clear out the response buffer
        memset(aucBufIn, 0xCC, 43); // Clear out the response buffer
        
        /* Send a BL Query Command */
        aucBufOut[0] = 0; // Report ID, ignored
        aucBufOut[1] = PCTransfer;
        aucBufOut[2] = RequestSi4711Access;
        aucBufOut[3] = dataIn.size() + 1;
        aucBufOut[4] = cmd;
        memcpy(&aucBufOut[5], &dataIn[0], dataIn.size());

        hid_write(phd, aucBufOut, 43);
        int r = hid_read(phd, aucBufIn, 42);
        /*
        for (int x = 0; x < 25; x++) {
            printf("%x ", aucBufIn[x]);
        }
        printf("\n");
        */
        if (!ignoreFailures && aucBufIn[2] != 1) {
            LogWarn(VB_PLUGIN, "Si4711/USB: command failed (%d): %s, returned (FALSE)\n", aucBufIn[2], Si471xStatusStr(aucBufIn[2]));
            //sometimes it timeoutes, but the CTS=0, so we must re-read this byte again.
            //return false;
        }
        
        if (!ignoreFailures && aucBufIn[3]!=SI4711_OK) {
            LogWarn(VB_PLUGIN, "Si4711/USB: I2C_READ failed (%d): %s\n", aucBufIn[3], Si471xStatusStr(aucBufIn[3]));
            return false;
        }
        
        if(!ignoreFailures && aucBufIn[4] > 16) {
            LogWarn(VB_PLUGIN, "Si4711/USB: I2C_READ failed, too much bytes received: %d\n", aucBufIn[4]);
            return false;
        }
        
        //LogDebug(VB_PLUGIN, "Si4711/USB: Volume: %d.%d (deviation: %d)\n", (int) aucBufIn[21], (int ) aucBufIn[22], (int ) (aucBufIn[23] << 8 | aucBufIn[24]));
        //printf("Si4711/USB: Volume: %d.%d (deviation: %d)\n", (int ) aucBufIn[21], (int ) aucBufIn[22], (int ) (aucBufIn[23] << 8 | aucBufIn[24]));
        //printf("datasize: %d\n", aucBufIn[4]);
        int sz = 16; // aucBufIn[4]
        dataOut.resize(sz);
        memcpy(&dataOut[0], &aucBufIn[5], sz);
        return true;
    }
    virtual bool setProperty(uint16_t prop, uint16_t val)  override {
        unsigned char aucBufIn[43];
        unsigned char aucBufOut[43];
        memset(aucBufOut, 0x00, 43); // Clear out the response buffer
        memset(aucBufIn, 0xCC, 43); // Clear out the response buffer
        aucBufOut[0] = 0x00;            //report number, would be unused!
        aucBufOut[1] = PCTransfer;      //
        aucBufOut[2] = RequestSi4711SetProp;
        aucBufOut[3] = prop >> 8;
        aucBufOut[4] = prop;
        aucBufOut[5] = val >> 8;
        aucBufOut[6] = val;
        hid_write(phd, aucBufOut, 43);
        hid_read(phd, aucBufIn, 42);
        
        if (aucBufIn[0] & PCRequestError) {
            LogWarn(VB_PLUGIN, "Si4713/USB: request error for property %X.\n", prop);
            return false;
        }
        
        if (!(aucBufIn[1] & RequestDone)) {
            LogWarn(VB_PLUGIN, "Si4713/USB: request is not done!\n");
            return false;
        }
        
        if (aucBufIn[8]!=SI4711_OK) {
            LogWarn(VB_PLUGIN, "Si4713/USB: Device request \"%s\" failed (%d): %s\n", Si471xRequestStr(RequestSi4711SetProp), aucBufIn[2], Si471xStatusStr(aucBufIn[2]));
            return false;
        }
        
        if (aucBufIn[7] & STATUS_BIT_ERR) {
            LogWarn(VB_PLUGIN, "Si4713/USB: Answers: Error setting property 0x%04x to \"0x%04x\".\n", prop, val);
            return false;
        }
        
        if (!(aucBufIn[7] & STATUS_BIT_CTS)) {
            LogWarn(VB_PLUGIN, "Si4713/USB: Answers: NO CTS! When setting property 0x%04x.\n", prop);
            return false;
        }
        
        if (aucBufIn[6] != 1) {
            LogWarn(VB_PLUGIN, "Si4713/USB: Device request \"%s\" failed (%02x): %s (false!)\n", Si471xRequestStr(RequestSi4711SetProp), aucBufIn[6]);
            return false;
        }
        return true;
    }
    virtual bool getProperty(uint16_t prop, uint16_t &val) override {
        unsigned char aucBufIn[43];
        unsigned char aucBufOut[43];
        memset(aucBufOut, 0x00, 43); // Clear out the response buffer
        memset(aucBufIn, 0xCC, 43); // Clear out the response buffer
        aucBufOut[0] = 0x00;            //report number, would be unused!
        aucBufOut[1] = PCTransfer;      //
        aucBufOut[2] = RequestSi4711GetProp;
        aucBufOut[3] = prop >> 8;
        aucBufOut[4] = prop;
        hid_write(phd, aucBufOut, 43);
        hid_read(phd, aucBufIn, 42);

        if (aucBufIn[0] & PCRequestError) {
            LogWarn(VB_PLUGIN, "Si4713/USB: request error for property %X.\n", prop);
            return false;
        }
        
        if (!(aucBufIn[1] & RequestDone)) {
            LogWarn(VB_PLUGIN, "Si4713/USB: request is not done!\n");
            return false;
        }
        
        if (aucBufIn[8]!=SI4711_OK) {
            LogWarn(VB_PLUGIN, "Si4713/USB: Device request \"%s\" failed (%d): %s\n", Si471xRequestStr(RequestSi4711SetProp), aucBufIn[2], Si471xStatusStr(aucBufIn[2]));
            return false;
        }
        
        if (aucBufIn[7] & STATUS_BIT_ERR) {
            LogWarn(VB_PLUGIN, "Si4713/USB: Answers: Error setting property 0x%04x to \"0x%04x\".\n", prop, val);
            return false;
        }
        
        if (!(aucBufIn[7] & STATUS_BIT_CTS)) {
            LogWarn(VB_PLUGIN, "Si4713/USB: Answers: NO CTS! When setting property 0x%04x.\n", prop);
            return false;
        }
        
        if (aucBufIn[6] != 1) {
            LogWarn(VB_PLUGIN, "Si4713/USB: Device request \"%s\" failed (%02x): %s (false!)\n", Si471xRequestStr(RequestSi4711SetProp), aucBufIn[6]);
            return false;
        }
        val = (int16_t ) ((aucBufIn[4] & 0x00FF) << 8) | aucBufIn[5];
        return true;
    }


    hid_device *phd = nullptr;
};


Si4713::Si4713(uint16_t _usVID, uint16_t _usPID) {
    connector = new USBSi4713Connector(_usVID, _usPID);
    if (isOk()) {
        Init();
    }
}


Si4713::~Si4713() {
    if (connector) {
        delete connector;
    }
}

bool Si4713::isOk() {
    return connector && connector->isOk();
}


void Si4713::Init() {
    std::string rev = getRev();
    //setProperty(SI4713_PROP_REFCLK_FREQ, 32768); // crystal is 32.768
    setProperty(SI4713_PROP_TX_PREEMPHASIS, isEUPremphasis ? 1 : 0); // 75uS pre-emph (default for US)
    setProperty(SI4713_PROP_TX_ACOMP_ENABLE, 0x03); // turn on limiter and AGC
    
    setProperty(SI4713_PROP_TX_ACOMP_THRESHOLD, 0x10000-15); // -15 dBFS
    setProperty(SI4713_PROP_TX_ATTACK_TIME, 0); // 0.5 ms
    setProperty(SI4713_PROP_TX_RELEASE_TIME, 4); // 1000 ms
    setProperty(SI4713_PROP_TX_ACOMP_GAIN, 5); // dB
    
    powerUp();
}
bool Si4713::sendDeviceCommand(uint8_t cmd, bool ignoreFailures) {
    std::vector<uint8_t> out;
    return connector->sendDeviceCommand(cmd, out, ignoreFailures);
}
bool Si4713::sendDeviceCommand(uint8_t cmd, std::vector<uint8_t> &out, bool ignoreFailures) {
    return connector->sendDeviceCommand(cmd, out, ignoreFailures);
}
bool Si4713::setProperty(uint16_t prop, uint16_t val) {
    return connector->setProperty(prop, val);
}
bool Si4713::getProperty(uint16_t prop, uint16_t &val) {
    return connector->getProperty(prop, val);
}

bool Si4713::sendSi4711Command(uint8_t cmd, const std::vector<uint8_t> &data, bool ignoreFailures) {
    std::vector<uint8_t> out;
    return connector->sendSi4711Command(cmd, data, out, ignoreFailures);
}
bool Si4713::sendSi4711Command(uint8_t cmd, const std::vector<uint8_t> &data, std::vector<uint8_t> &out, bool ignoreFailures) {
    return connector->sendSi4711Command(cmd, data, out, ignoreFailures);
}

void Si4713::powerUp() {
    sendDeviceCommand(RequestSi4711PowerUp, true);
}
void Si4713::powerDown() {
    sendDeviceCommand(RequestSi4711PowerDown, true);
}
void Si4713::reset() {
    sendDeviceCommand(RequestSi4711Reset, true);
}

std::string Si4713::getRev() {
    std::vector<uint8_t> out;
    sendSi4711Command(0x10, {0}, out);
    
    if (sendDeviceCommand(RequestCpuId, out)) {
        std::string rev = (char*)(&out[3]);
        std::string board = (char*)(&out[5 + rev.size()]);
        return board + " - " + rev;
    }
    return "";
}

void Si4713::setFrequency(int frequency) {
    uint8_t ft = frequency>>8;
    uint8_t fl = 0x00FF & frequency;
    sendSi4711Command(TX_TUNE_FREQ, {0x00, ft, fl});
}
void Si4713::setTXPower(int power, double antCap) {
    uint8_t rfcap0 = antCap/0.25;
    uint8_t p = power & 0xff;
    sendSi4711Command(TX_TUNE_POWER, {0x00, 0x00, p, rfcap0});
}


std::string Si4713::getASQ() {
    std::vector<uint8_t> out;
    if (sendDeviceCommand(RequestSi4711AsqStatus, out)) {
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
    return "";
}
std::string Si4713::getTuneStatus() {
    std::vector<uint8_t> out;
    if (sendDeviceCommand(RequestSi4711TuneStatus, out)) {
        int currFreq = out[1] << 8 | out[2];
        int currdBuV = out[3];
        int currAntCap = out[4];
        
        float f = currFreq / 100.0f;
        float cap = ((float)currAntCap) * 0.25f;
        
        std::string r = "Freq:" + std::to_string(f)
            + " MHz - Power:" + std::to_string(currdBuV)
            + "dBuV - ANTcap:" + std::to_string(cap);
        return r;
    }
    return "";
}

void Si4713::enableAudio() {
    sendDeviceCommand(RequestSi4711AudioEnable);
}
void Si4713::disableAudio() {
    sendDeviceCommand(RequestSi4711AudioDisable);
}


void Si4713::beginRDS() {
    //66.25KHz (default is 68.25)
    setProperty(SI4713_PROP_TX_AUDIO_DEVIATION, 6625);
    // 2KHz (default)
    setProperty(SI4713_PROP_TX_RDS_DEVIATION, 200);
    
    //RDS IRQ
    setProperty(SI4713_PROP_TX_RDS_INTERRUPT_SOURCE, 0x0001);
    // program identifier
    setProperty(SI4713_PROP_TX_RDS_PI, 0x40A7);
    // 50% mix (default)
    setProperty(SI4713_PROP_TX_RDS_PS_MIX, 0x03);
    //  RDSD0 & RDSMS (default)
    setProperty(SI4713_PROP_TX_RDS_PS_MISC, 0x1008  | (pty << 5));
    // 3 repeats (default)
    setProperty(SI4713_PROP_TX_RDS_PS_REPEAT_COUNT, 3);
    
    setProperty(SI4713_PROP_TX_RDS_MESSAGE_COUNT, 1);
    setProperty(SI4713_PROP_TX_RDS_PS_AF, 0xE0E0); // no AF
    setProperty(SI4713_PROP_TX_RDS_FIFO_SIZE, 7);
    
    setProperty(SI4713_PROP_TX_COMPONENT_ENABLE, 0x0007);
}
void Si4713::setRDSStation(const std::vector<std::string> &station) {
    uint8_t buf[8];
    
    if (lastStation == station) {
        return;
    }
    lastStation = station;

    setProperty(SI4713_PROP_TX_RDS_MESSAGE_COUNT, station.size());
    uint8_t idx = 0;
    for (auto &a : station) {
        int sl = a.size();
        memset(buf, ' ', 8);
        if (sl > 8) {
            sl = 8;
        }
        for (int x = 0; x < sl; x++) {
            buf[x] = a[x];
        }
        for (uint8_t i = 0; i < 2; i++) {
            sendSi4711Command(TX_RDS_PS, {idx, buf[i*4], buf[(i*4)+1], buf[(i*4)+2], buf[(i*4)+3]});
            idx++;
        }
    }
}
void Si4713::setRDSBuffer(const std::string &station) {
    if (lastRDS == station) {
        return;
    }
    lastRDS = station;
    uint8_t buf[64];
    memset(buf, ' ', 64);
    
    
    int sl = station.size();
    if (sl > 64) {
        sl = 64;
    }
    for (int x = 0; x < sl; x++) {
        buf[x] = station[x];
    }
    for (int x = (sl-1); x > 0; --x) {
        if (buf[x] == ' ') {
            sl--;
        } else {
            break;
        }
    }

    std::vector<uint8_t> out;
    sendSi4711Command(TX_RDS_BUFF, {TX_RDS_BUFF_IN_MTBUFF, 0, 0, 0, 0, 0, 0}, out);
    if (station.size() != 0) {
        int count = (sl + 3) / 4;
        //printf("%d,   %s\n", count, station.c_str());
        for (uint8_t i = 0; i < count; i++) {
            uint8_t sb = TX_RDS_BUFF_IN_LDBUFF;
            if (i == 0) {
                sb |= TX_RDS_BUFF_IN_MTBUFF;
            }
            sendSi4711Command(TX_RDS_BUFF, {sb, 0x20, i, buf[i*4], buf[(i*4)+1], buf[(i*4)+2], buf[(i*4)+3]}, out);
            
            if (out.size() < 8) {
                out.resize(8);
            }
            //printf("bu out %d:  %2X %2X %2X %2X %2X %2X %2X %2X\n", i, out[0], out[1], out[2], out[3], out[4], out[5], out[6], out[7]);
        }
        //sendRtPlusInfo(1, 0, 10, 4, 11, sl - 11);
    }
    sendTimestamp();

    setProperty(SI4713_PROP_TX_COMPONENT_ENABLE, 0x0007);
    sendSi4711Command(0x14, {}, out);
    //printf("0x14 out:  %2X %2X %2X %2X %2X %2X %2X %2X\n", out[0], out[1], out[2], out[3], out[4], out[5], out[6], out[7]);
    sendSi4711Command(TX_RDS_BUFF, {TX_RDS_BUFF_IN_INTACK, 0, 0, 0, 0, 0, 0}, out);
    //printf("int ack out:  %2X %2X %2X %2X %2X %2X %2X %2X\n", out[0], out[1], out[2], out[3], out[4], out[5], out[6], out[7]);
}


static int rtplus_toggle_bit = 1; //XXX:used to save RT+ toggle bit value
#define RTPLUS_GROUP_ID 0b1011
void Si4713::sendRtPlusInfo(int content1, int content1_pos, int content1_len,
                            int content2, int content2_pos, int content2_len) {
    uint8_t buff[16];
    char msg[6];

    if (content1_len || content2_len) {
    
    
        memset (msg, 0x00, 6);
        bitstream_t *bs = nullptr;
        bs_create(&bs);
        bs_attach(bs, (uint8_t *) msg, 6);
        
        bs_put(bs, 0x00, 3);                    //seek to start pos
        rtplus_toggle_bit = !rtplus_toggle_bit; //if we using RT+, update information!
        bs_put(bs, rtplus_toggle_bit, 1);       //Item toggle bit
        bs_put(bs, 1, 1);                       //Item running bit
    
        if (content1_len) {
            bs_put(bs, content1, 6);        //RT content type 1
            bs_put(bs, content1_pos, 6);    //start marker 1
            bs_put(bs, content1_len, 6);    //length marker 1
        }
    
        if (content2_len) {
            bs_put(bs, content2, 6);        //RT content type 2
            bs_put(bs, content2_pos, 6);    //start marker 2
            bs_put(bs, content2_len, 5);    //length marker 2 (5 bits!)
        }
    
        std::vector<uint8_t> out;
        sendSi4711Command(TX_RDS_BUFF, {TX_RDS_BUFF_IN_LDBUFF,
                        RTPLUS_GROUP_ID << 4,
            msg[0], msg[1], msg[2], msg[3], msg[4]}, out);
        printf("RTPLUS Settings:  %2X %2X %2X %2X %2X %2X %2X %2X\n", out[0], out[1], out[2], out[3], out[4], out[5], out[6], out[7]);

        //send RT+ announces
        //  FmRadioController::HandleRDSData
        //  FmRadioRDSParser::ParseRDSData  RDSGroup:6
        //  !!TEST FmRadioRDSParser::RT+ GROUP_TYPE_3A : RT+ Message
        //  !!TEST FmRadioRDSParser::RT+ Message RT+ flag : 0 0, RT+ AID : 4bd7 19415 (flag : 0 0)
        //  !!TEST FmRadioRDSParser::RT+ Message application group type code : 16 22
        sendSi4711Command(TX_RDS_BUFF, {TX_RDS_BUFF_IN_LDBUFF,
            0x30, //RDS GROUP: 3A
            RTPLUS_GROUP_ID << 1, //we are describing RT+ group 11A
                        //RTPLUS_GROUP_ID, //we are describing RT+ group 11A
            0, //xxx.y.zzzz
               //xxx  - rfu
               //y    - cb flag (for template number)
               //zzzz - for rds server needs.
            0, //template id=0
            0x4B, 0xD7 //it's RT+
        }, out);
        printf("RTPLUS Group:  %2X %2X %2X %2X %2X %2X %2X %2X\n", out[0], out[1], out[2], out[3], out[4], out[5], out[6], out[7]);
    }
}


void Si4713::sendTimestamp() {
    uint32_t MJD;
    int y, m, d, k;
    struct tm  *ltm;
    int8_t offset;
    struct timezone tz;
    struct timeval tv;
    gettimeofday( &tv, &tz);
    ltm=localtime(&tv.tv_sec);
    offset = tz.tz_minuteswest / 30;
    
    y = ltm->tm_year;
    m = ltm->tm_mon + 1;
    d = ltm->tm_mday;
    k = 0;
    if((m == 1) || (m == 2)) {
        k = 1;
    }
    
    MJD = 14956 + d + ((int) ((y - k) * 365.25)) + ((int) ((m + 1 + k * 12) * 30.6001));
    
    if (offset > 0) {
        offset = (abs(offset) & 0x1F) | 0x20;
    } else {
        offset = abs(offset) & 0x1F;
    }
    
    uint8_t sb = TX_RDS_BUFF_IN_LDBUFF | TX_RDS_BUFF_IN_FIFO;
    std::vector<uint8_t> out;
    
    uint8_t arg1 = (MJD >> 15);
    uint8_t arg2 = (MJD >> 7);
    uint8_t arg3 = (MJD << 1) | ((ltm->tm_hour & 0x1F)>> 4);
    uint8_t arg4 = ((ltm->tm_hour & 0x1F)<< 4)|((ltm->tm_min & 0x3F)>> 2);
    uint8_t arg5 = ((ltm->tm_min & 0x3F)<< 6)|offset;
    sendSi4711Command(TX_RDS_BUFF,{sb, 0x40, arg1, arg2, arg3, arg4, arg5}, out);
    //printf("ts out:  %2X %2X %2X %2X %2X %2X %2X %2X\n", out[0], out[1], out[2], out[3], out[4], out[5], out[6], out[7]);
}

