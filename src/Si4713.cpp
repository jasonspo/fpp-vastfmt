#include "fpp-pch.h"
#include <sys/time.h>

#include "Si4713.h"
#include "util/I2CUtils.h"
#include "bitstream.h"


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



Si4713::Si4713() {
}


Si4713::~Si4713() {
}


void Si4713::Init() {
    std::string rev = getRev();
    //setProperty(SI4713_PROP_REFCLK_FREQ, 32768); // crystal is 32.768
    setProperty(SI4713_PROP_TX_PREEMPHASIS, isEUPremphasis ? 1 : 0); // 75uS pre-emph (default for US)
    
    uint16_t t = audioCompression ? 0x1 : 0;
    if (audioLimitter) {
        t |= 0x2;
    }
    setProperty(SI4713_PROP_TX_ACOMP_ENABLE, t); // turn on limiter and AGC
    setProperty(SI4713_PROP_TX_ACOMP_THRESHOLD, 0x10000 + audioCompressionThreshold);
    setProperty(SI4713_PROP_TX_ATTACK_TIME, 0); // 0.5 ms
    setProperty(SI4713_PROP_TX_RELEASE_TIME, 4); // 1000 ms
    setProperty(SI4713_PROP_TX_ACOMP_GAIN, audioGain); // dB
}


void Si4713::setFrequency(int frequency) {
    uint8_t ft = frequency>>8;
    uint8_t fl = 0x00FF & frequency;
    sendSi4711Command(TX_TUNE_FREQ, {0x00, ft, fl});
}
void Si4713::setTXPower(int power, double antCap) {
    uint8_t rfcap0 = antCap;
    if (rfcap0 > 191) {
        rfcap0 = 191;
    }
    uint8_t p = power & 0xff;
    sendSi4711Command(TX_TUNE_POWER, {0x00, 0x00, p, rfcap0});
}

bool Si4713::sendSi4711Command(uint8_t cmd, const std::vector<uint8_t> &data, bool ignoreFailures) {
    std::vector<uint8_t> out;
    return sendSi4711Command(cmd, data, out, ignoreFailures);
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
    int i = (0x1848 & 0xFB1F) | (pty << 5);
    uint16_t i2 = i;
    setProperty(SI4713_PROP_TX_RDS_PS_MISC, i);
    // 3 repeats (default)
    setProperty(SI4713_PROP_TX_RDS_PS_REPEAT_COUNT, 3);
    
    setProperty(SI4713_PROP_TX_RDS_MESSAGE_COUNT, 1);
    setProperty(SI4713_PROP_TX_RDS_PS_AF, 0xE0E0); // no AF
    setProperty(SI4713_PROP_TX_RDS_FIFO_SIZE, 0);
    
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
            sendSi4711Command(TX_RDS_PS, {idx, buf[i*4], buf[(i*4)+1], buf[(i*4)+2], buf[(i*4)+3], 0});
            idx++;
        }
    }
}
void Si4713::setRDSBuffer(const std::string &station,
                          int artistPos, int artistLen,
                          int titlePos, int titleLen) {
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

    std::vector<uint8_t> resp(6);
    if (station.size() != 0) {
        int count = (sl + 3) / 4;
        //printf("%d,   %s\n", count, station.c_str());
        for (uint8_t i = 0; i < count; i++) {
            uint8_t sb = TX_RDS_BUFF_IN_LDBUFF;
            if (i == 0) {
                sb |= TX_RDS_BUFF_IN_MTBUFF;
            }
            sendSi4711Command(TX_RDS_BUFF, {sb, 0x20, i, buf[i*4], buf[(i*4)+1], buf[(i*4)+2], buf[(i*4)+3], 0}, resp);
            LogExcess(VB_PLUGIN, "   res:  %2X %2X %2X %2X %2X %2X\n", resp[0], resp[1], resp[2], resp[3], resp[4], resp[5]);
        }
        uint8_t idx = count;
        sendSi4711Command(TX_RDS_BUFF, {TX_RDS_BUFF_IN_LDBUFF, 0x20, idx, 0x0d, 0x00, 0x00, 0x00, 0}, resp);
        LogExcess(VB_PLUGIN, "   res:  %2X %2X %2X %2X %2X %2X\n", resp[0], resp[1], resp[2], resp[3], resp[4], resp[5]);
        sendRtPlusInfo(1, titlePos, titleLen, 4, artistPos, artistLen);
    } else {
        sendSi4711Command(TX_RDS_BUFF, {TX_RDS_BUFF_IN_MTBUFF, 0, 0, 0, 0, 0, 0});
    }
    sendTimestamp();

    setProperty(SI4713_PROP_TX_COMPONENT_ENABLE, 0x0007);
    sendSi4711Command(0x14, {});
    sendSi4711Command(TX_RDS_BUFF, {TX_RDS_BUFF_IN_INTACK, 0, 0, 0, 0, 0, 0});
}


static int rtplus_toggle_bit = 1; //XXX:used to save RT+ toggle bit value
#define RTPLUS_GROUP_ID 0b1011
void Si4713::sendRtPlusInfo(int content1, int content1_pos, int content1_len,
                            int content2, int content2_pos, int content2_len) {
    uint8_t buff[16];
    char msg[6];
    std::vector<uint8_t> resp(6);

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
    
        sendSi4711Command(TX_RDS_BUFF, {TX_RDS_BUFF_IN_LDBUFF,
                        RTPLUS_GROUP_ID << 4,
                        msg[0], msg[1], msg[2], msg[3], msg[4]}, resp);
        LogExcess(VB_PLUGIN, "   res+:  %2X %2X %2X %2X %2X %2X\n", resp[0], resp[1], resp[2], resp[3], resp[4], resp[5]);

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
        }, resp);
        LogExcess(VB_PLUGIN, "   res+:  %2X %2X %2X %2X %2X %2X\n", resp[0], resp[1], resp[2], resp[3], resp[4], resp[5]);
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
    
    uint8_t sb = TX_RDS_BUFF_IN_LDBUFF;// | TX_RDS_BUFF_IN_FIFO;
    std::vector<uint8_t> out(6);
    
    uint8_t arg1 = (MJD >> 15);
    uint8_t arg2 = (MJD >> 7);
    uint8_t arg3 = (MJD << 1) | ((ltm->tm_hour & 0x1F)>> 4);
    uint8_t arg4 = ((ltm->tm_hour & 0x1F)<< 4)|((ltm->tm_min & 0x3F)>> 2);
    uint8_t arg5 = ((ltm->tm_min & 0x3F)<< 6)|offset;
    sendSi4711Command(TX_RDS_BUFF,{sb, 0x40, arg1, arg2, arg3, arg4, arg5}, out);
    LogExcess(VB_PLUGIN, "ts out:  %2X %2X %2X %2X %2X %2X\n", out[0], out[1], out[2], out[3], out[4], out[5]);
}

