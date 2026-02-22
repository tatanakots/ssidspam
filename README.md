# ESP32 Beacon Spam

<p align="center">
  <img src="https://img.shields.io/badge/ESP32-WiFi-green" alt="ESP32 WiFi">
  <img src="https://img.shields.io/badge/License-MIT-blue" alt="License">
  <img src="https://img.shields.io/badge/Platform-PlatformIO-orange" alt="PlatformIO">
</p>

> ⚠️ **Disclaimer**: This project is for educational and entertainment purposes only. The author is not responsible for any illegal use, malicious activities, or any use that causes discomfort to others. Please use responsibly and in compliance with local laws and regulations.

## Features

- 📡 Broadcast multiple SSID beacons from ESP32
- 🔐 Support for multiple WiFi security types (Open, WPA, WPA2, WPA3)
- 🌐 UTF-8 support (Chinese, emojis, special characters)
- ⚙️ Configurable via JSON files
- 📊 Serial monitor output for debugging

## Hardware

- **Board**: ESP32 Dev Module
- **Flash**: 4MB minimum (requires LittleFS filesystem)

## Installation

### Prerequisites

- [PlatformIO Core](https://platformio.org/install/cli) or [PlatformIO IDE for VS Code](https://platformio.org/install/ide)

### Build

```bash
# Clone the repository
git clone https://github.com/yourusername/ssidspam.git
cd ssidspam

# Build the project
pio run
```

### Upload

```bash
# Upload firmware
pio run --target upload

# Upload filesystem (required for config files)
pio run --target uploadfs
```

## Configuration

### File Structure

```
data/
├── settings.json    # System settings
└── ssid.json        # SSID list
```

### settings.json

```json
{
    "security": 4,
    "channel": 6,
    "mac_prefix": [2, 17, 34, 51, 68],
    "beacon_interval": 100,
    "serial_baud": 115200
}
```

| Parameter | Description |
|-----------|-------------|
| security | WiFi security level (0-7) |
| channel | WiFi channel (1-13) |
| mac_prefix | MAC address prefix (5 bytes) |
| beacon_interval | Beacon interval in ms |
| serial_baud | Serial baud rate |

### Security Levels

| Value | Security Type |
|-------|---------------|
| 0 | Open |
| 1 | WPA-TKIP |
| 2 | WPA-AES |
| 3 | WPA2-TKIP |
| 4 | WPA2-AES (Recommended) |
| 5 | WPA2 Mixed |
| 6 | WPA3-SAE |
| 7 | WPA2/WPA3 Mixed |

### ssid.json

```json
{
    "ssids": [
        "Your_SSID_1",
        "Your_SSID_2",
        "中文网络名称",
        "🎉Emoji WiFi🎉"
    ]
}
```

**Rules**:
- Maximum 32 bytes per SSID (UTF-8 characters may use multiple bytes)
- Maximum 100 SSIDs

## Pre-built Binaries

Pre-built firmware binaries are available in the [Releases](https://github.com/yourusername/ssidspam/releases) page.

You can also find automated builds in the [Actions](https://github.com/yourusername/ssidspam/actions) tab.

## Troubleshooting

### Serial Monitor

Connect at 115200 baud to see debug output:

```bash
pio device monitor
```

### Common Issues

1. **No SSIDs appearing**: Make sure to upload the filesystem after modifying config files
2. **SSIDs not showing**: Check if channel is valid (1-13)
3. **Serial garbled**: Verify baud rate matches settings.json

## License

MIT License - See [LICENSE](LICENSE) for details.

---

<p align="center">
Made with ❤️ for educational purposes
</p>
