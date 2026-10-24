# SpotPear ESP32-S3-AI-BOX-V1.28 port

This is a hardware adaptation of [ABSpresso by Tomek Kent (@tomekent)](https://github.com/tomekent/ABSpresso)
for the SpotPear ESP32-S3-AI-BOX-V1.28. The port was developed with OpenAI Codex. The device owner
built and flashed it on the physical board and reported the hardware results below.

## Official device documentation

- [SpotPear product wiki](https://spotpear.com/index.php/wiki/ESP32-S3-N16R8-AI-DeepSeek-XiaoZhi-XiaGe-Qwen-DouBao-1.28-inch-Round-LCD-BOX-TouchScreen.html)
- [SpotPear board schematic (PDF)](https://cdn.static.spotpear.com/uploads/picture/learn/ESP32/ESP32-S3-LCD-1.28-BOX-THIN/ESP32S3-1.28inch-BOX.pdf)

The pin map below follows the SpotPear schematic and product wiki. The owner's copy of the
CST816D controller datasheet identifies the touch controller; the SpotPear wiki links its hardware
documents.

## Pin map

| Function | GPIO / value | Notes |
| --- | --- | --- |
| LCD | MOSI 2, SCLK 4, CS 5, DC 47, reset 38, backlight 42 | 240×240 GC9A01, 4-wire SPI |
| Touch | SDA 11, SCL 7, interrupt 12, reset 6 | CST816D, I²C address 0x15 |
| Audio control | SDA 15, SCL 14 | ES8311, I²C address 0x18 |
| Audio I²S | MCLK 16, BCLK 9, LRCK 45, data to codec 8 | Mono microphone path is not used |
| Speaker amplifier | Enable 46 | NS4150B |
| SD card | CLK 17, CMD 18, D0 21 | 1-bit SDMMC; D3 is not used |
| Battery voltage | GPIO 1 / ADC1 channel 0 | Equal 100 kΩ divider legs; firmware doubles ADC voltage |
| BOOT button | GPIO 0 | Active low |
| Native USB | D− 19, D+ 20 | ESP32-S3 USB Serial/JTAG |

The schematic routes touch and audio control over separate I²C pin pairs. It does not show the
TCA9554 expander used by the original Waveshare board.

## What changed

- Replaced the Waveshare ST77916/QSPI display and expander setup with a GC9A01 SPI display using
  SpotPear's board pins.
- Adapted CST816D touch wiring and polling, plus the ES8311 audio-control and I²S paths, speaker
  amplifier enable, SDMMC storage, battery ADC, and wake controls.
- Corrected the panel's native horizontal mirror while keeping touch coordinates aligned with the
  corrected display.
- Adapted the original 360×360 interface to the 240×240 display and enlarged the top navigation,
  bottom shelf/library selectors, and key player touch targets.
- Updated the component configuration for this board instead of the Waveshare display and I/O
  expander.

## Build and flash

Use [ESP-IDF v5.5.6](https://docs.espressif.com/projects/esp-idf/en/v5.5.6/esp32s3/get-started/); this
port was built with **v5.5.6**. In an ESP-IDF PowerShell with the environment exported, change to
the project folder and run:

```powershell
idf.py set-target esp32s3   # first time only
idf.py build
idf.py -p COM3 flash monitor   # replace COM3 with the board's serial port
```

The first build downloads the components listed in `main/idf_component.yml`. The default console
uses the ESP32-S3 native USB Serial/JTAG interface. If the board does not enter download mode
automatically, hold **BOOT**, press and release **RST**, then release **BOOT** before flashing.

## Device checks reported by the owner

- The firmware built and flashed, and the device boots with a usable display and touchscreen.
- Audiobookshelf connection and book streaming work. The owner retested streaming with the microSD
  card both inserted and removed; no card-dependent stream failure was reproduced. Downloads also
  work when a card is installed.
- The horizontal display correction is in place, with touch coordinates kept in the panel's
  logical orientation.
- The larger touch targets are in the latest flashed update. Their usability has not yet been
  separately reported.

Battery percentage remains an estimate; its voltage reading has not been calibrated against a
meter. The touch controller responds on the tested device, although its supplied datasheet does not
document the coordinate-register protocol used by the Espressif CST816S-family driver.

## Quick checks for another device

1. Confirm the setup screen appears and the image is upright.
2. Test taps and drags across the screen, especially the top navigation, bottom selectors, and
   player controls.
3. Connect to Wi-Fi and Audiobookshelf, then stream a short item at low volume.
4. If using microSD, test browsing, a download, and streaming with the card inserted.
5. Compare the displayed battery voltage with a meter before relying on the battery percentage.

If the board does not enter download mode, hold **BOOT**, press and release **RST**, and then
release **BOOT** before retrying the flash.
