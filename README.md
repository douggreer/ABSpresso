<p align="center"><img src="docs/brand/banner.png" alt="ABSpresso: an espresso-sized Audiobookshelf player for an ESP32 with a round display"></p>


# SpotPear AI-BOX 1.28" Port of ABSpresso

I created this fork of ABSpresso because I could not find the device it was built for (Waveshare 1.85" Dev Kit) in stock anywhere, and the retail price was quite high at $50-$60.  This version adapts [ABSpresso by Tomek Kent (@tomekent)](https://github.com/tomekent/ABSpresso)
for SpotPear's 1.28-inch ESP32-S3-AI-BOX-V1.28. This device can be found for as low as $25 on Aliexpress.  I got it for $15 using coupons and coins.  The screen is a little smaller, so the UI had to be adapted somewhat.  It works great, and I thank the original developer for 99% of the code!

## What is ABSpresso?

An espresso-sized audiobook player for [Audiobookshelf](https://www.audiobookshelf.org/). It runs
on an inexpensive ESP32-S3 board with a round touchscreen and a built-in speaker, streams
your books and podcasts over Wi-Fi, and keeps your listening progress in sync with the server, so
you can pick up where you left off here, on your phone or on the web.

It's meant for the desk or the bedside table: a dedicated "carry on with my book" device, with no
phone needed.

## What changed in this port

- Replaced the Waveshare display and board setup with the SpotPear 240×240 GC9A01 display and
  CST816D touch controller.
- Adapted audio, SD card, battery sensing, buttons, and GPIO assignments to the SpotPear schematic.
- Corrected the display's mirrored native orientation while preserving touch coordinates.
- Resized the original interface for 240×240 and enlarged the top navigation and bottom selector
  touch targets, plus key player controls, for easier use on the small screen.
- Updated the build configuration and component list for the SpotPear hardware.
- Added title, author, and series search from Home. Author search lists matching names first, then
  opens the selected author's books. Series search groups numbered entries under one series name
  and opens that series' books in numerical order. Long result titles stay on one line and keep
  their beginning visible.
- Made book and shelf scrolling easier to start, added a drag indicator, and lowered the swipe
  distance needed to change a carousel or shelf.
- Added an asynchronous, bounded playback-error log on the SD card to help diagnose failed
  sessions, streams, local downloads, decoder errors, and audio-output errors.

### Credits and device status

The original ABSpresso project is by **Tomek Kent (@tomekent)**; see the
[upstream project](https://github.com/tomekent/ABSpresso). This SpotPear adaptation and its code
updates were made with **OpenAI Codex**. The device owner compiled and flashed the firmware and
reports successful Audiobookshelf use, working display and touch, and streaming with a microSD card
either inserted or removed. The enlarged touch targets are included in the latest flashed update;
their usability still merits a hands-on check.

Official device references:
[SpotPear product wiki](https://spotpear.com/index.php/wiki/ESP32-S3-N16R8-AI-DeepSeek-XiaoZhi-XiaGe-Qwen-DouBao-1.28-inch-Round-LCD-BOX-TouchScreen.html) ·
[SpotPear board schematic (PDF)](https://cdn.static.spotpear.com/uploads/picture/learn/ESP32/ESP32-S3-LCD-1.28-BOX-THIN/ESP32S3-1.28inch-BOX.pdf).

**License status:** The upstream repository currently has no `LICENSE` file. GitHub permits public
repositories to be viewed and forked within GitHub, but that is not a general software reuse
license. This port adds no license; ask the original author for permission before distributing the
code outside GitHub or telling others they may reuse or modify it. See GitHub's
[Terms of Service](https://docs.github.com/en/site-policy/github-terms/github-terms-of-service)
and [No License guidance](https://choosealicense.com/no-permission/).

See [PORTING.md](PORTING.md) for the SpotPear pin map, build steps, and first hardware checks.

## Features

- **Streams from your server**: audiobooks and podcasts, in any format Audiobookshelf can serve.
  Progress syncs every 20 s and on pause, seek and stop.
- **Home**: Continue Listening, Search by Title, Search by Series, Search by Author, Recently Added,
  and Downloaded. Series matches show one row per series; selecting it opens the books in series
  number order. Author matches open a list of that author's books.
- **Library**: browse by cover, as an A-Z list of books, or by author. Drag the ring on the right
  edge to jump through the alphabet.
- **Now Playing**: cover art backdrop, a progress arc to scrub through the chapter, a volume arc,
  skip back/forward and previous/next chapter.
- **Podcasts**: shows list their episodes (in progress first, then newest); episodes play and
  sync like books.
- **Multiple libraries**: switch between the server's libraries from Settings.
- **SD card (optional)**: caches the library and covers for instant start-up and offline
  browsing, and holds downloaded books for offline listening. Progress made offline is sent to the
  server when it's back in reach. Playback failures are recorded to a small rotating log for later
  diagnosis.
- **Setup from your phone**: the device runs its own Wi-Fi network with a QR code to join. Sign
  in with your Audiobookshelf username and password; no API key needed.
- **Battery aware**: battery level and charging state in the status row, a dimming screen, screen
  off, and deep sleep when idle.
- **Physical controls**: BOOT plays and pauses, or puts the device to sleep when held.
- Accented characters, Cyrillic and Vietnamese titles display correctly.

## Screenshots

These are upstream screenshots captured on the original 360×360 Waveshare board; the SpotPear
port scales the layout for its 240×240 screen.

| Home | Recently Added | Library: covers | Library: books |
| :---: | :---: | :---: | :---: |
| <img src="docs/media/home_continue.png" width="200"> | <img src="docs/media/home_recent.png" width="200"> | <img src="docs/media/library_covers.png" width="200"> | <img src="docs/media/library_list.png" width="200"> |
| **Authors** | **Now Playing** | **Settings** | **Book details** |
| <img src="docs/media/library_authors.png" width="200"> | <img src="docs/media/player_resume.png" width="200"> | <img src="docs/media/settings_device.png" width="200"> | <img src="docs/media/book_sheet.png" width="200"> |

| Browsing covers | Resuming a book | A-Z scrubbing |
| :---: | :---: | :---: |
| <img src="docs/media/carousel.gif" width="260"> | <img src="docs/media/playing.gif" width="260"> | <img src="docs/media/library_scrub.png" width="260"> |

## Getting started

### What you need

- A **SpotPear ESP32-S3-AI-BOX-V1.28** (ESP32-S3, 16 MB flash, 8 MB PSRAM). The port targets its
  GC9A01 display and ES8311 / NS4150B audio hardware; see [Hardware](#hardware).
- An **Audiobookshelf server** that the device can reach over 2.4 GHz Wi-Fi.
- Optional: a FAT-formatted microSD card for caching and downloads, and a 3.7 V LiPo battery.

### Build the SpotPear port from source

You need [ESP-IDF v5.5.6](https://docs.espressif.com/projects/esp-idf/en/v5.5.6/esp32s3/get-started/).
This port was built with **ESP-IDF 5.5.6**. LVGL and the other components are fetched on the first
build.

From an ESP-IDF PowerShell with the environment exported, open this project folder and run:

```powershell
idf.py set-target esp32s3   # first time only
idf.py build
idf.py -p COMx flash monitor   # replace COMx with the board's USB serial port, e.g. COM3
```

The first build downloads LVGL, the GC9A01 panel driver and other ESP-IDF components. If the board
isn't detected, hold **BOOT**, press **RST**, release BOOT, then retry. `idf.py erase-flash` clears
saved settings; normal reflashing keeps them.

## First-time setup

No server address or credentials are built into the firmware. On first start the device goes
straight into setup mode:

<p align="center"><img src="docs/media/setup.png" width="240" alt="Setup screen with QR code"></p>

1. **Join the setup network.** Scan the QR code with your phone camera, or join `ABSpresso-XXXX`
   with the password on the screen. The password is new each time.
2. **Open the setup page.** It usually opens by itself; otherwise go to `http://192.168.4.1`.
3. **Fill it in:**
   - your home Wi-Fi network and password
   - your Audiobookshelf server address, e.g. `https://abs.example.com` (add the port if it isn't the default)
   - your Audiobookshelf **username and password** (or paste an API key instead)
   - optionally, brightness, screen-off and sleep timers, skip lengths and screen rotation
4. **Save & connect.** The device checks that it can join your Wi-Fi and sign in, and tells you
   on the page and the screen if something is wrong. Nothing is saved until both work. It then
   restarts and loads your library.

To change any of this later, open **Settings → Server → Wi-Fi & login setup**. Leave a password
blank to keep the saved one.

The device keeps the sign-in session's tokens, never your password. It renews them automatically
and asks you to sign in again only if the server ends the session.

### Buttons

| Button | Action |
| --- | --- |
| **BOOT**, press | Play / pause (with nothing loaded, resumes your latest book); wakes the screen |
| **BOOT**, hold 2 s | Sleep: stops playback and deep-sleeps until BOOT or a touch wakes it |
| **RST** | Restart |
| Hold **BOOT**, press **RST** | Download mode, for flashing |

## Possible next features

- **Bluetooth audio**: the ESP32-S3 only has Bluetooth Low Energy, not the Classic Bluetooth
  (A2DP) that ordinary headphones use. It would need an external A2DP module, or a different
  chip. A Bluetooth Low Energy remote (play/pause from a button or watch) is possible on this one.
- **Better battery readings**: calibrate the voltage curve against a real discharge, and detect
  charging directly (the charger's status pin isn't connected to the ESP32 on this board).
- **Sleep timer** and **playback speed** on Now Playing.
- **Chapter list** and **collections** in the Library.
- **Downloading podcast episodes** (books can already be downloaded).
- **Over-the-air updates** from GitHub releases.
- **Listening sessions for offline playback**, so it counts in the server's stats.

## Limitations

- This port has been built and flashed on the SpotPear board. Its owner reports working Audiobookshelf
  streaming, display, touch, and SD-card operation both with a card inserted and removed. Battery
  readings have not been calibrated against a meter, and the latest larger touch targets still need
  a usability check.
- Greek, Chinese/Japanese/Korean and emoji don't display.
- Without an SD card, the first library load and covers take a few seconds after each start, and
  playback errors cannot be saved to the persistent log.
- A few unusual JPEG covers can't be decoded and show a title card instead.
- Podcast libraries show up to 150 episodes per show.
- Changing a cover on the server isn't picked up until the SD cache is cleared.

---

## Technical details

### Hardware

**SpotPear ESP32-S3-AI-BOX-V1.28**:

| Part | Details |
| --- | --- |
| SoC | ESP32-S3R8, dual-core 240 MHz, Wi-Fi, Bluetooth LE |
| Memory | 8 MB octal PSRAM, 16 MB flash |
| Display | 1.28" round 240×240 IPS, GC9A01 over SPI |
| Touch | CST816D capacitive (I2C) |
| Storage | microSD slot (1-bit SDMMC) |
| Audio | ES8311 codec + NS4150B amplifier, onboard speaker |
| Battery | PL4054 charger, battery voltage on GPIO1 through a 1:2 divider |

Pin assignments (see `main/hardware/board.c`):

| Function | GPIO |
| --- | --- |
| LCD MOSI / SCLK / CS / DC / reset / backlight | 2 / 4 / 5 / 47 / 38 / 42 |
| Touch I2C SDA / SCL / interrupt / reset | 11 / 7 / 12 / 6 |
| Audio I2C SDA / SCL | 15 / 14 |
| I2S MCLK / BCK / LRCK / data to codec | 16 / 9 / 45 / 8 |
| Speaker amplifier enable | 46 |
| microSD CLK / CMD / D0 (1-bit SDMMC) | 17 / 18 / 21 |
| Battery voltage (ADC1 ch0) | 1 |
| BOOT button | 0 |

`main/hardware/board.c` maps these pins from SpotPear's
[official schematic](https://cdn.static.spotpear.com/uploads/picture/learn/ESP32/ESP32-S3-LCD-1.28-BOX-THIN/ESP32S3-1.28inch-BOX.pdf)
and [product wiki](https://spotpear.com/index.php/wiki/ESP32-S3-N16R8-AI-DeepSeek-XiaoZhi-XiaGe-Qwen-DouBao-1.28-inch-Round-LCD-BOX-TouchScreen.html).
The board separates the touch I2C pins from the codec I2C pins and has no TCA9554 expander. The
firmware uses Espressif's CST816S-family driver for the CST816D; touch was confirmed working on the
owner's device, although the supplied controller datasheet does not document the coordinate
register protocol.

### How playback works

Most audiobooks are large single-file `.m4b` (AAC) files, often 20–40 hours long. Their MP4
index tables run to megabytes, which makes seeking into them directly on a microcontroller
impractical. Instead the player uses Audiobookshelf's HLS transcode mode:

1. `POST /api/items/:id/play` advertises only `audio/mpeg` with `forceTranscode`, so the server
   serves every book, whatever its format, as HLS: 6-second MPEG-TS segments named
   `output-N.ts`, containing AAC (or MP3 for MP3 sources, stream-copied).
2. Segment N always starts at N×6 s, even after the server restarts its transcoder for a seek
   (verified: a segment is byte-identical whichever way it was produced). So the playlist is
   never downloaded: to play from time *t*, the player fetches segments from ⌊t/6⌋ and drops the
   first (t mod 6) seconds of decoded audio.
3. A **fetch task** streams segments over a keep-alive HTTPS connection into a 512 KB PSRAM
   buffer (about a minute of audio). A **decode task** feeds it through `esp_audio_codec`'s TS
   demuxer and AAC/MP3 decoder to I2S. A **control task** handles commands, the playback
   session, and progress sync.

Cover art is downloaded already resized by the server, decoded with the ESP32-S3's ROM JPEG
decoder, and held in an LRU cache in PSRAM (and on the SD card when one is fitted).

### SD card layout

The card is never formatted by the firmware. Everything lives under `/sdcard/abs/`. Each library
has its own cache folder, so switching libraries never overwrites another's cache; downloads are
keyed by item id, which is unique across the server.

| Path | Contents |
| --- | --- |
| `libraries.json`, `me.json` | The server's libraries and the user's progress |
| `lib/<library id>/items.json` | The library's last item list, shown at boot before Wi-Fi is up |
| `lib/<library id>/covers/<id>_<size>.jpg` | Cover JPEGs as downloaded |
| `lib/<library id>/episodes/<id>.json` | A podcast's episode list, for browsing offline |
| `dl/<id>/audio.ts` | A downloaded book: its HLS segments appended in order |
| `dl/<id>/index.bin` | End offset of each completed segment (seeking is one lookup) |
| `dl/<id>/meta.json` | Duration and chapters, for playback without the server |
| `dl/<id>/progress.json` | Local listening position, and whether the server still needs it |
| `logs/playback.log` and `.1` / `.2` | Recent playback failures as JSON lines; rotated and size-limited |

**Downloads** fetch the same transcoded segments the player streams, using a separate session
(with its own device ID), one at a time in the background. A 1-hour book takes about 2 minutes.
Index entries are written only after the audio is synced to the card, so an interrupted download
resumes from the last complete segment. **Playing a downloaded book** reads `audio.ts` through the
same decoder, with no server session; progress is saved locally and sent with
`PATCH /api/me/progress/:id` whenever Wi-Fi is up, taking priority over the server's older value.

### Setup portal

`network/portal.c` switches Wi-Fi to AP+STA. It runs a WPA2 access point (random 8-digit
password), a DNS server that answers every name with `192.168.4.1` (so phones show the captive
portal), and a small web server with the page in `network/portal_page.h`. A save is tested first
(Wi-Fi join, then `POST /login` with `x-return-tokens: true`, or `GET /api/me` for an API key)
before it's written to NVS. Access tokens are renewed with `POST /auth/refresh` when the server
answers 401. The network list is scanned once before the access point starts, because a scan
takes the radio off channel for seconds and drops connected phones.

### Project layout

```
main/
  main.c                 startup: display, Wi-Fi, library load, refresh loop
  hardware/              board bring-up, battery, power (dim/off/sleep, BOOT button), SD card
  network/               Wi-Fi, setup portal, Audiobookshelf REST client
  app/                   settings (NVS), library catalog and cache, covers, downloads, player, text helpers
  ui/                    UI shell, PSRAM allocator for LVGL, generated fonts, screenshot capture
    widgets/             cover carousel, switcher pill, status row
    pages/               Home, Library, Now Playing, Settings, setup screen, book details, episodes
tools/
  capture_to_media.py    turns a screenshot-capture log into PNGs and GIFs
  gen_fonts.sh           regenerates the UI fonts (Montserrat + extended character ranges)
docs/
  media/                 screenshots
  brand/                 logo, icon and banners
```

### Notes from bring-up

- **Internal RAM is the real constraint, not PSRAM.** Wi-Fi, TLS, DMA buffers and task stacks all
  compete for ~512 KB of on-chip SRAM. LVGL uses a custom allocator (`lv_mem_psram.c`) so every UI
  object lives in PSRAM, and most task stacks are in PSRAM too. A task with a PSRAM stack can't
  touch flash (flash access disables the cache PSRAM sits behind), so settings writes go through
  a small worker with an internal stack (`flash_safe()` in `app/config.c`).
- **SD card and PSRAM don't mix:** the SDMMC controller's DMA corrupts data going to or from
  PSRAM buffers, so file data goes through an internal-RAM bounce buffer (`storage.c`), and
  `CONFIG_FATFS_VFS_FSTAT_BLKSIZE` is left at 0.
- **Power saving:** after a while without a touch the screen dims, then turns off (panel asleep,
  LVGL paused, CPU down to 80 MHz) while audio keeps playing. With the screen off, nothing playing
  or downloading and no USB host attached, the device deep-sleeps; the touch controller's GPIO12
  interrupt or BOOT wakes it. Wi-Fi uses modem sleep.
- **Battery:** the voltage is mapped through a typical LiPo curve. The charger's status pin only
  drives its LED and USB power isn't wired to a GPIO, so charging is inferred: a USB host on the
  ESP32's own port, or the voltage jumping or trending. "Charged" is inferred from about 3 minutes
  of flat readings just below 4.2 V while on power.
- **Touch:** the board schematic identifies a CST816D. The port uses Espressif's CST816S-family
  driver with ID reads disabled; touch responds on the owner's device. Raw touch coordinates are
  unmirrored, while the display's native X mirror is corrected in the panel configuration. The old
  Waveshare-only `MotionMask` and sleep-register writes were removed because the CST816D datasheet
  does not document them.
- **Display and controls:** the SpotPear panel is a 240×240 GC9A01 over 4-wire SPI. The original
  360×360 UI is adapted to the smaller panel; navigation, shelf selectors, and player hit targets
  were enlarged in this port. The owner has flashed the update; touch-target usability should be
  assessed during regular use.
- **Server:** some reverse proxies reject default HTTP user agents, so the client sends
  `ABSpresso/<version>`.
- **Debugging aids:** add `target_compile_definitions(${COMPONENT_LIB} PRIVATE ...)` to
  `main/CMakeLists.txt` with `PLAYER_STATUS_LOG` (pipeline stats every second), `PLAYER_NO_SYNC`
  (no progress sync, for tests) or `LAYOUT_AUDIT` (reports controls that overlap the edge arcs).

### Capturing screenshots

The screenshots come from the device. Build with `UI_CAPTURE` and `PLAYER_NO_SYNC` (so the demo
playback doesn't move your real progress):

```cmake
target_compile_definitions(${COMPONENT_LIB} PRIVATE UI_CAPTURE PLAYER_NO_SYNC)
```

After the library loads, `main/ui/ui_capture.c` runs a scripted tour of the UI and streams each
frame over the console as base64 RGB565. LVGL's clock is replaced by a virtual one, so animation
frames land at exact moments. Record the console to a file, then convert it:

```sh
python3 tools/capture_to_media.py serial.log docs/media   # needs ffmpeg
```

The capture shows your own library's titles and covers.
