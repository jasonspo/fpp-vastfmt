#include <string>
#include <vector>

#include <unistd.h>
#include <termios.h>

#include <httpserver.hpp>
#include "mediadetails.h"
#include "common.h"
#include "settings.h"
#include "Plugin.h"
#include "log.h"

#include "Si4713.h"


// VASTFM vendor/product
//HID\VID_0451&PID_2100&REV_0101&MI_02
const uint16_t _usVID=0x0451;  /*!< USB vendor ID. */
const uint16_t _usPID=0x2100;  /*!< USB product ID. */

static std::string padToNearest(std::string s, int l) {
    if (!s.empty()) {
        int n = 0;
        while (n < s.size()) {
            n += l;
        }
        if (n != s.size()) {
            size_t a = n - s.size();
            s.append(a, ' ');
        }
    }
    return s;
}
static void padTo(std::string &s, int l) {
    size_t n = l - s.size();
    if (n) {
        s.append(n, ' ');
    }
}

class FPPVastFMPlugin : public FPPPlugin {
public:
    bool enabled = true;
    FPPVastFMPlugin() : FPPPlugin("fpp-vastfmt") {
        setDefaultSettings();
        if (settings["Start"] == "FPPDStart") {
            startVast();
        }
    }
    virtual ~FPPVastFMPlugin() {
        if (si4713 != nullptr) {
            //si4713->powerDown();
            delete si4713;
            si4713 = nullptr;
        }
    }
    
    void startVast() {
        if (si4713 == nullptr) {
            si4713 = new Si4713(_usVID, _usPID);
            if (si4713->isOk()) {
                si4713->powerUp();
                
                std::string rev = si4713->getRev();
                LogInfo(VB_PLUGIN, "VAST-FMT: %s\n", rev.c_str());
                
                if (settings["Preemphasis"] == "50us") {
                    si4713->setEUPreemphasis();
                }
                
                float f = std::stoi(settings["AntCap"]);
                si4713->setTXPower(std::stoi(settings["Power"]), f);
                
                std::string freq = settings["Frequency"];
                f = std::stof(freq);
                f *= 100;
                
                si4713->setFrequency(f);
                si4713->setPTY(std::stoi(settings["Pty"]));
                
                std::string asq = si4713->getASQ();
                LogInfo(VB_PLUGIN, "VAST-FMT: %s\n", asq.c_str());
                
                std::string ts = si4713->getTuneStatus();
                LogInfo(VB_PLUGIN, "VAST-FMT: %s\n", ts.c_str());
                
                if (settings["EnableRDS"] == "True") {
                    si4713->beginRDS();
                    formatAndSendText(settings["StationText"], "", "", true);
                    formatAndSendText(settings["RDSTextText"], "", "", false);
                }
            }
            if (!si4713->isOk()) {
                delete si4713;
                si4713 = nullptr;
            }
        }
    }
    void stopVast() {
        if (si4713 != nullptr) {
            //si4713->powerDown();
            delete si4713;
            si4713 = nullptr;
        }
    }
    
    void formatAndSendText(const std::string &text, const std::string &artist, const std::string &title, bool station) {
        std::string output;
        for (int x = 0; x < text.length(); x++) {
            if (text[x] == '[') {
                if (artist == "" && title == "") {
                    while (text[x] != ']' && x < text.length()) {
                        x++;
                    }
                }
            } else if (text[x] == ']') {
                //nothing
            } else if (text[x] == '{') {
                const static std::string ARTIST = "{Artist}";
                const static std::string TITLE = "{Title}";
                std::string subs = text.substr(x);
                if (subs.rfind(ARTIST) == 0) {
                    x += ARTIST.length() - 1;
                    output += artist;
                } else if (subs.rfind(TITLE) == 0) {
                    x += TITLE.length() - 1;
                    output += title;
                } else {
                    output += text[x];
                }
            } else {
                output += text[x];
            }
        }
        if (station) {
            LogDebug(VB_PLUGIN, "Setting RDS Station text to %s\n", output.c_str());
            std::vector<std::string> fragments;
            while (output.size()) {
                if (output.size() <= 8) {
                    padTo(output, 8);
                    fragments.push_back(output);
                    output.clear();
                } else {
                    std::string lft = output.substr(0, 8);
                    padTo(lft, 8);
                    output = output.substr(8);
                    fragments.push_back(lft);
                }
            }
            if (fragments.empty()) {
                std::string m = "        ";
                fragments.push_back(m);
            }
            si4713->setRDSStation(fragments);
        } else {
            LogDebug(VB_PLUGIN, "Setting RDS text to %s\n", output.c_str());
            si4713->setRDSBuffer(output);
        }
    }
    
    virtual void playlistCallback(const Json::Value &playlist, const std::string &action, const std::string &section, int item) {
        if (settings["Start"] == "PlaylistStart" && action == "start") {
            startVast();
        } else if (settings["Stop"] == "PlaylistStop" && action == "stop") {
            stopVast();
        }
    }
    virtual void mediaCallback(const Json::Value &playlist, const MediaDetails &mediaDetails) {
        std::string title = mediaDetails.title;
        std::string artist = mediaDetails.artist;
        std::string album = mediaDetails.album;
        int track = mediaDetails.track;
        int length = mediaDetails.length;
        
        std::string type = playlist["currentEntry"]["type"].asString();
        if (type != "both" && type != "media") {
            title = "";
            artist = "";
        }
        
        formatAndSendText(settings["StationText"], artist, title, true);
        formatAndSendText(settings["RDSTextText"], artist, title, false);
    }
    
    
    void setDefaultSettings() {
        setIfNotFound("Start", "FPPDStart");
        setIfNotFound("Frequency", "100.10");
        setIfNotFound("Power", "110");
        setIfNotFound("Preemphasis", "75us");
        setIfNotFound("AntCap", "0");
        setIfNotFound("EnableRDS", "False");
        setIfNotFound("StationText", "Merry   Christ- mas", true);
        setIfNotFound("RDSTextText", "[{Artist} - {Title}]", true);
        setIfNotFound("Pty", "2");
    }
    void setIfNotFound(const std::string &s, const std::string &v, bool emptyAllowed = false) {
        if (settings.find(s) == settings.end()) {
            settings[s] = v;
        } else if (!emptyAllowed && settings[s] == "") {
            settings[s] = v;
        }
    }
    
    Si4713 *si4713 = nullptr;
};


extern "C" {
    FPPPlugin *createPlugin() {
        return new FPPVastFMPlugin();
    }
}
