
#include <cstring>
#include <sys/time.h>

#include "log.h"


#include "VASTFMT.h"

#include "hidapi.h"

// VASTFM vendor/product
//HID\VID_0451&PID_2100&REV_0101&MI_02
const uint16_t _usVID=0x0451;  /*!< USB vendor ID. */
const uint16_t _usPID=0x2100;  /*!< USB product ID. */


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


VASTFMT::VASTFMT() : Si4713() {
    struct hid_device_info *phdi = nullptr;
    phdi = hid_enumerate(_usVID,_usPID);
    if (phdi == nullptr) {
        return;
    }
    phd = hid_open_path(phdi->path);
    hid_free_enumeration(phdi);
    phdi=nullptr;
    
    
    if (phd) {
        powerUp();
    }
}
VASTFMT::~VASTFMT() {
    if (phd) {
        hid_close(phd);
    }
}
bool VASTFMT::isOk() {
    return phd != nullptr;
    
}
bool VASTFMT::sendDeviceCommand(uint8_t cmd, bool ignoreFailures) {
    std::vector<uint8_t> out;
    return sendDeviceCommand(cmd, out, ignoreFailures);
}
bool VASTFMT::sendDeviceCommand(uint8_t cmd, std::vector<uint8_t> &dataOut, bool ignoreFailures) {
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
bool VASTFMT::sendSi4711Command(uint8_t cmd, const std::vector<uint8_t> &dataIn, std::vector<uint8_t> &dataOut, bool ignoreFailures) {
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
    printf("CMD:  %2x    Read: %d\n", cmd, r);
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
bool VASTFMT::setProperty(uint16_t prop, uint16_t val) {
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
bool VASTFMT::getProperty(uint16_t prop, uint16_t &val) {
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
void VASTFMT::powerUp() {
    sendDeviceCommand(RequestSi4711PowerUp, true);
}
void VASTFMT::powerDown() {
    sendDeviceCommand(RequestSi4711PowerDown, true);
}
void VASTFMT::reset() {
    sendDeviceCommand(RequestSi4711Reset, true);
}
std::string VASTFMT::getRev() {
    std::vector<uint8_t> out;
    sendSi4711Command(0x10, {0}, out);
    
    if (sendDeviceCommand(RequestCpuId, out)) {
        std::string rev = (char*)(&out[3]);
        std::string board = (char*)(&out[5 + rev.size()]);
        return board + " - " + rev;
    }
    return "";
}

std::string VASTFMT::getASQ() {
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
std::string VASTFMT::getTuneStatus() {
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
void VASTFMT::enableAudio() {
    sendDeviceCommand(RequestSi4711AudioEnable);
}
void VASTFMT::disableAudio() {
    sendDeviceCommand(RequestSi4711AudioDisable);
}
