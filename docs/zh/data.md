# ESP32 Beacon Spam - 配置文件说明

## 文件结构

```
data/
├── settings.json  # 系统设置
└── ssid.json      # SSID 列表
```

## settings.json 配置说明

```json
{
    "security": 4,                    // WiFi 安全等级 (0-7)
    "channel": 6,                     // WiFi 信道 (1-13)
    "mac_prefix": [2, 17, 34, 51, 68], // MAC 地址前5字节
    "beacon_interval": 100,           // Beacon 发送间隔 (毫秒)
    "serial_baud": 115200             // 串口波特率
}
```

### 安全等级对照表

| 值 | 说明 |
|----|------|
| 0 | SECURITY_OPEN - 开放网络 |
| 1 | SECURITY_WPA_TKIP - WPA1-TKIP |
| 2 | SECURITY_WPA_AES - WPA1-AES |
| 3 | SECURITY_WPA2_TKIP - WPA2-TKIP |
| 4 | SECURITY_WPA2_AES - WPA2-AES (推荐) |
| 5 | SECURITY_WPA2_MIXED - WPA2 混合模式 |
| 6 | SECURITY_WPA3 - WPA3-SAE |
| 7 | SECURITY_WPA2_WPA3 - WPA2/WPA3 混合 |

## ssid.json 配置说明

```json
{
    "ssids": [
        "WiFi_Name_1",
        "WiFi_Name_2",
        "中文网络名称",
        "🎉表情符号WiFi🎉"
    ]
}
```

### SSID 规则

- 支持 UTF-8 编码（中文、表情符号等）
- 最大长度：32 字节（注意：中文和表情占多个字节）
- 超过 32 字节会自动截断
- 最多支持 100 个 SSID

## 上传配置文件到 ESP32

### 方法 1: PlatformIO (推荐)

```bash
pio run --target uploadfs
```

### 方法 2: VS Code

1. 打开 PlatformIO 侧边栏
2. 找到 PROJECT TASKS
3. 展开 env:esp32dev
4. 点击 Platform -> Upload Filesystem Image

## 注意事项

1. **中文和表情符号**：
   - 一个中文字符通常占 3 字节
   - 一个表情符号通常占 4 字节
   - 例如："测试WiFi🎉" 实际占用 12+4=16 字节

2. **MAC 地址**：
   - `mac_prefix` 设置前 5 个字节
   - 最后一个字节会根据 SSID 索引自动变化

3. **信道选择**：
   - 2.4GHz: 1-13 (中国)
   - 建议使用 1, 6, 11 (互不干扰)

4. **Beacon 间隔**：
   - 默认 100ms
   - 太快可能导致射频拥塞
   - 太慢手机可能扫描不到
