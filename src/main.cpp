#include "WiFi.h"
#include "esp_wifi.h"
#include <cstring>
#include <LittleFS.h>
#include <ArduinoJson.h>

// WiFi Security Types
enum WiFiSecurity {
    SECURITY_OPEN = 0,       // Open network (no password)
    SECURITY_WPA_TKIP = 1,   // WPA-PSK (TKIP) - legacy device compatibility
    SECURITY_WPA_AES = 2,    // WPA-PSK (AES/CCMP)
    SECURITY_WPA2_TKIP = 3,  // WPA2-PSK (TKIP) - compatibility mode
    SECURITY_WPA2_AES = 4,   // WPA2-PSK (AES/CCMP) - recommended
    SECURITY_WPA2_MIXED = 5, // WPA2-PSK (TKIP+AES) - maximum compatibility
    SECURITY_WPA3 = 6,       // WPA3-SAE
    SECURITY_WPA2_WPA3 = 7   // WPA2/WPA3 mixed mode
};

// Configuration
#define MAX_SSIDS 100
#define MAX_SSID_LEN 33  // 32 bytes + null terminator

// SSID storage (UTF-8 support for Unicode characters and emojis)
char ssidList[MAX_SSIDS][MAX_SSID_LEN];
uint8_t ssidLengths[MAX_SSIDS];  // Actual byte length of each SSID
int ssidCount = 0;

// Settings (loaded from config files)
WiFiSecurity wifiSecurity = SECURITY_WPA2_AES;
uint8_t channel = 6;
uint8_t macPrefix[5] = {0x02, 0x11, 0x22, 0x33, 0x44};
uint32_t beaconInterval = 100;
uint32_t serialBaud = 115200;

uint32_t lastPacketTime = 0;
uint32_t lastProbeTime = 0;
uint8_t macAddr[6] = {0x02, 0x11, 0x22, 0x33, 0x44, 0x00};  // Base spoofed MAC

// Probe Request storage
static wifi_promiscuous_pkt_type_t lastProbeType;
static uint8_t probeSSID[33] = {0};
static uint8_t probeRequestMac[6] = {0};
static bool hasProbeRequest = false;

// Base frame header template
uint8_t baseFrameHeader[] = {
    0x80, 0x00, 0x00, 0x00, 
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Receiver: broadcast
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, // Source MAC (modified in code)
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, // BSSID (modified in code)
    0x00, 0x00,                         // Sequence number
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Timestamp
    0x64, 0x00,                         // Beacon interval (0x64 = 102.4ms)
    0x31, 0x00,                         // Capability information
};

// Rate information element template (channel updated at runtime)
uint8_t rateInfoTemplate[] = {
    0x01, 0x08, 0x82, 0x84, 0x8b, 0x96, 0x24, 0x30, 0x48, 0x6c, // Rates
    0x03, 0x01, 0x06,                   // Channel parameter (updated at runtime)
};

// ===== WPA1 Information Elements (Microsoft OUI: 00-50-F2) ===== //

// WPA1-TKIP Information Element
uint8_t wpaWPA1_TKIP[] = {
    0xdd, 0x16,                         // Vendor Specific tag and length
    0x00, 0x50, 0xf2, 0x01,             // Microsoft OUI + WPA type
    0x01, 0x00,                         // Version 1
    0x00, 0x50, 0xf2, 0x02,             // Group cipher: TKIP
    0x01, 0x00,                         // Pairwise cipher count: 1
    0x00, 0x50, 0xf2, 0x02,             // Pairwise cipher: TKIP
    0x01, 0x00,                         // AKM suite count: 1
    0x00, 0x50, 0xf2, 0x02              // AKM suite: PSK
};

// WPA1-AES Information Element
uint8_t wpaWPA1_AES[] = {
    0xdd, 0x16,                         // Vendor Specific tag and length
    0x00, 0x50, 0xf2, 0x01,             // Microsoft OUI + WPA type
    0x01, 0x00,                         // Version 1
    0x00, 0x50, 0xf2, 0x04,             // Group cipher: CCMP (AES)
    0x01, 0x00,                         // Pairwise cipher count: 1
    0x00, 0x50, 0xf2, 0x04,             // Pairwise cipher: CCMP (AES)
    0x01, 0x00,                         // AKM suite count: 1
    0x00, 0x50, 0xf2, 0x02              // AKM suite: PSK
};

// ===== WPA2 (RSN) Information Elements ===== //

// RSN (WPA2-TKIP) Information Element
uint8_t rsnWPA2_TKIP[] = {
    0x30, 0x14,                         // RSN tag and length
    0x01, 0x00,                         // Version 1
    0x00, 0x0f, 0xac, 0x02,             // Group cipher: TKIP
    0x01, 0x00,                         // Pairwise cipher count: 1
    0x00, 0x0f, 0xac, 0x02,             // Pairwise cipher: TKIP
    0x01, 0x00,                         // AKM suite count: 1
    0x00, 0x0f, 0xac, 0x02,             // AKM suite: PSK
    0x00, 0x00                          // RSN capabilities
};

// RSN (WPA2-AES/CCMP) Information Element
uint8_t rsnWPA2_AES[] = {
    0x30, 0x14,                         // RSN tag and length
    0x01, 0x00,                         // Version 1
    0x00, 0x0f, 0xac, 0x04,             // Group cipher: CCMP
    0x01, 0x00,                         // Pairwise cipher count: 1
    0x00, 0x0f, 0xac, 0x04,             // Pairwise cipher: CCMP
    0x01, 0x00,                         // AKM suite count: 1
    0x00, 0x0f, 0xac, 0x02,             // AKM suite: PSK
    0x00, 0x00                          // RSN capabilities
};

// RSN (WPA2 TKIP+AES Mixed) Information Element
uint8_t rsnWPA2_MIXED[] = {
    0x30, 0x18,                         // RSN tag and length
    0x01, 0x00,                         // Version 1
    0x00, 0x0f, 0xac, 0x02,             // Group cipher: TKIP
    0x02, 0x00,                         // Pairwise cipher count: 2
    0x00, 0x0f, 0xac, 0x02,             // Pairwise cipher: TKIP
    0x00, 0x0f, 0xac, 0x04,             // Pairwise cipher: CCMP
    0x01, 0x00,                         // AKM suite count: 1
    0x00, 0x0f, 0xac, 0x02,             // AKM suite: PSK
    0x00, 0x00                          // RSN capabilities
};

// RSN (WPA3-SAE) Information Element
uint8_t rsnWPA3[] = {
    0x30, 0x14,                         // RSN tag and length
    0x01, 0x00,                         // Version 1
    0x00, 0x0f, 0xac, 0x04,             // Group cipher: CCMP
    0x01, 0x00,                         // Pairwise cipher count: 1
    0x00, 0x0f, 0xac, 0x04,             // Pairwise cipher: CCMP
    0x01, 0x00,                         // AKM suite count: 1
    0x00, 0x0f, 0xac, 0x08,             // AKM suite: SAE (WPA3)
    0x00, 0x00                          // RSN capabilities
};

// RSN (WPA2/WPA3 Mixed Mode) Information Element
uint8_t rsnWPA2WPA3[] = {
    0x30, 0x18,                         // RSN tag and length
    0x01, 0x00,                         // Version 1
    0x00, 0x0f, 0xac, 0x04,             // Group cipher: CCMP
    0x01, 0x00,                         // Pairwise cipher count: 1
    0x00, 0x0f, 0xac, 0x04,             // Pairwise cipher: CCMP
    0x02, 0x00,                         // AKM suite count: 2
    0x00, 0x0f, 0xac, 0x02,             // AKM suite: PSK (WPA2)
    0x00, 0x0f, 0xac, 0x08,             // AKM suite: SAE (WPA3)
    0x00, 0x00                          // RSN capabilities
};

// Send complete Beacon or Probe Response frame
void sendFrame(const uint8_t* ssid, uint8_t ssidLen, uint8_t frameType) {
    uint8_t frameBuffer[200];
    uint16_t frameLen = 0;
    
    // Copy frame header
    memcpy(frameBuffer, baseFrameHeader, sizeof(baseFrameHeader));
    
    // Set frame type (Beacon: 0x80, Probe Response: 0x50)
    frameBuffer[0] = frameType;
    
    frameLen = sizeof(baseFrameHeader);
    
    // Add SSID element (dynamic length)
    frameBuffer[frameLen++] = 0x00; // SSID tag
    frameBuffer[frameLen++] = ssidLen;
    memcpy(&frameBuffer[frameLen], ssid, ssidLen);
    frameLen += ssidLen;
    
    // Copy rate information element
    memcpy(&frameBuffer[frameLen], rateInfoTemplate, sizeof(rateInfoTemplate));
    frameLen += sizeof(rateInfoTemplate);
    
    // Add WPA/RSN IE based on security level
    switch (wifiSecurity) {
        case SECURITY_WPA_TKIP:
            memcpy(&frameBuffer[frameLen], wpaWPA1_TKIP, sizeof(wpaWPA1_TKIP));
            frameLen += sizeof(wpaWPA1_TKIP);
            break;
        case SECURITY_WPA_AES:
            memcpy(&frameBuffer[frameLen], wpaWPA1_AES, sizeof(wpaWPA1_AES));
            frameLen += sizeof(wpaWPA1_AES);
            break;
        case SECURITY_WPA2_TKIP:
            memcpy(&frameBuffer[frameLen], rsnWPA2_TKIP, sizeof(rsnWPA2_TKIP));
            frameLen += sizeof(rsnWPA2_TKIP);
            break;
        case SECURITY_WPA2_AES:
            memcpy(&frameBuffer[frameLen], rsnWPA2_AES, sizeof(rsnWPA2_AES));
            frameLen += sizeof(rsnWPA2_AES);
            break;
        case SECURITY_WPA2_MIXED:
            memcpy(&frameBuffer[frameLen], rsnWPA2_MIXED, sizeof(rsnWPA2_MIXED));
            frameLen += sizeof(rsnWPA2_MIXED);
            break;
        case SECURITY_WPA3:
            memcpy(&frameBuffer[frameLen], rsnWPA3, sizeof(rsnWPA3));
            frameLen += sizeof(rsnWPA3);
            break;
        case SECURITY_WPA2_WPA3:
            memcpy(&frameBuffer[frameLen], rsnWPA2WPA3, sizeof(rsnWPA2WPA3));
            frameLen += sizeof(rsnWPA2WPA3);
            break;
        case SECURITY_OPEN:
        default:
            // Open network - no security element
            break;
    }
    
    // Send frame
    esp_wifi_80211_tx(WIFI_IF_STA, frameBuffer, frameLen, false);
}

// Probe Request parsing callback
void promiscuousCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (type == WIFI_PKT_MGMT) {
        wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
        uint8_t* payload = pkt->payload;
        uint8_t frameCtrl = payload[0];
        
        // Check if Probe Request (subtype = 0x04)
        if ((frameCtrl & 0xFC) == 0x40) {
            // Parse source MAC
            memcpy(probeRequestMac, payload + 10, 6);
            
            // Parse SSID (search for SSID tag 0x00)
            memset(probeSSID, 0, 33);
            for (int i = 24; i < 100 && payload[i] != 0; i++) {
                if (payload[i] == 0x00 && payload[i+1] != 0x00) {
                    uint8_t ssidLen = payload[i+1];
                    if (ssidLen > 32) ssidLen = 32;
                    memcpy(probeSSID, &payload[i+2], ssidLen);
                    hasProbeRequest = true;
                    break;
                }
            }
            
            // Broadcast SSID (length 0) case
            if (payload[24] == 0x00 && payload[25] == 0x00) {
                hasProbeRequest = true;
            }
        }
    }
}

// ===== Configuration Loading Functions ===== //

// Load settings configuration
bool loadSettings() {
    File file = LittleFS.open("/settings.json", "r");
    if (!file) {
        Serial.println("Failed to open settings.json");
        return false;
    }
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    
    if (error) {
        Serial.print("Failed to parse settings.json: ");
        Serial.println(error.c_str());
        return false;
    }
    
    // Read configuration
    wifiSecurity = (WiFiSecurity)doc["security"].as<int>();
    channel = doc["channel"].as<uint8_t>();
    beaconInterval = doc["beacon_interval"].as<uint32_t>();
    serialBaud = doc["serial_baud"].as<uint32_t>();
    
    // Read MAC prefix
    JsonArray macArray = doc["mac_prefix"].as<JsonArray>();
    for (int i = 0; i < 5 && i < macArray.size(); i++) {
        macPrefix[i] = macArray[i].as<uint8_t>();
        macAddr[i] = macPrefix[i];
    }
    
    // Update channel in rate template
    rateInfoTemplate[12] = channel;
    
    Serial.println("Settings loaded successfully");
    Serial.printf("Security: %d, Channel: %d, Beacon Interval: %d ms\n", 
                  wifiSecurity, channel, beaconInterval);
    return true;
}

// Load SSID list
bool loadSSIDs() {
    File file = LittleFS.open("/ssid.json", "r");
    if (!file) {
        Serial.println("Failed to open ssid.json");
        return false;
    }
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    
    if (error) {
        Serial.print("Failed to parse ssid.json: ");
        Serial.println(error.c_str());
        return false;
    }
    
    // Read SSID array
    JsonArray ssids = doc["ssids"].as<JsonArray>();
    ssidCount = 0;
    
    for (JsonVariant ssid : ssids) {
        if (ssidCount >= MAX_SSIDS) break;
        
        const char* ssidStr = ssid.as<const char*>();
        if (ssidStr) {
            size_t len = strlen(ssidStr);
            if (len > 32) len = 32;
            
            memcpy(ssidList[ssidCount], ssidStr, len);
            ssidList[ssidCount][len] = '\0';
            ssidLengths[ssidCount] = len;
            ssidCount++;
        }
    }
    
    Serial.printf("Loaded %d SSIDs\n", ssidCount);
    return true;
}

void setup() {
    // Initialize serial (will reinitialize if baud rate changes)
    Serial.begin(115200);
    delay(100);
    
    Serial.println("\n=== ESP32 Beacon Spam ===");
    
    // Initialize LittleFS
    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS Mount Failed!");
        Serial.println("Using default configuration...");
    } else {
        Serial.println("LittleFS mounted successfully");
        
        // Load configuration files
        if (loadSettings()) {
            if (serialBaud != 115200) {
                Serial.end();
                delay(100);
                Serial.begin(serialBaud);
                delay(100);
                Serial.printf("Serial reinitialized at %d baud\n", serialBaud);
            }
        }
        
        if (!loadSSIDs()) {
            Serial.println("Failed to load SSIDs, using empty list");
        }
    }
    
    // Initialize WiFi
    WiFi.mode(WIFI_MODE_STA);
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    
    // Register promiscuous callback for Probe Request
    esp_wifi_set_promiscuous_rx_cb(promiscuousCallback);
    
    Serial.println("Beacon Spam Started!");
    Serial.printf("Broadcasting %d SSIDs on channel %d\n", ssidCount, channel);
}

void loop() {
    uint32_t now = millis();
    
    // Handle Probe Response
    if (hasProbeRequest) {
        if (now - lastProbeTime > 50) {  // Rate limit Probe Response
            lastProbeTime = now;
            
            // Check if matching our SSIDs
            for (int i = 0; i < ssidCount; i++) {
                if (probeSSID[0] == 0 ||  // Broadcast Probe Request
                    strncmp((char*)probeSSID, ssidList[i], ssidLengths[i]) == 0) {
                    
                    // Set MAC
                    macAddr[5] = 0x10 + i;
                    memcpy(&baseFrameHeader[10], macAddr, 6);
                    memcpy(&baseFrameHeader[16], macAddr, 6);
                    
                    // Send Probe Response (frame type 0x50)
                    sendFrame((uint8_t*)ssidList[i], ssidLengths[i], 0x50);
                    delay(1);
                }
            }
            hasProbeRequest = false;
        }
    }
    
    // Periodically send Beacon frames
    if (now - lastPacketTime > beaconInterval) { 
        lastPacketTime = now;

        for (int i = 0; i < ssidCount; i++) {
            // Modify MAC address for each SSID
            macAddr[5] = 0x10 + (i & 0xFF); 
            memcpy(&baseFrameHeader[10], macAddr, 6);
            memcpy(&baseFrameHeader[16], macAddr, 6);

            // Send Beacon (frame type 0x80)
            sendFrame((uint8_t*)ssidList[i], ssidLengths[i], 0x80);
            
            // Small delay between SSIDs to avoid RF buffer congestion
            delay(2); 
        }
    }
}