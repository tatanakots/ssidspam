# ESP32 Beacon Spam

<p align="center">
  <img src="https://img.shields.io/badge/ESP32-WiFi-green" alt="ESP32 WiFi">
  <img src="https://img.shields.io/badge/License-MIT-blue" alt="License">
  <img src="https://img.shields.io/badge/Platform-PlatformIO-orange" alt="PlatformIO">
</p>

*[English](README.md) | [中文](README.zh.md)*

> ⚠️ **免责声明**: 本项目仅供学习和娱乐目的。作者不对任何非法使用、恶意活动或任何令人感到不适的使用负责。请负责任地使用并遵守当地法律法规。

## 功能特点

- 📡 从 ESP32 广播多个 SSID 信标
- 🔐 支持多种 WiFi 安全类型（开放、WPA、WPA2、WPA3）
- 🌐 UTF-8 支持（中文、表情符号、特殊字符）
- ⚙️ 通过 JSON 文件配置
- 📊 串口监视器输出用于调试

## 硬件

- **开发板**: ESP32 Dev Module
- **闪存**: 至少 4MB（需要 LittleFS 文件系统）

## 安装

### 前置要求

- [PlatformIO Core](https://platformio.org/install/cli) 或 [PlatformIO IDE for VS Code](https://platformio.org/install/ide)

### 编译

```bash
# 克隆仓库
git clone https://github.com/tatanakots/ssidspam.git
cd ssidspam

# 编译项目
pio run
```

### 上传

```bash
# 上传固件
pio run --target upload

# 上传文件系统（配置文件需要）
pio run --target uploadfs
```

## 配置

### 文件结构

```
data/
├── settings.json    # 系统设置
└── ssid.json       # SSID 列表
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

| 参数 | 描述 |
|------|------|
| security | WiFi 安全等级 (0-7) |
| channel | WiFi 信道 (1-13) |
| mac_prefix | MAC 地址前缀 (5 字节) |
| beacon_interval | 信标发送间隔 (毫秒) |
| serial_baud | 串口波特率 |

### 安全等级

| 值 | 安全类型 |
|----|----------|
| 0 | 开放网络 |
| 1 | WPA1-TKIP |
| 2 | WPA1-AES |
| 3 | WPA2-TKIP |
| 4 | WPA2-AES (推荐) |
| 5 | WPA2 混合模式 |
| 6 | WPA3-SAE |
| 7 | WPA2/WPA3 混合 |

### ssid.json

```json
{
    "ssids": [
        "你的SSID_1",
        "你的SSID_2",
        "中文网络名称",
        "🎉表情WiFi🎉"
    ]
}
```

**规则**:
- 每个 SSID 最多 32 字节（UTF-8 字符可能占用多个字节）
- 最多支持 100 个 SSID

## 预编译固件

预编译固件可在 [Releases](https://github.com/tatanakots/ssidspam/releases) 页面获取。

也可以在 [Actions](https://github.com/tatanakots/ssidspam/actions) 选项卡中找到自动化构建。

## 工厂烧录指南

本节介绍如何使用 `esptool.py` 烧录预编译固件用于工厂生产。

### 分区布局 (minimal.csv - 单OTA)

| 二进制文件 | 偏移地址 | 说明 |
|-----------|----------|------|
| bootloader.bin | 0x1000 | ESP32 启动引导程序 |
| partitions.bin | 0x8000 | 分区表 |
| firmware.bin | 0x10000 | 主应用程序 |
| littlefs.bin | 0x150000 | LittleFS 文件系统（配置） |

> **注意**: LittleFS 偏移地址 0x150000 适用于 4MB 闪存使用 minimal.csv 的配置。如使用不同闪存大小请调整。

### 所需文件

编译完成后，以下文件生成在 `.pio/build/esp32dev/` 目录：

- `bootloader.bin` - ESP32 启动引导程序
- `partitions.bin` - 分区表
- `firmware.bin` - 编译后的应用程序
- `littlefs.bin` - 文件系统镜像（包含 `settings.json` 和 `ssid.json`）

### 烧录命令

```bash
# 烧录所有组件
esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 460800 \
  write_flash 0x1000 .pio/build/esp32dev/bootloader.bin \
  0x8000 .pio/build/esp32dev/partitions.bin \
  0x10000 .pio/build/esp32dev/firmware.bin \
  0x150000 .pio/build/esp32dev/littlefs.bin

# 仅烧录固件（如果引导程序/分区未更改）
esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 460800 \
  write_flash 0x10000 .pio/build/esp32dev/firmware.bin
```

### Windows 示例

```powershell
# 以 COM3 为例
esptool.exe --chip esp32 --port COM3 --baud 460800 write_flash 0x1000 .pio\build\esp32dev\bootloader.bin 0x8000 .pio\build\esp32dev\partitions.bin 0x10000 .pio\build\esp32dev\firmware.bin 0x150000 .pio\build\esp32dev\littlefs.bin
```

### 使用 GitHub Actions

项目包含 GitHub Actions 工作流 (`.github/workflows/build.yml`)，会在每次推送和拉取请求时自动构建固件。

**使用方法：**

1. Fork 此仓库
2. 进入 GitHub 仓库的 **Actions** 选项卡
3. 选择 **Build** 工作流
4. 点击 **Run workflow**
5. 完成后，从工作流运行中下载产物

工作流生成以下产物：
- `bootloader.bin` - ESP32 启动引导程序
- `partitions.bin` - 分区表
- `firmware.bin` - 编译后的应用程序
- `littlefs.bin` - 文件系统镜像

您也可以从 [Actions](https://github.com/tatanakots/ssidspam/actions) 页面下载预编译的二进制文件 - 点击已完成的 workflow 运行，向下滚动到 "Artifacts"。

## 故障排除

### 串口监视器

连接 115200 波特率以查看调试输出：

```bash
pio device monitor
```

### 常见问题

1. **没有 SSID 出现**: 确保在修改配置文件后上传文件系统
2. **SSID 不显示**: 检查信道是否有效 (1-13)
3. **串口乱码**: 验证波特率是否与 settings.json 匹配

## 许可证

MIT 许可证 - 详见 [LICENSE](LICENSE) 文件。

---

<p align="center">
仅供学习使用 ❤️
</p>
