/******************************************************************************
 *
 *  OUMA RIA SMART CLOCK
 *  Version 1.0  --  Release Candidate 1
 *
 *  Designed and Developed by
 *      Henry Vermaak
 *      Koster, North West
 *      South Africa
 *
 *------------------------------------------------------------------------------
 *
 *  Project Purpose
 *
 *  The OUMA RIA SMART CLOCK is an ESP32-based home information, reminder
 *  and safety system developed to assist people in their everyday lives,
 *  with special attention given to older people living independently.
 *
 *  Every major feature was developed in response to a real-world need
 *  identified during daily use in the author's own home.
 *
 *  
 ******************************************************************************/

/******************************************************************************
 * FEATURE OVERVIEW
 ******************************************************************************/
//    NTP time sync        -- auto-syncs on boot + every 6 hours via WiFi
//    DS3231 RTC           -- temperature-compensated, +/-2ppm, built-in temp sensor
//    BH1750 light sensor  -- automatic day/night display brightness
//    Web config portal    -- browse to the clock's IP to set WiFi, time, volume
//    OTA updates          -- flash new firmware over WiFi, no USB cable needed
//    Automatic WiFi recovery -- reconnects on its own after a power outage,
//                               no button press required (manual backup kept)
//    Medicine reminders   -- up to 4 daily doses, plus two independent short
//                             courses (e.g. two different antibiotics at once)
//                             with Pushover reminders and phone acknowledgement
//    Birthday calendar, doctor appointments, and a general events calendar
//    Shopping / reminder notepad
//    Security alarm       -- 2 zones, 5 PIR sensors, siren, panic button,
//                             Pushover emergency alerts
//    Egg timer, smoker uptimer, braai timer, and a built-in braai times guide
//    Multiple alarms with day-of-week scheduling and snooze
//    Bilingual Afrikaans / English interface throughout
//    All settings stored in NVS -- survive firmware updates and power cuts

/******************************************************************************
 * HARDWARE
 ******************************************************************************/
//    DS3231 RTC         -- I2C  SDA=GPIO21  SCL=GPIO22   VCC=3.3V
//    BH1750 light       -- I2C  shared bus              VCC=3.3V
//    DFPlayer Mini      -- UART2  TX=GPIO17  RX=GPIO16   VCC=5V
//    4x 8x8 WS2812B     -- GPIO13  (470 ohm series resistor)
//    WS2812B 3x10 bar   -- GPIO4  (470 ohm series resistor)
//    Buttons (active LOW, INPUT_PULLUP unless noted):
//      GPIO19  BTN_MODE    -- cycle display modes              (Sw1  Red)
//      GPIO26  BTN_SET     -- adjust / start-pause egg timer    (Sw2  Blue)
//      GPIO27  BTN_UP      -- adjust / dismiss alarm / SNOOZE   (Sw3  White)
//      GPIO14  BTN_ALM     -- toggle alarm on/off               (Sw4  White)
//      GPIO12  BTN_CHIME   -- toggle chimes on/off              (Sw5  Red)
//      GPIO32  BTN_CVUP    -- volume up                         (Sw6  Blue)
//      GPIO33  BTN_CVDN    -- volume down                       (Sw7  Yellow, INPUT only)
//      GPIO34  BTN_SOUND   -- cycle sound set                   (Sw8  Yellow, INPUT only)
//      GPIO35  BTN_ALMSEL  -- reserved / spare                  (Sw9  Red,    INPUT only)
//      GPIO2   BTN_PILL    -- WiFi/network reset (hold 3s)      (Sw10 Blue)
//      GPIO18  BTN_MED     -- medicine confirm                  (Sw11 Yellow)
//      GPIO25  BTN_BDAY    -- birthday/doctor accept            (Sw12 White)
//    GPIO33, 34, 35 are input-only pins with no internal pull-up resistor --
//    fit an EXTERNAL 10K ohm pull-up to 3.3V on each of these three.

/******************************************************************************
 * WIRING SUMMARY
 ******************************************************************************/
//  Component            | Connection
//  --------------------- ----------------------------------------------------
//  DS3231 RTC           | SDA->GPIO21  SCL->GPIO22  VCC->3.3V  GND->GND
//  BH1750               | SDA->GPIO21  SCL->GPIO22  VCC->3.3V  GND->GND  ADDR->GND
//  DFPlayer Mini        | TX2(GPIO17)->DFP_RX  RX2(GPIO16)<-DFP_TX
//                        | 1K ohm resistor on DFPlayer RX pin. VCC->5V  GND->GND
//  WS2812B x4 panels    | DIN->GPIO13 (via 470 ohm)  VCC->5V (3A+)  GND->GND
//  WS2812B 3x10 bar     | DIN->GPIO4 (via 470 ohm)  VCC->5V  GND->GND
//    (one chain, cut      physical  0-9  = pill/appointment day strip
//     from one roll)      physical 10-19 = medicine dose + birthday indicators
//                         physical 20-29 = status: alarms, security, internet,
//                                          calendar, courtyard-armed, wekkers
//  Buttons x12          | one pin -> GPIO   other pin -> GND
//    GPIO33, 34, 35       EXTERNAL 10K ohm pull-up to 3.3V required
//    (input-only pins)    (no internal pull-up on 33/34/35/36/39)
//  Siren (piezo)         | GPIO5 -> IRLZ44N MOSFET gate, 10K series resistor,
//                          10K pull-down resistor to GND (prevents a boot chirp)
//  PIR sensors x5        | AM312, 3.3V, direct to GPIO, no divider needed

/******************************************************************************
 * DISPLAY MODES
 ******************************************************************************/
//    0 = Clock             amber
//    1 = Alarm set          cyan
//    2 = Egg timer run     green
//    3 = Egg timer set    orange
//    4 = Temperature      magenta (DS3231 built-in sensor, C or F)
//    5 = Uptime              blue (days.hours since boot)
//    Long-press BTN_MODE (>1s) toggles 12hr / 24hr display

/******************************************************************************
 * NETWORK, OTA & WEB PORTAL
 ******************************************************************************/
//    First boot: the clock creates a WiFi setup AP -- connect a phone to it
//    and follow the captive portal to enter your home WiFi credentials and
//    timezone offset. After that it connects automatically on every boot,
//    and retries automatically in the background if WiFi is down or drops
//    (loadshedding-safe -- no button press needed under normal conditions;
//    a manual hold-3-seconds reset button and a Restart button in /config
//    remain available as a backup). Time syncs on boot and every 6 hours.
//
//    OTA updates: Arduino IDE -> Sketch -> Upload -> select the clock's
//    network port. Set your own OTA_PASSWORD before flashing a real device.
//
//    Web portal pages include: / (status), /alarms, /wekkers, /medicine,
//    /calendar, /notepad, /alarm (security), /config, /set, /ota, and
//    several on-device reference pages (LED colour guide, button guide,
//    braai times).

/******************************************************************************
 * LIBRARIES (install via Arduino Library Manager)
 ******************************************************************************/
//    FastLED          by Daniel Garcia
//    RTClib           by Adafruit               (DS3231 driver)
//    BH1750           by Christopher Laws
//    WiFiManager      by tzapu                  (captive portal WiFi setup)
//    ArduinoOTA       (built into ESP32 Arduino core)
//    WebServer        (built into ESP32 Arduino core)
//    Preferences      (built into ESP32 Arduino core)
//    Wire             (built into ESP32 Arduino core)
//    time.h / sntp.h  (built into ESP32 Arduino core)
//    NOTE: DFRobotDFPlayerMini library is NOT needed -- a direct serial
//    driver is used instead.

/******************************************************************************
 * BOARD SETTINGS (Arduino IDE)
 ******************************************************************************/
//    Board:      ESP32 Dev Module
//    Flash Size: 4MB
//    Partition:  "Minimal SPIFFS (Large APPS with OTA)" -- REQUIRED for OTA;
//                do not use a "Huge APP" scheme, it removes the second OTA
//                app slot and disables over-the-air updates entirely.
//    CPU Speed:  240MHz
//    Upload:     921600 baud


// ── Arduino / ESP32 Core ──────────────────────────────────────────────────
#include <Arduino.h>
#include <Wire.h>                // I2C bus — shared by the RTC and light sensor
#include <time.h>                // system time / NTP (configTime, getLocalTime)

// ── Communications ──────────────────────────────────────────────────────────
#include <WiFiManager.h>         // tzapu WiFiManager — captive portal WiFi setup
#include <ESPmDNS.h>             // http://oumariaclock.local
#include <WiFiClientSecure.h>    // HTTPS client — Pushover notifications

// ── Display ────────────────────────────────────────────────────────────────
#include <FastLED.h>             // WS2812B main matrix + status strip

// ── RTC ────────────────────────────────────────────────────────────────────
#include <RTClib.h>              // Adafruit RTClib — DS3231 real-time clock

// ── Sensors ────────────────────────────────────────────────────────────────
#include <BH1750.h>              // ambient light sensor — auto-brightness

// ── Audio ──────────────────────────────────────────────────────────────────
// (none — DFPlayer Mini uses a direct UART serial driver, no library needed)

// ── Storage ────────────────────────────────────────────────────────────────
#include <Preferences.h>         // NVS key-value storage — all settings persist here

// ── Web Server ─────────────────────────────────────────────────────────────
#include <WebServer.h>           // config portal + status pages
#include <ArduinoOTA.h>          // over-the-air firmware updates

/******************************************************************************
 * VERSION INFORMATION
 ******************************************************************************/
#define FW_NAME        "OUMA RIA SMART CLOCK"
#define FW_VERSION     "1.0"
#define FW_STATUS      "Release Candidate 1"
#define FW_AUTHOR      "Henry Vermaak"

#define FIRMWARE_VERSION   FW_NAME " v" FW_VERSION   // used for the web footer / serial banner

/******************************************************************************
 * PIN DEFINITIONS — every physical GPIO assignment, in one place
 ******************************************************************************/
#define WS_PIN       13   // Main matrix WS2812B data
#define BTN_MODE     19   // NOTE: physical GPIO25 has a confirmed ~100 ohm stray
                          // leakage fault on this specific ESP32 module (measured
                          // directly on the pin with the button and wire fully
                          // removed — not a wiring/switch issue). BTN_MODE was
                          // moved here to GPIO19 and BTN_BDAY moved to GPIO25 below
                          // (a low-consequence role: its logic only fires silently
                          // when nothing is due, so a stray trigger there is
                          // harmless, unlike MODE's long-press which was falsely
                          // toggling C/F at every boot). Physical button wiring to
                          // be swapped to match — see BTN_BDAY comment below.
#define BTN_SET      26
#define BTN_UP       27
#define BTN_ALM      14
#define BTN_CHIME    12
#define BTN_CVUP     32
#define BTN_CVDN     33
#define BTN_SOUND    34   // input-only — needs external 10K pull-up to 3.3V
#define BTN_ALMSEL   35   // input-only — needs external 10K pull-up to 3.3V
#define BTN_PILL      2   // pill confirm / repurposed: hold 3s = WiFi/network reset
#define BTN_MED      18   // medicine confirm (INPUT_PULLUP)
#define BTN_BDAY     25   // birthday/doctor accept (INPUT_PULLUP) — deliberately
                          // parked on the faulty pin (see BTN_MODE note above).
                          // Physical wires: swap the MODE and BIRTHDAY button
                          // wires between GPIO19 and GPIO25 so the panel labels
                          // once again match their real function.
#define DFP_RX_PIN   16   // ESP32 RX ← DFPlayer TX
#define DFP_TX_PIN   17   // ESP32 TX → DFPlayer RX
#define BAR3_PIN      4   // single WS2812B data pin — ONE chain, cut from one roll:
                          //   physical 0-9   = day-of-week / pill strip
                          //   physical 10-19 = medicine dose + birthday/doctor LEDs
                          //   physical 20-29 = free/status bar (alarm, internet, calendar...)
#define PIR_PARKED  255   // sentinel: PIR slot with no GPIO — always reads "no motion"
#define PIR1_PIN   PIR_PARKED // Zone 1 Courtyard sensor A — no free GPIO remains on
                          // this board; see the wiring note further down for a
                          // PCF8575 I2C expander as the clean way to add it back
#define PIR2_PIN     15   // Zone 1 Courtyard sensor B  (INPUT, always armed)
                          // GPIO15 LOW at boot is safe — only silences ROM boot log
#define PIR3_PIN     36   // Zone 1 Courtyard sensor C  (input-only, always armed)
#define PIR4_PIN     39   // Zone 2 Front door/Entrance (input-only, arm-away only)
#define PIR5_PIN     23   // Zone 2 Passage/Lounge      (INPUT,      arm-away only)
#define SIREN_PIN     5   // IRLZ44N MOSFET gates — needs 10K gate pulldown resistor

/******************************************************************************
 * CONSTANTS — global, cross-cutting values. Subsystem-local sizing constants
 * (e.g. how many Kort Kursus slots, how many calendar events) stay next to
 * the struct/array they configure, further down in each subsystem's section.
 ******************************************************************************/

// -- Firmware behaviour -------------------------------------------------------
#define PILLS_ENABLED  0   // 0 = daily-pills subsystem disabled (the Medicine
                           // schedule below covers it). Set to 1 to re-enable.
#define SPLASH_MAJOR  1   // boot splash scrolls "V <MAJOR>.<MINOR>" on the LED
#define SPLASH_MINOR  0   // matrix — UPDATE THESE TOGETHER WITH FW_VERSION!
                          // (single digits 0-9 only: they index FONT5 directly)

// -- NTP / time ----------------------------------------------------------------
#define NTP_SERVER1        "pool.ntp.org"
#define NTP_SERVER2        "time.google.com"
#define NTP_SYNC_INTERVAL  (6UL * 3600UL)     // resync every 6 hours (seconds)

// -- WiFi / network --------------------------------------------------------------
// If WiFi can't connect (or drops mid-session, e.g. a power outage), the
// clock keeps retrying by itself in the background, forever, with no button
// press or menu needed. See wifi_connect_with_retries() and the WiFi-health
// check inside wifi_task()'s main loop.
#define OTA_PASSWORD       "CHANGE_ME"        // set your own password before flashing
#define WIFI_CONNECT_ATTEMPT_TIMEOUT_MS   15000            // how long ONE attempt waits before giving up
#define WIFI_RETRY_FIRST_WAIT_MS          (60UL * 1000UL)  // wait 60s before the very first retry
#define WIFI_RETRY_INTERVAL_MS            (3UL * 60UL * 1000UL)  // then retry every 3 minutes, forever
#define WIFI_HEALTH_CHECK_INTERVAL_S      60               // how often to notice a mid-session drop
#define WIFI_BTN_HOLD_MS                  3000             // hold BTN_PILL this long for a manual WiFi/reboot reset
#define INET_CHECK_INTERVAL               30               // seconds between internet-reachability probes
// Diagnostic switch — set to 0 to fully disable WiFi/NTP/OTA/webserver, e.g.
// for isolating whether background WiFi activity affects DFPlayer audio.
#define ENABLE_WIFI  1

// -- Rollover-safe millis() deadline check ----------------------------------------
// Fixes a subtle rollover bug found on review: code of the form
//   end = millis() + duration;   ... later ...   if (millis() >= end) { ... }
// looks fine, but millis() wraps back to 0 roughly every 49.7 days. If a
// timer is armed close enough to the wrap that "millis() + duration" itself
// overflows past 4,294,967,295, "end" comes out as a SMALL number even
// though it represents a point still in the future — and a plain ">="
// comparison then trips immediately (or in the case of "<", never trips),
// firing early or late for one timer, exactly once, until the wrap has
// fully passed. Unsigned subtraction doesn't have this problem: casting
// the difference to a signed 32-bit value gives the correct sign even
// across a wraparound, as long as the two timestamps are within ~24.8 days
// of each other (true for every timer in this firmware — all are seconds
// long). Use TIME_REACHED(now, deadline) wherever a "millis() + duration"
// deadline is compared against the current time.
#define TIME_REACHED(now, deadline)  ((int32_t)((now) - (deadline)) >= 0)

// -- General behaviour -----------------------------------------------------------
#define BUTTON_SETTLE_MS   2000                // ignore hold/long-press logic for this
                                                // long after loop() first starts scanning
                                                // buttons — lets every GPIO settle through
                                                // its pull-up before the firmware trusts it
#define SNOOZE_MINUTES     9
#define SMOKE_PEEK_MS      5000                // how long the quick smoker-uptime peek stays on screen
#define NIGHT_START        18   // winter — sun sets 18:00
#define NIGHT_END           7   // winter — sun rises 07:00
#define NUM_ALARM_SLOTS    3

// -- Security alarm timing --------------------------------------------------------
#define ZONE1_COUNT  3    // Outdoor courtyard sensors  (24/7, immediate siren)
#define ZONE2_COUNT  2    // Indoor sensors             (away mode, entry delay)
#define PIR_COUNT    5
#define ALARM_EXIT_DELAY_S   30
#define ALARM_ENTRY_DELAY_S  30
#define ALARM_SIREN_TIMEOUT_S   180   // PIR alarm auto-resets after 3 minutes (power-saving when away)
#define PANIC_SIREN_TIMEOUT_S   900   // Panic siren runs 15 minutes independently
#define DISARM_LOCKOUT_MS      8000   // 8s lockout — covers AM312 worst-case hold time

// -- Pushover / notifications ------------------------------------------------------
// Get your own free credentials at https://pushover.net :
//   1. Create an account and install the Pushover app on your phone.
//   2. Your "User Key" is shown on the dashboard after login.
//   3. Create an "Application" (Settings > Applications > Create New App)
//      to get an "API Token/Key". Name it whatever you like.
// Paste your own values below. NEVER commit real tokens to a public repo.
#define PO_APP_TOKEN  "YOUR_PUSHOVER_APP_TOKEN_HERE"
#define PO_USER_KEY   "YOUR_PUSHOVER_USER_KEY_HERE"
#define PO_HOST       "api.pushover.net"
#define PO_QUEUE_SIZE 8
#define PO_RETRY_SECS   60     // priority-2 re-alert interval (panic/intruder)
#define PO_EXPIRE_SECS  3600   // priority-2 gives up after this (1 hour)
// Medicine reminder pacing — gentler than panic: re-alert every 5 min for up
// to 30 min, until someone taps Acknowledge in the Pushover app.
#define MED_PUSHOVER        1     // 0 = no Pushover for medicine doses
#define MED_PO_LEAD_MIN     5     // warn the phone this many minutes BEFORE the dose
#define MED_PO_RETRY_SECS   300
#define MED_PO_EXPIRE_SECS  1800

// -- Display / LED hardware --------------------------------------------------------
#define NUM_LEDS     256
#define LED_TYPE     WS2812B
#define COLOR_ORDER  GRB
#define BAR3_NUM_LEDS  30
#define MED_NUM_LEDS  8   // Sunlephant board (we use up to 4 = max doses)
#define PILL_NUM_LEDS 8   // Sunlephant 8 LEDs (we use 7 = Sun-Sat, LED7 off)

// -- Display / LED layout — index positions on the shared BAR3 strip --------------
#define PILL_BASE       0
#define MED_BASE       10
#define FREE_BASE      20
#define BDAY_LED_3   4    // med_leds[4] = 3 days before birthday
#define BDAY_LED_2   5    // med_leds[5] = 2 days before birthday
#define BDAY_LED_1   6    // med_leds[6] = tomorrow / today birthday
#define DOC_LED      7    // pill_leds[7] = doctor appointment indicator

// -- Audio track numbers (DFPlayer SD card) ----------------------------------------
#define MED_TRACK    62   // "Medisyne tyd Ouma!"  — above all chime tracks (max=53)
#define BDAY_TRACK   63   // birthday song
#define DOC_TRACK    64   // "Doktersafspraak vandag!"
#define PILL_TRACK   61   // "Pille geneem!" (PILLS_ENABLED only)

// -- Medicine / birthday / doctor limits ------------------------------------------
#define MED_MAX_DOSES 4   // maximum doses per day
#define MED_START_HH  7   // medicine window start hour (07:00)
#define MED_END_HH   19   // medicine window end hour  (19:00)
#define BDAY_MAX     30   // maximum birthday entries (30 — lots of grandchildren!)
#define DOC_MAX      10   // maximum doctor appointment entries

/******************************************************************************
 * SECURITY ALARM — WIRING NOTES
 ******************************************************************************/
//  GPIO 0, 1 and 3 are NOT used for any peripheral on this board. GPIO0 is
//  the boot-strapping pin (must read HIGH at reset, or the board drops into
//  the serial bootloader); a sensor idling LOW there can prevent boot
//  entirely. GPIO1 carries the boot log at every reset. GPIO3 is driven by
//  the on-board USB-serial chip whenever the board is powered through its
//  USB socket — loading a peripheral onto it caused real, hard-to-diagnose
//  failures (intermittent notification/Wi-Fi problems) during development.
//  Treat all three as reserved for USB/serial only.
//
//  PIR sensors: AM312 — VCC=3.3V, GND=GND, OUT→GPIO (direct, no resistor needed)
//  Siren: 2x piezo tweeter, each via an IRLZ44N MOSFET
//    GPIO5 ──[10KΩ series]──► Gate IRLZ44N #1  Drain→Speaker1(-)  Source→GND
//    GPIO5 ──[10KΩ series]──► Gate IRLZ44N #2  Drain→Speaker2(-)  Source→GND
//    Both Speaker(+) → 12V supply.  Add a 1N4007 diode across each speaker.
//    REQUIRED: one 10KΩ pull-down resistor from the gate side to GND (a
//    single shared resistor covers both gates; ready-made 2-channel MOSFET
//    driver modules usually include one) — this defeats GPIO5's weak
//    internal pull-up at reset so the siren can't chirp during boot.
//  PIR1 (Zone 1 Courtyard A) has no free GPIO left on this board and is
//  parked (see PIR1_PIN in Pin Definitions above) — a PCF8575 I2C expander
//  is the clean way to add it back if you have a free I2C address to spare.

/******************************************************************************
 * GLOBAL VARIABLES — organized by subsystem, subsystem-local sizing
 * constants (array/struct sizing) stay right next to the variables they
 * configure rather than living in the Constants block above.
 ******************************************************************************/

// -- Pushover — runtime state -----------------------------------------------
// Result of the most recent po_send attempt — shown on /alarm and /potest so
// Pushover problems are diagnosable from the phone, no serial cable needed.
String g_po_last = "geen poging nog / no attempt yet";


/******************************************************************************
 * 5x7 FONT — clean outline style (open/hollow, like a 7-segment display)
 ******************************************************************************/
// Indices: 0-9 digits, 10=colon, 11=space, 12=V, 13=dot, 14=dash, 15=C, 16=F
//          17=° degree symbol
// Bit 6 = top row, bit 0 = bottom row. Each byte is one vertical column.
const uint8_t FONT5[18][5] = {
  {0x7F,0x41,0x41,0x41,0x7F},  //  0  full border, hollow centre
  {0x00,0x42,0x7F,0x40,0x00},  //  1  serif top-left + vertical bar + foot
  {0x79,0x49,0x49,0x49,0x4F},  //  2  top-right + mid + bottom-left
  {0x49,0x49,0x49,0x49,0x7F},  //  3  mid + right side full
  {0x0F,0x08,0x08,0x08,0x7F},  //  4  top-left + mid + right
  {0x4F,0x49,0x49,0x49,0x79},  //  5  top-left + mid + bottom-right
  {0x7F,0x49,0x49,0x49,0x79},  //  6  left side + mid + bottom-right
  {0x01,0x01,0x01,0x01,0x7F},  //  7  top bar + right side
  {0x7F,0x49,0x49,0x49,0x7F},  //  8  full border + mid bar
  {0x4F,0x49,0x49,0x49,0x7F},  //  9  top-left + mid + right side
  {0x00,0x36,0x36,0x00,0x00},  // 10  colon — two small dots
  {0x00,0x00,0x00,0x00,0x00},  // 11  space
  {0x3F,0x40,0x40,0x40,0x3F},  // 12  V — outline chevron
  {0x00,0x60,0x60,0x00,0x00},  // 13  dot — bottom area
  {0x08,0x08,0x08,0x08,0x08},  // 14  dash / minus
  {0x38,0x44,0x44,0x44,0x00},  // 15  c — baseline aligned with digits (Oupa's view)
  {0x7F,0x09,0x09,0x01,0x01},  // 16  F — top + mid bars + left side
  {0x06,0x09,0x09,0x06,0x00},  // 17  ° — small open circle
};

// ── Colour themes ─────────────────────────────────────────────────────────────
// Index:  0=clock  1=alarmset  2=egg  3=temp  4=uptime
const uint8_t THR[5] = {255,   0,   0, 220,   0};
const uint8_t THG[5] = {140, 220, 200,   0, 100};
const uint8_t THB[5] = {  0, 220,   0, 220, 255};

// ── FastLED array ─────────────────────────────────────────────────────────────
CRGB leds[NUM_LEDS];
CRGB pill_leds[PILL_NUM_LEDS];   // Sunlephant day-of-week LEDs (mirrored into bar3_leds)
CRGB med_leds[MED_NUM_LEDS];     // Sunlephant medicine dose LEDs (mirrored into bar3_leds)
CRGB bar3_leds[BAR3_NUM_LEDS];   // THE physical 30-LED chain on BAR3_PIN: pills+meds+free/status

// ── Hardware objects ──────────────────────────────────────────────────────────
RTC_DS3231   rtc;
BH1750       lightMeter;
WebServer    webServer(80);

// ── Time globals ──────────────────────────────────────────────────────────────
uint8_t  g_hh = 12, g_mm = 0, g_ss = 0;
uint8_t  g_day = 1, g_month = 1;   // for date display
uint16_t g_year = 2026;            // Full year, kept fresh by rtc_sync()
uint8_t  g_dow  = 0;               // Day of week 0=Sun..6=Sat, cached by rtc_sync()
int16_t  g_tz_offset_hours = 2;    // timezone offset from UTC (e.g. SAST = +2)
bool     g_use_12hr  = false;       // 12hr/24hr display
bool     g_temp_degF = false;       // false=°C (default), true=°F — separate from clock mode
bool     g_ntp_synced = false;     // true after first NTP sync
// Once-per-minute latch, set in the 1s tick — replaces
// all the fragile "g_ss == 0" exact-second triggers. See timekeeping().
bool     g_new_minute = false;


// -- Medicine schedule globals ------------------------------------------------
// Dose times: set via web portal, saved to NVS
// doses_per_day = 0 means medicine finished / no current script
struct MedDose {
  uint8_t hh;       // hour
  uint8_t mm;       // minute
  bool    taken;    // confirmed taken today
};

uint8_t  g_med_doses     = 2;       // doses per day: 0,1,2,3,4
MedDose  g_med[MED_MAX_DOSES] = {   // default schedule
  { 7,  0, false},   // dose 1: 07:00
  {13,  0, false},   // dose 2: 13:00
  {19,  0, false},   // dose 3: 19:00 (used when doses>=3)
  {16,  0, false},   // dose 4: 16:00 (used when doses==4)
};
bool     g_med_btn_prev   = false;  // debounce for medicine button
uint8_t  g_med_alm_active  = 255;   // which dose is currently alarming (255=none)
uint32_t g_med_last_day    = 255;   // day-of-week when med was last reset

// -- Kort Kursus / Short Course globals (LED 7 on Strip 2) --
// TWO independent course slots — Ria's own real case: two different
// antibiotics for pneumonia, on two different schedules, running at the
// same time. One shared course slot couldn't hold both. Same
// pattern as the three Wekkers slots — each course is fully independent:
// own name, own dose count/times, own day countdown. Struct-based so the
// slot count can grow later by just changing COURSE_SLOTS.
#define COURSE_SLOTS     2
#define COURSE_MAX_DOSES 4
struct Course {
  char    name[24];                          // e.g. "Ciprofloxactin 500mg"
  uint8_t doses;                              // 0 = this slot is inactive
  uint8_t hh[COURSE_MAX_DOSES];
  uint8_t mm[COURSE_MAX_DOSES];
  bool    taken[COURSE_MAX_DOSES];
  uint8_t days_total;
  uint8_t days_left;                          // counts down; 0 = inactive/finished
  uint8_t last_day;                           // g_day when this slot last rolled over
};
Course   g_course[COURSE_SLOTS] = {
  { "", 0, {8, 14, 20, 0}, {0,0,0,0}, {false,false,false,false}, 0, 0, 255 },
  { "", 0, {9, 21,  0, 0}, {0,0,0,0}, {false,false,false,false}, 0, 0, 255 },
};
uint8_t  g_course_alm_slot = 255;   // which slot is currently alarming (255=none)
uint8_t  g_course_alm_dose = 255;   // which dose within that slot

// -- Ad-hoc extra dose log ----------------------------------------------------
// Henry's follow-up: the Kort Kursus scheduler (above) covers a real course
// with fixed times, but there's a SEPARATE everyday need it doesn't cover —
// an unscheduled one-off, e.g. "Ouma needs an extra painkiller right now."
// No time to set, no multi-day count, just "log that this happened today."
// This is what the old single on/off toggle actually did, restored here as
// its own simple log rather than folded into the scheduled course.
#define ADHOC_MAX_LOG 6
uint8_t  g_adhoc_count    = 0;               // how many logged today
uint8_t  g_adhoc_hh[ADHOC_MAX_LOG];          // time of each log entry
uint8_t  g_adhoc_mm[ADHOC_MAX_LOG];
uint8_t  g_adhoc_last_day = 255;             // day-of-week reset detector, same pattern as med_check_reset

// ── Birthday Calendar globals ─────────────────────────────────────────────────
struct Birthday {
  uint8_t  day;          // 1–31
  uint8_t  month;        // 1–12
  char     name[14];     // first name, max 13 chars + null
};
Birthday g_bdays[BDAY_MAX];
uint8_t  g_bday_count       = 0;    // how many entries are set
bool     g_bday_today       = false; // someone has a birthday today
uint8_t  g_bday_today_idx   = 255;  // index of today's birthday (255 = none)
bool     g_bday_acked        = false; // accept button pressed today — stop flashing
uint8_t  g_bday_last_day    = 255;  // day-of-month when we last checked/reset ack
bool     g_bday_played      = false; // MP3 already played today
bool     g_bday_btn_prev    = false; // debounce for BTN_BDAY

// ── Doctor Appointment Calendar globals ───────────────────────────────────────
struct DocAppt {
  uint8_t  day;          // 1–31
  uint8_t  month;        // 1–12
  char     desc[16];     // short description e.g. "Dr Botha" max 15 chars
};
DocAppt  g_docs[DOC_MAX];
uint8_t  g_doc_count      = 0;    // how many appointments are set
bool     g_doc_today      = false; // appointment today
uint8_t  g_doc_today_idx  = 255;  // index of today's appointment
bool     g_doc_acked      = false; // accept button pressed today
uint8_t  g_doc_last_day   = 255;  // day-of-month when ack last reset
bool     g_doc_played     = false; // MP3 already played today
bool     g_doc_warned     = false; // evening-before warning already played

// ── Kalender general-event globals ──────────────────
// General events ("Ria vergadering", "kar diens") for the /calendar month
// grid. Same day/month + auto-purge pattern as doctor appointments — no year
// stored, past events purge once the date wraps. LED: BAR3 slot 28.
#define EVT_MAX 20
struct Evt {
  uint8_t  day;          // 1–31
  uint8_t  month;        // 1–12
  char     desc[24];     // short description, max 23 chars + null
};
Evt      g_events[EVT_MAX];
uint8_t  g_evt_count      = 0;
bool     g_evt_today      = false;  // any event today (drives LED pulse)
bool     g_evt_acked      = false;  // accept button pressed today — stop flashing
int16_t  g_evt_days_min   = 999;    // days to the NEAREST upcoming event
uint8_t  g_evt_last_day   = 255;    // new-day detector for purge + notify reset
bool     g_evt_notified   = false;  // 07:00 Pushover sent today

// ── Notaboekie globals (Ouma's request) ─────────────────────────────────────
// Small shopping/reminder notes: tap ✓ to strike through when bought, X to
// delete, "Vee gedoen uit" clears all completed. NVS-persisted like all else.
#define NOTE_MAX 20
struct Note {
  char text[41];   // max 40 chars
  bool done;
};
Note     g_notes[NOTE_MAX];
uint8_t  g_note_count = 0;

// -- Pill reminder globals ---------------------------------------------------
bool     g_pill_taken     = false;   // did Ouma take her pills today?
uint8_t  g_pill_last_day  = 255;     // day-of-week when pill was last confirmed
uint8_t  g_pill_alm_hh    = 8;       // pill reminder alarm hour
uint8_t  g_pill_alm_mm    = 0;       // pill reminder alarm minute
bool     g_pill_alm_en    = true;    // pill reminder alarm enabled
bool     g_pill_alm_fire  = false;   // pill reminder currently firing
bool     g_pills_active   = true;    // true = Ouma must take daily pills (set via web portal)
uint32_t g_pill_pulse_ms  = 0;       // for pulsing the LEDs during alarm
bool     g_pill_btn_prev  = false;   // debounce for pill button

// ── WiFi/network reset button (repurposed GPIO2/BTN_PILL, pill subsystem
// retired — see wifi_reset_check_button()) ─────────────────────────────────
bool     g_wifi_btn_prev     = false;   // debounce
uint32_t g_wifi_btn_down_ms  = 0;       // millis() when button was first pressed (0 = not held)
bool     g_wifi_btn_fired    = false;   // true once this press has already triggered a restart

// -- Alarm ----------------------------------------------------------------------
// A single daily wake alarm — no day-of-week mask, fires every day.
struct Alarm {
  uint8_t hh;
  uint8_t mm;
  bool    enabled;
};
Alarm g_alarm1 = {6, 30, true};

// -- Wekkers — three independent daily alarms, separate from the wake alarm ------
// above. Each has its own time, its own day-of-week mask, and its own
// enabled state. A firing Wekker repeats every WEKKER_REPEAT_MS (0002.mp3 is
// short enough to miss on a single play) until acknowledged via BTN_BDAY —
// the same accept/dismiss button shared with birthdays and doctor
// appointments — or until WEKKER_MAX_REPEATS is reached.
bool    g_wekker_enabled[NUM_ALARM_SLOTS]     = {false, false, false};
uint8_t g_wekker_hh[NUM_ALARM_SLOTS]          = {6, 7, 8};
uint8_t g_wekker_mm[NUM_ALARM_SLOTS]          = {0, 0, 0};
uint8_t g_wekker_days[NUM_ALARM_SLOTS]        = {0x7F, 0x7F, 0x7F};
bool    g_wekker_fired_today[NUM_ALARM_SLOTS] = {false, false, false};
#define WEKKER_REPEAT_MS     20000UL   // 20 seconds between repeats
#define WEKKER_MAX_REPEATS   6         // give up after this many repeats (~2 min)
bool     g_wekker_ringing      = false;  // true from first fire until accepted/expired
uint8_t  g_wekker_repeat_count = 0;      // how many times it has played this ringing session
uint32_t g_wekker_last_play_ms = 0;      // millis() of the most recent play
uint8_t g_wekker_slot = 0;   // which slot is being edited on the web page

// ── Flags ─────────────────────────────────────────────────────────────────────
volatile bool g_colon     = false;
bool g_alm_fire   = false;
uint32_t g_alm_fire_ms = 0;   // millis() when g_alm_fire became true — for 3-min auto-stop
volatile bool g_force_redraw = false;  // set true to skip render() 100ms throttle once
bool g_alm_armed  = true;
bool g_chime_en   = true;
bool g_chime_req  = false;
bool g_show_set   = false;
bool g_show_date  = false;

// ── Egg timer ─────────────────────────────────────────────────────────────────
bool     g_egg_run     = false;
bool     g_egg_done    = false;   // finished, waiting to be dismissed — separate from the main alarm
uint32_t g_egg_done_ms = 0;       // millis() when it finished, for the 10s auto-clear
uint8_t  g_egg_set_mm = 5, g_egg_set_ss = 0;   // remembered preference — saved to NVS
uint8_t  g_egg_mm = 5, g_egg_ss = 0;           // live countdown — NOT saved, resets on reboot

// ── Smoker Uptimer ───────────────────────────────────────────────────────────
// Counts UP, not down — for tracking how long meat's been in the smoker.
// RTC-based (not millis()-based) and saved to NVS on every state change, so a
// power cut mid-smoke (loadshedding!) doesn't lose the elapsed time — on next
// boot it just keeps counting from where it actually is, using real wall-clock
// time from the battery-backed DS3231, not time since the ESP32 last rebooted.
bool     g_smoke_running      = false;
bool     g_smoke_paused       = false;
uint32_t g_smoke_start_unix   = 0;   // rtc.now().unixtime() when Start pressed
uint32_t g_smoke_pause_unix   = 0;   // rtc.now().unixtime() when Pause pressed
uint32_t g_smoke_paused_accum = 0;   // total seconds spent paused so far
String   g_smoke_note         = "";  // optional: "3kg pork leg" etc.
uint32_t g_smoke_last_reminder = 0;  // which 30-min boundary last triggered a beep
bool     g_smoke_peek_active   = false;  // quick-look overlay currently showing on the matrix
uint32_t g_smoke_peek_end_ms   = 0;      // millis() when the overlay should auto-clear

// ── Braai Timer (Ouma's idea!) ────────────────────────────────────────────────
// Counts DOWN from a selected total cook time, with a repeating turn-reminder
// at a selected interval (e.g. "turn every 1 minute"). Same RTC-based survive-
// reboot pattern as the Smoker Uptimer above — a power blip mid-braai doesn't
// lose the countdown. Plays 0025.mp3 on every turn boundary, and a distinct
// "done" sound (track 2, the egg-timer alert) when the total time is up, so
// the two cues sound different by ear.
bool     g_braai_enabled      = false;  // tickbox — must be on for the page/timer to run
bool     g_braai_running      = false;
bool     g_braai_paused       = false;
uint32_t g_braai_start_unix   = 0;      // rtc.now().unixtime() when Start pressed
uint32_t g_braai_pause_unix   = 0;      // rtc.now().unixtime() when Pause pressed
uint32_t g_braai_paused_accum = 0;      // total seconds spent paused so far
uint16_t g_braai_total_sec    = 600;    // selected total cook time, default 10 min
uint16_t g_braai_turn_sec     = 60;     // selected turn interval, default 1 min
uint32_t g_braai_last_turn    = 0;      // which turn-interval boundary last triggered a beep
bool     g_braai_done_played  = false;  // "time's up" sound already played this session

// ── Chime ─────────────────────────────────────────────────────────────────────
uint8_t g_chime_done_mm = 61;
// Voice-priority lock. Any spoken announcement (medisyne,
// dokter, verjaarsdag, pille, egg alert) records its minute here; the chime
// yields for that minute instead of cutting the voice off mid-word (the
// DFPlayer obeys whoever spoke LAST, and the chime request was consumed after
// the announcements in the same tick — so the chime always won). 61 = none.
// Cleared automatically as soon as the minute moves on, so a med dose at
// 08:00 doesn't wrongly silence the 09:00 chime as well.
uint8_t g_voice_lock_mm = 61;
inline void voice_lock(void) { g_voice_lock_mm = g_mm; }
uint8_t g_chime_track   = 0;

// ── Sound & volume ────────────────────────────────────────────────────────────
uint8_t g_sound_set  = 0;
uint8_t g_vol        = 20;
uint8_t g_night_vol  = 8;
// Alarm volume — separate from day/night ambient volume so a quiet
// night/day setting never makes Alarm or Wekkers too soft to actually wake
// you. Used by both Alarm and Wekkers, including their repeat loops — day/
// night volume is ignored for these specifically.
uint8_t g_alarm_vol  = 25;
uint8_t g_show_timer = 0;

// ── Mode ──────────────────────────────────────────────────────────────────────
uint8_t g_mode = 0;
// 0=clock 1=alarm-set 2=egg-run 3=egg-set 4=temperature 5=uptime

// ── Brightness ────────────────────────────────────────────────────────────────
// Two independent values — see update_brightness_smooth() and leds_show_all()
// for why. g_bright/g_bright_target = MAIN DISPLAY, still BH1750 lux-driven.
// g_bar3_bright/g_bar3_bright_target = BAR3 (pills/meds/status), fixed
// day/night schedule via g_bright_day/g_bright_night.
uint8_t g_bright        = 120;
uint8_t g_bright_target = 120;  // smooth fade target
uint8_t g_bar3_bright        = 120;
uint8_t g_bar3_bright_target = 120;
uint8_t g_bright_day    = 180;
uint8_t g_bright_night  = 40;

// ── Idle / rainbow ────────────────────────────────────────────────────────────
uint32_t g_last_interaction_ms = 0;
bool     g_rainbow_active      = false;
uint8_t  g_rainbow_hue         = 0;

// ── Snooze ────────────────────────────────────────────────────────────────────
bool    g_snooze_active   = false;
uint8_t g_snooze_hh       = 0;
uint8_t g_snooze_mm       = 0;

// ── Security alarm globals ────────────────────────────────────────────────────
// Sensor arrays — Zone 1 first (indices 0..ZONE1_COUNT-1), Zone 2 after
const uint8_t PIR_PINS[PIR_COUNT]  = {PIR1_PIN, PIR2_PIN, PIR3_PIN, PIR4_PIN, PIR5_PIN};
const char* PIR_NAMES[PIR_COUNT]   = {"Courtyard A","Courtyard B","Courtyard C","Front door","Passage"};
const char* PIR_ZONES[PIR_COUNT]   = {"Zone 1","Zone 1","Zone 1","Zone 2","Zone 2"};

// Single guarded PIR reader — a slot whose pin is
// PIR_PARKED (255, currently Courtyard A) always reads "no motion" instead of
// digitalRead()-ing a pin that doesn't exist. All of check_pir() goes
// through here, so re-homing a sensor later (e.g. onto the PCF8575) only
// needs this one function to change.
static inline bool pir_read(uint8_t i) {
  if (PIR_PINS[i] == PIR_PARKED) return false;
  return digitalRead(PIR_PINS[i]) == HIGH;
}

// ── Alarm state machine ───────────────────────────────────────────────────────
// Zone 1 (outdoor courtyard) — always armed 24/7, fires siren immediately
// Zone 2 (indoor)            — armed only when away, has exit+entry delay
bool     g_z1_active       = true;   // Zone 1 always on (set false to suspend)
bool     g_z2_armed        = false;  // Zone 2 armed (away mode)

// Shared siren state
bool     g_sec_triggered   = false;  // siren is wailing
uint8_t  g_trig_zone       = 0;      // which zone triggered (1 or 2)
uint8_t  g_trig_sensor     = 0;      // which sensor index triggered

// Zone 2 exit/entry delay
bool     g_sec_exit_delay  = false;  // true = counting down exit delay
bool     g_sec_entry_delay = false;  // true = motion seen, grace period running
uint32_t g_sec_arm_ms      = 0;      // millis() when Zone 2 armed
uint32_t g_sec_entry_ms    = 0;      // millis() when entry delay started

// Disarm flag — set by web handler, cleared by check_pir on next loop pass
// This fixes the bug where disarm during exit delay was ignored
bool     g_disarm_requested = false;

// Panic button — set by web handler, fires siren immediately, zone 0 = panic
bool     g_panic_requested  = false;

// Auto-reset: timestamp when siren started — resets after ALARM_SIREN_TIMEOUT_S
uint32_t g_siren_start_ms   = 0;

// Panic has its own independent siren timer (15 min, manual stop only)
bool     g_panic_active     = false;   // true = panic siren running
uint32_t g_panic_start_ms   = 0;       // millis() when panic was pressed

// Post-disarm lockout — prevents PIR retriggering immediately after disarm
// (AM312 holds its output HIGH for ~2s after movement stops)
uint32_t g_disarm_lockout_ms = 0;      // millis() when disarm completed; 0 = no lockout

// ── Uptime ────────────────────────────────────────────────────────────────────
uint32_t g_boot_time_unix = 0;   // unix timestamp at boot (set after NTP sync)

// ── Temperature flip (auto-show temp for 3s, 5s after each minute tick) ──────
bool     g_temp_flip_active = false;   // currently showing temp overlay in clock mode
uint32_t g_temp_flip_end_ms = 0;       // millis() when the flip ends
uint8_t  g_last_flip_mm     = 255;     // Half-minute key (mm*2+half, 0-119; 255=never) we last triggered on

// ── Timing ────────────────────────────────────────────────────────────────────
uint32_t g_last_sec_ms   = 0;
uint32_t g_last_half_ms  = 0;
uint32_t g_last_btn_ms   = 0;
uint32_t g_last_ntp_sync = 0;   // millis() of last NTP write to RTC
// Live internet-reachability indicator for BAR3 LED 27.
// WiFi-connected only means the router link is up; it says nothing about the
// fibre/uplink being alive (loadshedding: router on UPS, ONT dead). This
// tracks whether the ESP32 could actually reach the outside world recently.
bool     g_inet_ok       = false;   // last reachability probe result
uint32_t g_last_inet_chk = 0;       // millis() of last probe
bool     g_dfp_alive     = false;   // did the DFPlayer respond to the boot diagnostic?
                                     // shown permanently on the free bar — no serial needed
uint32_t g_last_wifi_retry = 0;     // millis() of last mid-session reconnect attempt
// True while an OTA update is receiving — loop() on
// Core 1 stands down from all rendering so Core 0's OTA progress bar owns
// FastLED exclusively (see the render-race note at ArduinoOTA.onStart).
volatile bool g_ota_active = false;
uint8_t  g_rtc_tick      = 0;
uint8_t  g_lux_tick      = 0;

// ── Button hold-to-repeat ─────────────────────────────────────────────────────
uint32_t g_hold_ms  = 0;
uint8_t  g_hold_btn = 0;

// ── Button debounce ───────────────────────────────────────────────────────────
uint16_t btn_prev = 0xFFFF;

// ── Button bit masks ─────────────────────────────────────────────────────────
#define BMASK_MODE    0x001
#define BMASK_SET     0x002
#define BMASK_UP      0x004
#define BMASK_ALM     0x008
#define BMASK_CHIME   0x010
#define BMASK_CVUP    0x020
#define BMASK_CVDN    0x040
#define BMASK_SOUND   0x080
#define BMASK_ALMSEL  0x100

// ── FreeRTOS task handle ──────────────────────────────────────────────────────
TaskHandle_t h_wifi_task = NULL;

// ── Pushover priority queue ────────────────────────────────────────────────────
//
// THE PROBLEM THIS FIXES: with only one pending slot, a high-frequency caller
// (courtyard PIR checks, run every loop() pass) could silently overwrite a
// Braai/Smoker notification that Core 0 hadn't sent yet — message lost with
// no error. This is why Braai turn reminders sometimes arrived 4-5 minutes
// late: the one that *did* arrive was actually a *later* event that won the
// race, not the original one delayed.
//
// THE FIX: a small fixed-size ring buffer holds several pending messages.
// Core 0's drain loop always sends the HIGHEST-PRIORITY pending message first
// (FIFO within the same priority level). This guarantees Panic/Intruder/Alarm
// (priority 1) NEVER sit behind Braai/Smoker (priority 0) chatter — they jump
// the queue every time. If the queue is ever full, we drop the OLDEST
// priority-0 message to make room — a priority-1 message is NEVER dropped to
// make space, and a priority-1 message NEVER gets evicted by anything.
//
// Core 1 (loop(), any web handler) calls po_notify() — same signature as
// before, nothing else in the firmware needs to change.

struct PoMsg {
  bool     used;
  uint8_t  priority;
  char     title[48];
  char     body[160];
  char     sound[24];
  uint16_t retry;    // Per-message priority-2 pacing
  uint16_t expire;
};
PoMsg        g_po_queue[PO_QUEUE_SIZE];
portMUX_TYPE g_po_mux = portMUX_INITIALIZER_UNLOCKED;

// Call from ANY core — writes notification into the queue and returns immediately
// sound: a Pushover built-in sound name (see https://pushover.net/api#sounds), or
// "" (default) to use the user's own chosen default tone.
void po_notify(const String& title, const String& msg, uint8_t pri, const String& sound, uint16_t retry, uint16_t expire) {
  portENTER_CRITICAL(&g_po_mux);

  // Find a free slot
  int slot = -1;
  for (int i = 0; i < PO_QUEUE_SIZE; i++) {
    if (!g_po_queue[i].used) { slot = i; break; }
  }

  // Queue full — make room WITHOUT EVER touching a priority-1 (alarm/panic)
  // message. Only ever evict the oldest priority-0 slot. If every slot is
  // priority-1 (should be essentially impossible in practice), the new
  // message is dropped rather than risk losing an alarm event — this is the
  // one case where silently dropping is the SAFER failure mode.
  if (slot == -1) {
    for (int i = 0; i < PO_QUEUE_SIZE; i++) {
      if (g_po_queue[i].priority == 0) { slot = i; break; }
    }
  }

  if (slot != -1) {
    g_po_queue[slot].used     = true;
    g_po_queue[slot].priority = pri;
    strncpy(g_po_queue[slot].title, title.c_str(), 47); g_po_queue[slot].title[47] = 0;
    strncpy(g_po_queue[slot].body,  msg.c_str(),  159); g_po_queue[slot].body[159] = 0;
    strncpy(g_po_queue[slot].sound, sound.c_str(), 23); g_po_queue[slot].sound[23] = 0;
    g_po_queue[slot].retry  = retry;   // G_po_queue[slot].expire = expire;
  } else {
    Serial.println("Pushover: queue full of priority-1 messages — dropped one (should not happen)");
  }

  portEXIT_CRITICAL(&g_po_mux);
}

// Call from Core 0 only — finds and claims the highest-priority pending
// message (FIFO within the same priority), copies it out, and frees the slot.
// Returns true if a message was found and copied into the out-params.
bool po_queue_pop(uint8_t& pri, String& title, String& body, String& sound, uint16_t& retry, uint16_t& expire) {
  portENTER_CRITICAL(&g_po_mux);
  int best = -1;
  for (int i = 0; i < PO_QUEUE_SIZE; i++) {
    if (g_po_queue[i].used) {
      if (best == -1 || g_po_queue[i].priority > g_po_queue[best].priority) {
        best = i;
      }
    }
  }
  bool found = (best != -1);
  if (found) {
    pri   = g_po_queue[best].priority;
    title = String(g_po_queue[best].title);
    body  = String(g_po_queue[best].body);
    sound = String(g_po_queue[best].sound);
    retry  = g_po_queue[best].retry;    // Expire = g_po_queue[best].expire;
    g_po_queue[best].used = false;
  }
  portEXIT_CRITICAL(&g_po_mux);
  return found;
}

// ── Thread-safe deferred beep request ─────────────────────────────────────────
// beep_single()/double()/triple() use tone()+delay() and block for 80-500ms.
// Calling them directly from a web handler (Core 0) stalls webServer.handleClient()
// for that long, delaying the HTTP response back to the phone/browser — this is
// what made Panic/Arm/Disarm feel slow or need repeat taps after WiFi sat idle.
// Fix: web handlers just set a request flag here (near-instant), and loop() on
// Core 1 does the actual beeping a few ms later.
enum BeepKind : uint8_t { BEEP_NONE = 0, BEEP_SINGLE, BEEP_DOUBLE, BEEP_TRIPLE };
volatile uint8_t g_beep_pending = BEEP_NONE;   // BeepKind value, or BEEP_NONE

// Call from ANY core (web handlers included) — returns immediately
void request_beep(uint8_t kind) {
  g_beep_pending = kind;   // last request wins; fine for a UI confirmation beep
}

/******************************************************************************
 * PREFERENCES — NVS (non-volatile) storage, replaces EEPROM
 ******************************************************************************/
// One shared Preferences object, reused across every subsystem's save/load
// functions. Each subsystem opens its OWN namespace (a short string, e.g.
// "pill", "med", "wekker", "course") via prefs.begin("namespace", readOnly)
// before reading or writing, and calls prefs.end() when done — so settings
// from different subsystems never collide, even though they all share this
// one object. Everything stored here survives a reboot, a power cut, and an
// OTA firmware update; nothing here needs the SD card or SPIFFS.
Preferences  prefs;

/******************************************************************************
 * FUNCTION PROTOTYPES
 ******************************************************************************/
void settings_save(void);
void settings_load(void);
void render(void);
void web_setup(void);
void rtc_sync(void);
void ntp_sync_to_rtc(void);
void inet_check(void);   // Bounded internet reachability probe
bool is_night_now(void);
void dfplayer_volume(uint8_t v);
void dfplayer_play(uint16_t t);
void dfplayer_stop(void);
void compose_bar3(void);
void leds_show_all(void);
void play_chime_with_vol(uint16_t track);
uint16_t chime_track_for_set(uint8_t ss, uint8_t q);
void update_brightness_smooth(void);
void splash_screen(void);
void check_factory_reset(void);
void factory_reset_defaults(void);
uint32_t smoke_elapsed_sec(void);
uint32_t braai_elapsed_sec(void);
uint32_t braai_remaining_sec(void);
String html_header(const char* title);
String html_footer(void);
void doc_save(void);
void doc_load(void);
void doc_purge_past(void);
void doc_check(void);
void doc_render_led(void);
void evt_save(void);       // Kalender events
void evt_load(void);
void evt_purge_past(void);
void evt_check(void);
void course_check_reset(uint8_t s);  // Kort Kursus (short course) — per slot
uint8_t course_due_dose(uint8_t s);
void adhoc_log(void);   // Ad-hoc extra dose log
void course_check(void);

// ── Security alarm forward declarations ──────────────────────────────────────
void sec_alarm_check_pir(void);
void sec_alarm_siren_update(void);
void sec_zone2_arm(void);
void sec_zone2_disarm(void);
void sec_full_disarm(void);
void sec_alarm_web_page(void);
void sec_panic(void);
void beep_single(void);
void beep_double(void);
void beep_triple(void);
void po_send(const String& title, const String& msg, uint8_t priority=0, const String& sound="", uint16_t retry=PO_RETRY_SECS, uint16_t expire=PO_EXPIRE_SECS);  // Pushover (Core 0 only)
void po_notify(const String& title, const String& msg, uint8_t pri = 0, const String& sound = "", uint16_t retry = PO_RETRY_SECS, uint16_t expire = PO_EXPIRE_SECS);  // Pushover (any core safe)
bool po_queue_pop(uint8_t& pri, String& title, String& body, String& sound, uint16_t& retry, uint16_t& expire);  // Pushover (Core 0 only)

/******************************************************************************
 * SETUP & MAIN LOOP
 * (all subsystem functions below are forward-declared above and implemented
 *  further down the file)
 ******************************************************************************/

/******************************************************************************
 * SETUP
 ******************************************************************************/
void setup(void) {
  Serial.begin(115200);
  delay(1000);   // give USB-serial enumeration a moment to settle after reset
  Serial.println();
  Serial.println("===== SETUP START =====");
  Serial.println(FIRMWARE_VERSION);

  // Button pins
  uint8_t pullup_btns[] = {BTN_MODE, BTN_SET, BTN_UP, BTN_ALM,
                            BTN_CHIME, BTN_CVUP, BTN_CVDN};
  for(uint8_t i = 0; i < 7; i++) pinMode(pullup_btns[i], INPUT_PULLUP);
  // GPIO34 & 35 = input-only, external 10K pullup required
  pinMode(BTN_SOUND,  INPUT);
  pinMode(BTN_ALMSEL, INPUT);
  pinMode(BTN_PILL,   INPUT_PULLUP);  // Ouma pill confirm button — also: hold 3s = WiFi/network reset
  pinMode(BTN_MED,    INPUT_PULLUP);  // Ouma medicine confirm button
  pinMode(BTN_BDAY,   INPUT_PULLUP);  // Birthday accept button

  // ── Security alarm pins ───────────────────────────────────────────────────
  // INPUT_PULLDOWN holds pins firmly LOW when no sensor connected (prevents
  // floating inputs from triggering false alarms during testing).
  // When real AM312 sensors are wired: their OUT pin actively drives HIGH/LOW
  // so the pulldown is simply overridden by the sensor — no change needed.
  if (PIR1_PIN != PIR_PARKED) pinMode(PIR1_PIN, INPUT_PULLDOWN);  // Parked, no GPIO
  pinMode(PIR2_PIN, INPUT_PULLDOWN);
  // GPIO36 and GPIO39 are input-only silicon — NO internal pulldown available
  // Wire a 10K resistor from each pin to GND on the PCB for those two
  // or leave floating — AM312 will drive them properly when connected
  pinMode(PIR5_PIN, INPUT_PULLDOWN);
  pinMode(SIREN_PIN, OUTPUT);
  digitalWrite(SIREN_PIN, LOW);  // make sure siren is off on boot

  // ── Boot lockout — prevents false Zone 1 alarm on power-up ───────────────
  // AM312 PIR sensors take 30-60 seconds to stabilise after power-on
  // Without lockout the floating or warm PIR output triggers Zone 1 immediately
  // Lockout gives 8 seconds for PIRs to settle before monitoring starts
  g_disarm_lockout_ms = millis();
  Serial.println("Boot lockout active — PIRs settling for 8 seconds...");
  Serial.println("[checkpoint] pins + lockout done");

  // FastLED
  FastLED.addLeds<LED_TYPE, WS_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.addLeds<LED_TYPE, BAR3_PIN, COLOR_ORDER>(bar3_leds, BAR3_NUM_LEDS);
  FastLED.setCorrection(TypicalLEDStrip);
  FastLED.setDither(0);
  // NOTE: this global brightness is effectively inert from here on — every
  // actual frame is pushed via leds_show_all()'s FastLED[0].showLeds(g_bright)
  // and FastLED[1].showLeds(g_bar3_bright), which scale each strip
  // independently. Left at a sane full-scale default just in case anything
  // ever calls FastLED.show() directly without going through leds_show_all().
  FastLED.setBrightness(255);
  FastLED.clear(true);
  Serial.println("[checkpoint] FastLED init done");

  // I2C — Wire uses GPIO21/22 by default on ESP32
  Wire.begin();
  Serial.println("[checkpoint] Wire.begin done");

  // DS3231 RTC
  if(!rtc.begin()) {
    Serial.println("DS3231 not found — check wiring!");
    // Flash red 5 times as error indicator
    for(uint8_t i = 0; i < 10; i++) {
      fill_solid(leds, NUM_LEDS, (i&1) ? CRGB::Black : CRGB(180,0,0));
      leds_show_all(); delay(200);
    }
  }
  if(rtc.lostPower()) {
    Serial.println("RTC lost power — time is not set. NTP will fix it.");
    rtc.adjust(DateTime(2025, 1, 1, 0, 0, 0));
  }
  Serial.println("[checkpoint] RTC init done");

  // Load settings FIRST — timezone and display preferences needed before rtc_sync
  settings_load();   // g_use_12hr and g_temp_degF both load from NVS, independently

  // Always boot in Celsius, regardless of what was last saved.
  // Reasoning: the long-press BTN_MODE shortcut (see ~line 3391) can silently
  // flip AND SAVE g_temp_degF with only a brief white LED flash as feedback —
  // easy to trigger by accident during normal use, easy to miss when it
  // happens. Rather than risk Ouma seeing °F after a power event (especially
  // during load-shedding, when Henry isn't there to fix it for her), every
  // boot now forces Celsius regardless of whatever was last saved. The
  // long-press shortcut still works during the session if ever needed —
  // it's just no longer persistent across a power cycle.
  g_temp_degF = false;
  pill_load();
  pill_check_reset();
#if !PILLS_ENABLED
  // Daily-pills retired — force the subsystem's existing "no pills"
  // mode regardless of what NVS says. Day strip + doctor LED keep working;
  // pill alarm can never fire; the /pills page that could re-enable it is
  // compiled out.
  g_pills_active = false;
  g_pill_alm_en  = false;
#endif
  med_load();
  med_check_reset();
  bday_load();
  doc_load();
  evt_load();    // Kalender events
  note_load();   // Notaboekie
  wekker_load();   // Wekkers restored from NVS after reboot
  Serial.println("[checkpoint] settings + pill/med/bday/doc/wekker load done");

  // Now sync RTC with correct timezone already loaded
  rtc_sync();
  Serial.println("[checkpoint] rtc_sync done");

  // BH1750 light sensor — auto brightness
  Wire.beginTransmission(0x23);
  uint8_t err23 = Wire.endTransmission();
  Wire.beginTransmission(0x5C);
  uint8_t err5C = Wire.endTransmission();
  if(err23 == 0) {
    lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23);
    Serial.println("BH1750 found at 0x23 — auto brightness active");
  } else if(err5C == 0) {
    lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x5C);
    Serial.println("BH1750 found at 0x5C — auto brightness active");
  } else {
    Serial.println("BH1750 NOT found — check wiring!");
    lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23);
  }
  Serial.println("[checkpoint] BH1750 init done");

  // Factory reset check (hold SET+UP at boot)
  check_factory_reset();
  Serial.println("[checkpoint] factory reset check done");

  // Settings already loaded above — splash, DFPlayer, WiFi next

  // Splash
  splash_screen();
  Serial.println("[checkpoint] splash_screen done");

  // DFPlayer on UART2
  Serial2.begin(9600, SERIAL_8N1, DFP_RX_PIN, DFP_TX_PIN);
  delay(1500);
  Serial.println("[checkpoint] Serial2.begin + 1.5s settle done");

  // ── Explicitly select TF (SD) card as playback device ──────────────────────
  // CONFIRMED FIX: this clone DFPlayer module returns cmd=0x40 error
  // (param=0x7E) on every play command until told explicitly which device
  // to use, even though TF card is supposed to be the default. Standalone
  // testing on 2026-06-17 confirmed adding this resolves playback.
  // SAFETY: every flush/listen loop below is hard time-bounded — a noisy or
  // floating RX line (loose wire, bars not yet connected) could otherwise
  // make available() stay true forever and hang setup() before WiFi/loop()
  // ever start. Added 2026-06-21 after a real freeze traced to this risk.
  {
    uint32_t fs = millis();
    while(Serial2.available() && (millis() - fs < 50)) Serial2.read();
  }
  dfp_send(0x09, 0, 2);   // 2 = TF card
  delay(500);

  // ── DFPlayer diagnostic — query module, listen for reply ──────────────────
  Serial.println("--- DFPlayer diagnostic ---");
  {
    uint32_t fs = millis();
    while(Serial2.available() && (millis() - fs < 50)) Serial2.read();
  }
  dfp_send(0x3F, 0, 0);   // query online storage devices
  uint32_t dfp_wait_start = millis();
  bool dfp_responded = false;
  uint8_t dfp_buf[10]; uint8_t dfp_idx = 0;
  while(millis() - dfp_wait_start < 800) {
    uint32_t inner_start = millis();
    while(Serial2.available() && (millis() - inner_start < 50)) {
      uint8_t b = Serial2.read();
      if(dfp_idx == 0 && b != 0x7E) continue;
      if(dfp_idx < 10) dfp_buf[dfp_idx++] = b;
      if(dfp_idx == 10 && dfp_buf[9] == 0xEF) {
        dfp_responded = true;
        Serial.printf("DFPlayer replied: cmd=0x%02X param=0x%02X%02X\n",
                       dfp_buf[3], dfp_buf[5], dfp_buf[6]);
      }
    }
    if(dfp_responded) break;
  }
  if(!dfp_responded) {
    Serial.println("DFPlayer: NO RESPONSE — check RX/TX wiring, power, or SD card!");
  } else {
    Serial.println("DFPlayer: module is alive and responding to commands");
  }

  // ── NEW DIAGNOSTIC (2026-06-23) — ask the module directly how many files
  // its own internal index actually found on the card, separate from whether
  // Windows shows 27 files. This is command 0x48 in the DFPlayer protocol —
  // the reply's param (paramH/paramL as a 16-bit value) is the file count
  // the chip itself believes are playable. If this comes back as 2 (or any
  // number well under 27) while the PC confirms 27 valid files exist, that
  // proves the module's own directory walk is stopping early — a real,
  // hardware/firmware-internal indexing limitation, not a wiring or file
  // corruption problem.
  Serial.println("--- DFPlayer file count query (cmd 0x48) ---");
  {
    uint32_t fs = millis();
    while(Serial2.available() && (millis() - fs < 50)) Serial2.read();
  }
  dfp_send(0x48, 0, 2);   // 2 = query track count on TF card
  uint32_t count_wait_start = millis();
  bool count_responded = false;
  uint8_t cnt_buf[10]; uint8_t cnt_idx = 0;
  while(millis() - count_wait_start < 800) {
    uint32_t inner_start = millis();
    while(Serial2.available() && (millis() - inner_start < 50)) {
      uint8_t b = Serial2.read();
      if(cnt_idx == 0 && b != 0x7E) continue;
      if(cnt_idx < 10) cnt_buf[cnt_idx++] = b;
      if(cnt_idx == 10 && cnt_buf[9] == 0xEF) {
        count_responded = true;
        uint16_t file_count = ((uint16_t)cnt_buf[5] << 8) | cnt_buf[6];
        Serial.printf("DFPlayer reports %u file(s) found on card (expected 27)\n", file_count);
      }
    }
    if(count_responded) break;
  }
  if(!count_responded) {
    Serial.println("DFPlayer: file-count query got no reply (older firmware on the clone chip may not support cmd 0x48)");
  }
  Serial.println("---------------------------");
  g_dfp_alive = dfp_responded;   // shown permanently on free bar — no serial needed
  Serial.println("[checkpoint] DFPlayer diagnostic done");

  dfplayer_volume(g_vol);
  Serial.println("[checkpoint] dfplayer_volume set");

  // Start WiFi / NTP / OTA / Web on core 0 (Arduino loop runs on core 1)
#if ENABLE_WIFI
  xTaskCreatePinnedToCore(wifi_task, "wifi", 8192, NULL, 1, &h_wifi_task, 0);
  Serial.println("[checkpoint] wifi_task created on core 0");
#else
  Serial.println("*** ENABLE_WIFI=0 — WiFi/NTP/OTA/web portal DISABLED for this test ***");
#endif

  g_last_interaction_ms = millis();
  Serial.println("===== SETUP COMPLETE — entering loop() =====");
}

/******************************************************************************
 * MAIN LOOP — runs on Core 1 at full speed; WiFi/OTA/Pushover run on Core 0
 ******************************************************************************/
void loop(void) {
  uint32_t now_ms = millis();

  // OTA render-race guard. While an OTA update is
  // receiving, Core 0's progress callbacks own FastLED — everything below
  // (renders, button flashes, alarms) stands down until the update finishes
  // (device reboots) or fails (flag cleared in onError, life resumes).
  if (g_ota_active) { delay(50); return; }

  // ── DIAGNOSTIC: type a track number + Enter in Serial Monitor to play it ──
  // Uses the REAL dfplayer_play()/DFP_TRACK_MAP path — same code the clock
  // itself uses for chimes/alarms/egg timer — so this is a true like-for-like
  // test, just triggered manually instead of waiting for a scheduled event.
  // e.g. type 2 = egg alert, 10 = chime set0/q0, 61=pill, 62=med, 63=bday, 64=doc
  {
    static char serial_buf[6];
    static uint8_t serial_len = 0;
    while(Serial.available()) {
      char c = Serial.read();
      if(c >= '0' && c <= '9') {
        if(serial_len < 5) serial_buf[serial_len++] = c;
      } else if((c == '\n' || c == '\r') && serial_len > 0) {
        serial_buf[serial_len] = '\0';
        int track = atoi(serial_buf);
        serial_len = 0;
        Serial.printf(">>> Manual test: dfplayer_play(%d)\n", track);
        dfplayer_volume(g_vol);
        dfplayer_play((uint16_t)track);
      }
    }
  }

  timekeeping_update();

  // ── Security alarm — check PIRs and update siren (every loop pass) ────────
  sec_alarm_check_pir();
  sec_alarm_siren_update();

  // ── Deferred beep — runs the actual blocking tone() here on Core 1, never
  // inside a web handler on Core 0. Keeps HTTP responses (Panic, Arm, Disarm,
  // Zone 1 toggle) instant regardless of how "cold" the WiFi connection is.
  if(g_beep_pending != BEEP_NONE) {
    uint8_t kind = g_beep_pending;
    g_beep_pending = BEEP_NONE;
    if(kind == BEEP_SINGLE)      beep_single();
    else if(kind == BEEP_DOUBLE) beep_double();
    else if(kind == BEEP_TRIPLE) beep_triple();
  }

  // Chime (never while alarm fires)
  if(g_chime_req) {
    g_chime_req = false;
    if(!g_alm_fire) play_chime_with_vol(chime_track_for_set(g_sound_set, g_chime_track));
  }

  // Brightness update — called every loop; internal timers handle sensor read (100ms) and fade (20ms)
  update_brightness_smooth();

  // Button scan every 10ms
  if(now_ms - g_last_btn_ms >= 10) {
    g_last_btn_ms = now_ms;
    check_inputs();
  }

  // Pill button check every 20ms
  static uint32_t last_pill_ms = 0;
  if(now_ms - last_pill_ms >= 20) {
    last_pill_ms = now_ms;
    pill_check_button();
    pill_render();
    wifi_reset_check_button();   // GPIO2 held 3s = manual reboot (WiFi reconnect after loadshedding)
    med_check_button();
    med_render();
    bday_check_button();
    bday_render_leds();
    doc_render_led();
  }

  // Render every 100ms (10fps is smooth enough for a clock)
  // OR immediately if g_force_redraw was set (e.g. web dismiss) — avoids stuck dark frame
  static uint32_t last_render_ms = 0;
  if(now_ms - last_render_ms >= 100 || g_force_redraw) {
    last_render_ms = now_ms;
    g_force_redraw = false;
    render();
  }

  // Auto-stop the wake/clock alarm after 3 minutes if nobody dismissed it
  // (power-saving when away — speaker + bar LED already did their job by then)
  if(g_alm_fire && !g_wekker_ringing && (now_ms - g_alm_fire_ms >= 180000UL)) {
    g_alm_fire = false;
    dfplayer_stop();
    Serial.println("Wake alarm auto-stopped after 3 minutes (not dismissed)");
  }

  // Re-loop DFPlayer alarm every 30s while alarm fires
  // (checks !g_wekker_ringing so Alarm's re-loop never fires while Wekkers
  // is the one actually ringing — they share g_alm_fire for status/chime
  // purposes, but each has its own repeat logic)
  static uint32_t alm_loop_ms = 0;
  if(g_alm_fire && !g_wekker_ringing && (now_ms - alm_loop_ms >= 30000)) {
    alm_loop_ms = now_ms;
    dfplayer_volume(g_alarm_vol);
    dfplayer_play(1);
  }
  if(!g_alm_fire) alm_loop_ms = now_ms;

  // ── Wekkers repeat-until-accepted ─────────────────────────────────────────
  // Repeats every WEKKER_REPEAT_MS, up to WEKKER_MAX_REPEATS times, or stops
  // immediately if BTN_BDAY (accept) was pressed (handled in
  // bday_check_button(), which clears g_wekker_ringing directly).
  if(g_wekker_ringing) {
    if(g_wekker_repeat_count >= WEKKER_MAX_REPEATS) {
      g_wekker_ringing = false;
      g_alm_fire       = false;
      Serial.println("[wekker] gave up after max repeats, not accepted");
    } else if(now_ms - g_wekker_last_play_ms >= WEKKER_REPEAT_MS) {
      g_wekker_last_play_ms = now_ms;
      g_wekker_repeat_count++;
      Serial.printf("[wekker] repeat %d/%d\n", g_wekker_repeat_count, WEKKER_MAX_REPEATS);
      dfplayer_volume(g_alarm_vol);
      dfplayer_play(2);
    }
  }
}

/******************************************************************************
 * SUBSYSTEMS
 ******************************************************************************/

/******************************************************************************
 * Pill Reminder Functions
 ******************************************************************************/

// Save pill status to NVS (survives power cuts)
void pill_save(void) {
  prefs.begin("pill", false);
  prefs.putBool ("taken",    g_pill_taken);
  prefs.putUChar("lastday",  g_pill_last_day);
  prefs.putUChar("almhh",    g_pill_alm_hh);
  prefs.putUChar("almmm",    g_pill_alm_mm);
  prefs.putBool ("almen",    g_pill_alm_en);
  prefs.putBool ("active",   g_pills_active);
  prefs.end();
}

// Load pill status from NVS
void pill_load(void) {
  prefs.begin("pill", true);
  g_pill_taken    = prefs.getBool ("taken",   false);
  g_pill_last_day = prefs.getUChar("lastday", 255);
  g_pill_alm_hh   = prefs.getUChar("almhh",  8);
  g_pill_alm_mm   = prefs.getUChar("almmm",  0);
  g_pill_alm_en   = prefs.getBool ("almen",  true);
  g_pills_active  = prefs.getBool ("active", true);
  prefs.end();
}

// Check if pill status needs resetting (new day)
void pill_check_reset(void) {
  uint8_t today = (rtc_dow() + 6) % 7;  // Monday=0 ... Sunday=6
  if (today != g_pill_last_day) {
    // New day — reset pill status
    g_pill_taken    = false;
    g_pill_last_day = today;
    g_pill_alm_fire = false;
    pill_save();
    Serial.println("Pill status reset for new day");
  }
}

// Ouma confirmed she took her pills
void pill_confirm(void) {
  g_pill_taken    = true;
  g_pill_alm_fire = false;
  g_pill_last_day = (rtc_dow() + 6) % 7;
  pill_save();
  // Play confirmation sound on DFPlayer
  dfplayer_volume(is_night_now() ? g_night_vol : g_vol);
  dfplayer_play(PILL_TRACK); voice_lock();
  Serial.println("Pills confirmed taken!");
}

// Render the Sunlephant 7-day LED strip
void pill_render(void) {
  uint8_t today   = (rtc_dow() + 6) % 7;  // Monday=0 ... Sunday=6
  uint8_t bright  = is_night_now() ? 30 : 120;

  for (uint8_t i = 0; i < PILL_NUM_LEDS; i++) {
    if (i == 7) {
      // LED 7 controlled by doc_render_led() — skip here
      continue;
    }

    if (g_pill_alm_fire) {
      // Pill alarm firing — pulse all orange
      uint8_t pulse = (uint8_t)(128 + 127 * sin(millis() / 400.0));
      pill_leds[i] = CRGB(pulse, pulse / 3, 0);
      continue;
    }

    if (i == today) {
      // Today
      if (!g_pills_active) {
        // No pills to take today — soft green pulse = geen medikasie
        uint8_t pulse = (uint8_t)(25 + 15 * sin(millis() / 1200.0));
        pill_leds[i] = CRGB(0, pulse, 0);
      } else if (g_pill_taken) {
        pill_leds[i] = CRGB(0, bright, 0);        // GREEN = taken
      } else {
        // Red pulse to draw attention
        uint8_t pulse = (uint8_t)(80 + 80 * sin(millis() / 600.0));
        pill_leds[i] = CRGB(pulse, 0, 0);          // RED pulsing = not taken
      }
    } else if (i < today) {
      // Past days this week
      uint8_t dim = bright / 2;
      pill_leds[i] = CRGB(dim, dim, dim);           // dim white = past
    } else {
      // Future days
      pill_leds[i] = CRGB::Black;                   // off = future
    }
  }
  leds_show_all();  // shows both leds[] and pill_leds[] together
}

// Check pill button (GPIO2) — debounced
void pill_check_button(void) {
#if !PILLS_ENABLED
  return;  // Daily-pills retired — button inert, kept wired for future use
#endif
  bool btn_now = (digitalRead(BTN_PILL) == LOW);
  if (btn_now && !g_pill_btn_prev) {
    // Rising edge — button just pressed
    if (!g_pill_taken) {
      pill_confirm();
    } else {
      // Already taken — flash green quickly as acknowledgement
      for (uint8_t f = 0; f < 3; f++) {
        fill_solid(pill_leds, PILL_NUM_LEDS, CRGB(0, 120, 0));
        leds_show_all(); delay(100);
        fill_solid(pill_leds, PILL_NUM_LEDS, CRGB::Black);
        leds_show_all(); delay(100);
      }
    }
    g_last_interaction_ms = millis();
  }
  g_pill_btn_prev = btn_now;
}

// Check WiFi/network reset button — repurposed GPIO2 (BTN_PILL, Blue,
// Sw10) since the daily-pills subsystem was retired (PILLS_ENABLED 0) and
// left this button permanently inert. NEW: Loadshedding fix, Henry's
// request — when the router/ONT is still coming back up after power is
// restored, WiFiManager's one-shot autoConnect() in wifi_task() can fail
// (SSID not broadcasting yet) and the clock then just runs offline until
// someone power-cycles it. Rather than needing to unplug/replug, HOLD this
// button for 3 seconds to reboot the clock in place — on the next boot,
// autoConnect() tries again and normally succeeds once the router is up.
// Held (not a single tap) on purpose so a stray knock never triggers a
// reboot. Works identically whether or not the web portal is reachable —
// it's a physical button, not a network request.

void wifi_reset_check_button(void) {
  bool btn_now = (digitalRead(BTN_PILL) == LOW);   // shared pin with BTN_PILL

  if (btn_now && !g_wifi_btn_prev) {
    // Rising edge — press started
    g_wifi_btn_down_ms = millis();
    g_wifi_btn_fired    = false;
  }

  if (btn_now && g_wifi_btn_down_ms != 0 && !g_wifi_btn_fired) {
    uint32_t held_ms = millis() - g_wifi_btn_down_ms;

    // Feedback while holding: pulse the internet-reachability LED [27] red
    // faster the longer it's held, so there's visible confirmation the
    // press has been registered well before the 3s trigger.
    bar3_leds[FREE_BASE + 7] = ((millis() / 150) % 2 == 0) ? CRGB(200, 0, 0) : CRGB::Black;

    if (held_ms >= WIFI_BTN_HOLD_MS) {
      g_wifi_btn_fired = true;
      Serial.println("BTN_PILL held 3s — manual WiFi/network reset requested, restarting...");
      // Quick solid-red flash on the whole free/status bar as a clear
      // "restarting now" cue before the reboot cuts the LEDs off anyway.
      for (uint8_t f = 0; f < 3; f++) {
        bar3_leds[FREE_BASE + 7] = CRGB(255, 0, 0);
        leds_show_all(); delay(120);
        bar3_leds[FREE_BASE + 7] = CRGB::Black;
        leds_show_all(); delay(120);
      }
      delay(200);
      ESP.restart();
    }
  }

  if (!btn_now && g_wifi_btn_prev) {
    // Released before reaching the 3s hold threshold: this was a short tap,
    // not a WiFi-reset request. Use it for a "quick peek" at the smoker's
    // elapsed time on the main display (Henry's request) — only if the
    // smoker is actually running, otherwise there's nothing worth showing.
    if (!g_wifi_btn_fired && g_smoke_running) {
      g_smoke_peek_active = true;
      g_smoke_peek_end_ms = millis() + SMOKE_PEEK_MS;
    }
    g_wifi_btn_down_ms = 0;
    g_wifi_btn_fired    = false;
  }

  g_wifi_btn_prev = btn_now;
}

// Check if pill reminder alarm should fire
void pill_check_alarm(void) {
  if (!g_pill_alm_en)    return;   // alarm disabled
  if (!g_pills_active)   return;   // no pills today
  if (g_pill_taken)      return;   // already taken today
  if (g_pill_alm_fire)   return;   // already firing

  if (g_hh == g_pill_alm_hh && g_mm == g_pill_alm_mm && g_new_minute) {  // was && g_ss == 0
    g_pill_alm_fire = true;
    dfplayer_volume(is_night_now() ? g_night_vol : g_vol);
    dfplayer_play(PILL_TRACK); voice_lock();
    Serial.println("Pill reminder alarm fired!");
  }
}


/******************************************************************************
 * Medicine Schedule Functions
 ******************************************************************************/

void med_save(void) {
  prefs.begin("med", false);
  prefs.putUChar("doses", g_med_doses);
  prefs.putUChar("lastday", g_med_last_day);
  // Kort Kursus — TWO independent slots, keys prefixed "c<slot>..."
  for (uint8_t s = 0; s < COURSE_SLOTS; s++) {
    String p = "c" + String(s);   // key prefix per slot, e.g. "c0name", "c1total"
    prefs.putString((p + "name").c_str(),  g_course[s].name);
    prefs.putUChar (("c" + String(s) + "d").c_str(), g_course[s].doses);
    prefs.putUChar ((p + "total").c_str(), g_course[s].days_total);
    prefs.putUChar ((p + "left").c_str(),  g_course[s].days_left);
    prefs.putUChar ((p + "lday").c_str(),  g_course[s].last_day);
    for (uint8_t i = 0; i < COURSE_MAX_DOSES; i++) {
      String k = p + String(i);
      prefs.putUChar(("h" + k).c_str(), g_course[s].hh[i]);
      prefs.putUChar(("m" + k).c_str(), g_course[s].mm[i]);
      prefs.putBool (("t" + k).c_str(), g_course[s].taken[i]);
    }
  }
  // Ad-hoc unscheduled extra-dose log
  prefs.putUChar ("adhcnt", g_adhoc_count);
  prefs.putUChar ("adhlday", g_adhoc_last_day);
  for (uint8_t i = 0; i < ADHOC_MAX_LOG; i++) {
    String k = String(i);
    prefs.putUChar(("ahh" + k).c_str(), g_adhoc_hh[i]);
    prefs.putUChar(("amm" + k).c_str(), g_adhoc_mm[i]);
  }
  for (uint8_t i = 0; i < MED_MAX_DOSES; i++) {
    String k = String(i);
    prefs.putUChar(("hh" + k).c_str(), g_med[i].hh);
    prefs.putUChar(("mm" + k).c_str(), g_med[i].mm);
    prefs.putBool (("tk" + k).c_str(), g_med[i].taken);
  }
  prefs.end();
}

void med_load(void) {
  prefs.begin("med", true);
  uint8_t d = prefs.getUChar("doses", 2);
  if (d <= MED_MAX_DOSES) g_med_doses = d;
  g_med_last_day      = prefs.getUChar("lastday",   255);
  // Kort Kursus — TWO independent slots
  for (uint8_t s = 0; s < COURSE_SLOTS; s++) {
    String p = "c" + String(s);
    String cname = prefs.getString((p + "name").c_str(), "");
    strncpy(g_course[s].name, cname.c_str(), sizeof(g_course[s].name) - 1);
    g_course[s].name[sizeof(g_course[s].name) - 1] = '\0';
    g_course[s].doses = prefs.getUChar(("c" + String(s) + "d").c_str(), 0);
    if (g_course[s].doses > COURSE_MAX_DOSES) g_course[s].doses = COURSE_MAX_DOSES;
    g_course[s].days_total = prefs.getUChar((p + "total").c_str(), 0);
    g_course[s].days_left  = prefs.getUChar((p + "left").c_str(),  0);
    g_course[s].last_day   = prefs.getUChar((p + "lday").c_str(),  255);
    for (uint8_t i = 0; i < COURSE_MAX_DOSES; i++) {
      String k = p + String(i);
      uint8_t chh = prefs.getUChar(("h" + k).c_str(), g_course[s].hh[i]);
      uint8_t cmm = prefs.getUChar(("m" + k).c_str(), g_course[s].mm[i]);
      if (chh < 24) g_course[s].hh[i] = chh;
      if (cmm < 60) g_course[s].mm[i] = cmm;
      g_course[s].taken[i] = prefs.getBool(("t" + k).c_str(), false);
    }
  }
  // Ad-hoc unscheduled extra-dose log
  g_adhoc_count    = prefs.getUChar("adhcnt",  0);
  if (g_adhoc_count > ADHOC_MAX_LOG) g_adhoc_count = ADHOC_MAX_LOG;
  g_adhoc_last_day = prefs.getUChar("adhlday", 255);
  for (uint8_t i = 0; i < ADHOC_MAX_LOG; i++) {
    String k = String(i);
    g_adhoc_hh[i] = prefs.getUChar(("ahh" + k).c_str(), 0);
    g_adhoc_mm[i] = prefs.getUChar(("amm" + k).c_str(), 0);
  }
  for (uint8_t i = 0; i < MED_MAX_DOSES; i++) {
    String k = String(i);
    uint8_t hh = prefs.getUChar(("hh" + k).c_str(), g_med[i].hh);
    uint8_t mm = prefs.getUChar(("mm" + k).c_str(), g_med[i].mm);
    if (hh < 24) g_med[i].hh = hh;
    if (mm < 60) g_med[i].mm = mm;
    g_med[i].taken = prefs.getBool(("tk" + k).c_str(), false);
  }
  prefs.end();
}

// Reset all doses at midnight / new day
void med_check_reset(void) {
  uint8_t today = (rtc_dow() + 6) % 7;  // Monday=0 ... Sunday=6
  if (today != g_med_last_day) {
    for (uint8_t i = 0; i < MED_MAX_DOSES; i++) g_med[i].taken = false;
    g_med_last_day      = today;
    g_med_alm_active    = 255;
    med_save();
    Serial.println("Medicine doses reset for new day");
  }
  // Ad-hoc extra-dose log resets independently (own day detector, same
  // pattern) — keeps this fully separate from the chronic reset above so a
  // future change to one can't silently affect the other.
  if (today != g_adhoc_last_day) {
    g_adhoc_count    = 0;
    g_adhoc_last_day = today;
    med_save();
  }
}

// ── Kort Kursus (Short Course) day rollover ─────────────
// Uses g_day (day-of-month), NOT day-of-week like med_check_reset() above —
// a course must count REAL elapsed calendar days (4 means 4, not "any day
// that differs from last time mod 7"). Same new-day pattern as
// evt_check()/doc_check(). Now takes a slot index — each of the two course
// slots rolls over and finishes completely independently, so Ria's two
// antibiotics on two different day-counts don't interfere with each other.
void course_check_reset(uint8_t s) {
  Course& c = g_course[s];
  if (c.doses == 0 || c.days_left == 0) return;   // this slot has no active course
  if (g_day == c.last_day) return;                 // same day, nothing to do
  c.last_day = g_day;
  c.days_left--;
  if (c.days_left == 0) {
    // Course finished — clear it automatically, no manual switch-off needed
    po_notify("Kort Kursus klaar / Course finished",
      String(c.name) + " is nou klaar. / course is now complete.\nTyd: " + po_time(), 0);
    c.doses = 0;
    c.name[0] = '\0';
    med_save();
    Serial.println("Kort Kursus slot " + String(s) + " finished and cleared");
    return;
  }
  for (uint8_t i = 0; i < c.doses; i++) c.taken[i] = false;
  if (g_course_alm_slot == s) { g_course_alm_slot = 255; g_course_alm_dose = 255; }
  med_save();
  Serial.println("Kort Kursus slot " + String(s) + " rolled to next day");
}

// Log an unscheduled extra dose RIGHT NOW . No time to
// set, no course to configure: just records that it happened, for today's
// record and for the family's peace of mind. FYI Pushover only (priority 0,
// no acknowledge needed) since this reports something already done, not a
// reminder to do something.
void adhoc_log(void) {
  if (g_adhoc_count >= ADHOC_MAX_LOG) return;   // 6/day is already a lot — quietly ignore further taps
  g_adhoc_hh[g_adhoc_count] = g_hh;
  g_adhoc_mm[g_adhoc_count] = g_mm;
  g_adhoc_count++;
  med_save();
  dfplayer_volume(is_night_now() ? g_night_vol : g_vol);
  dfplayer_play(MED_TRACK); voice_lock();
  char hhmm[6]; snprintf(hhmm, sizeof(hhmm), "%02u:%02u", g_hh, g_mm);
  po_notify("Ekstra Medisyne / Extra Medicine",
    "Ouma het 'n ekstra dosis geneem om " + String(hhmm) + ".\nTyd: " + po_time(), 0);
  Serial.print("Ad-hoc extra dose logged at "); Serial.println(hhmm);
}

// Find the next due-but-not-taken dose WITHIN a given course slot (255=none)
uint8_t course_due_dose(uint8_t s) {
  Course& c = g_course[s];
  if (c.doses == 0 || c.days_left == 0) return 255;
  for (uint8_t i = 0; i < c.doses; i++) {
    if (!c.taken[i] &&
        (g_hh > c.hh[i] || (g_hh == c.hh[i] && g_mm >= c.mm[i]))) {
      return i;
    }
  }
  return 255;
}

// ── Kort Kursus per-second check — fires voice/LED at dose time + Pushover ──
// pre-warning MED_PO_LEAD_MIN minutes ahead, same acknowledge-or-keep-nagging
// pattern as the chronic medicine Pushover. loops BOTH slots,
// completely independently — two courses can fire, pre-warn, and finish on
// their own separate schedules without ever touching each other's state.
// Notification text always names the course, so two simultaneous alerts are
// never ambiguous about which medicine they're about.
void course_check(void) {
  for (uint8_t s = 0; s < COURSE_SLOTS; s++) {
    course_check_reset(s);
    Course& c = g_course[s];
    if (c.doses == 0 || c.days_left == 0) continue;

#if MED_PUSHOVER
    if (g_new_minute) {
      for (uint8_t i = 0; i < c.doses; i++) {
        if (c.taken[i]) continue;
        int16_t warn = (int16_t)c.hh[i] * 60 + c.mm[i] - MED_PO_LEAD_MIN;
        if (warn < 0) warn += 24 * 60;
        if ((int16_t)g_hh * 60 + g_mm == warn) {
          char hhmm[6]; snprintf(hhmm, sizeof(hhmm), "%02u:%02u", c.hh[i], c.mm[i]);
          uint8_t day_num = c.days_total - c.days_left + 1;
          po_notify("Kort Kursus — " + String(c.name),
            "Oor " + String(MED_PO_LEAD_MIN) + " minute (om " + String(hhmm) + "): dosis " +
            String(i + 1) + " van " + String(c.doses) + ".\n" +
            "Dag " + String(day_num) + " van " + String(c.days_total) + ".\n" +
            "Druk Acknowledge sodra dit geneem is.\nTyd: " + po_time(),
            2, "", MED_PO_RETRY_SECS, MED_PO_EXPIRE_SECS);
        }
      }
    }
#endif

    for (uint8_t i = 0; i < c.doses; i++) {
      if (!c.taken[i] && g_hh == c.hh[i] && g_mm == c.mm[i] && g_new_minute) {
        g_course_alm_slot = s;
        g_course_alm_dose = i;
        dfplayer_volume(is_night_now() ? g_night_vol : g_vol);
        dfplayer_play(MED_TRACK); voice_lock();
        Serial.println("Kort Kursus slot " + String(s) + " dose alarm fired: " + String(i + 1));
        break;
      }
    }
  }
}

// Find the NEXT dose that is due but not yet taken
// Returns 255 if none due right now
uint8_t med_due_dose(void) {
  if (g_med_doses == 0) return 255;           // no script
  if (g_hh < MED_START_HH) return 255;        // before 07:00
  if (g_hh > MED_END_HH)   return 255;        // after 19:00
  for (uint8_t i = 0; i < g_med_doses; i++) {
    if (!g_med[i].taken) {
      // Is this dose time reached?
      if (g_hh > g_med[i].hh ||
         (g_hh == g_med[i].hh && g_mm >= g_med[i].mm)) {
        return i;
      }
    }
  }
  return 255;
}

// Confirm the current due dose taken
void med_confirm(void) {
  uint8_t due = med_due_dose();
  if (due == 255) {
    // No scheduled chronic dose due right now — check both Kort Kursus slots.
    // Two independent courses can both have doses due; slot 0 is
    // checked first. If both are due at once, this press resolves slot 0 —
    // pressing again immediately after resolves slot 1, same as how the
    // chronic medicine loop already resolves one dose per press.
    for (uint8_t s = 0; s < COURSE_SLOTS; s++) {
      uint8_t cdue = course_due_dose(s);
      if (cdue != 255) {
        g_course[s].taken[cdue] = true;
        if (g_course_alm_slot == s) { g_course_alm_slot = 255; g_course_alm_dose = 255; }
        med_save();
        dfplayer_volume(is_night_now() ? g_night_vol : g_vol);
        dfplayer_play(MED_TRACK); voice_lock();
        Serial.println("Kort Kursus slot " + String(s) + " dose confirmed taken: " + String(cdue + 1));
        return;
      }
    }
    // Nothing due at all — flash white briefly
    for (uint8_t f = 0; f < 2; f++) {
      fill_solid(med_leds, MED_NUM_LEDS, CRGB(60,60,60));
      leds_show_all(); delay(150);
      fill_solid(med_leds, MED_NUM_LEDS, CRGB::Black);
      leds_show_all(); delay(150);
    }
    return;
  }
  g_med[due].taken    = true;
  g_med_alm_active    = 255;
  med_save();
  dfplayer_volume(is_night_now() ? g_night_vol : g_vol);
  dfplayer_play(MED_TRACK); voice_lock();
  Serial.print("Medicine dose confirmed: "); Serial.println(due + 1);
}

// Check if any dose alarm should fire
void med_check_alarms(void) {
  if (g_med_doses == 0) return;
  if (g_hh < MED_START_HH || g_hh > MED_END_HH) return;
#if MED_PUSHOVER
  // Ouma's request: the phone warning now goes out
  // MED_PO_LEAD_MIN minutes BEFORE each dose (06:55 for a 07:00 dose), so
  // the pille are in hand by the time the clock announces. Emergency
  // priority as before — re-alerts every 5 min for up to 30 min until
  // Acknowledged, which comfortably spans the pre-warning AND the dose time.
  // Skipped if the dose was already confirmed taken by warn time. Minute
  // math wraps midnight correctly, though the 07:00-19:00 med window makes
  // that academic.
  if (g_new_minute) {
    for (uint8_t i = 0; i < g_med_doses; i++) {
      if (g_med[i].taken) continue;
      int16_t warn = (int16_t)g_med[i].hh * 60 + g_med[i].mm - MED_PO_LEAD_MIN;
      if (warn < 0) warn += 24 * 60;
      if ((int16_t)g_hh * 60 + g_mm == warn) {
        char hhmm[6]; snprintf(hhmm, sizeof(hhmm), "%02u:%02u", g_med[i].hh, g_med[i].mm);
        po_notify("Medisyne — Ouma",
          "Oor " + String(MED_PO_LEAD_MIN) + " minute (om " + String(hhmm) + "): dosis " +
          String(i + 1) + " van " + String(g_med_doses) + ".\n"
          "Druk Acknowledge sodra dit geneem is.\nTyd: " + po_time(),
          2, "", MED_PO_RETRY_SECS, MED_PO_EXPIRE_SECS);
      }
    }
  }
#endif

  for (uint8_t i = 0; i < g_med_doses; i++) {
    if (!g_med[i].taken &&
        g_hh == g_med[i].hh &&
        g_mm == g_med[i].mm &&
        g_new_minute) {   // was g_ss == 0 — a skipped second no longer skips the dose alarm
      g_med_alm_active = i;
      dfplayer_volume(is_night_now() ? g_night_vol : g_vol);
      dfplayer_play(MED_TRACK); voice_lock();
      Serial.print("Medicine alarm fired: dose "); Serial.println(i + 1);
      // Pushover MOVED to the 5-min pre-warning block above (Ouma's
      // request) — at the dose minute the clock does voice + LED only.
      break;
    }
  }
}

// Render the medicine LED strip
void med_render(void) {
  if (g_med_doses == 0) {
    // No active script — show one dim green LED to indicate "geen medikasie"
    // This way Ouma sees green = all good, not a scary blank strip
    fill_solid(med_leds, MED_NUM_LEDS, CRGB::Black);
    // Light LED 0 with a soft green pulse = "geen medikasie vir vandag"
    uint8_t pulse = (uint8_t)(30 + 20 * sin(millis() / 1200.0));
    med_leds[0] = CRGB(0, pulse, 0);
    leds_show_all();
    return;
  }

  uint8_t bright = is_night_now() ? 20 : 110;

  for (uint8_t i = 0; i < MED_NUM_LEDS; i++) {
    if (i >= g_med_doses) {
      // FIX 5 (2026-07-03): LEDs 4/5/6 belong to bday_render_leds(). This
      // blanking ran every 20ms and pushed the dark frame to the strip via
      // leds_show_all() below, BEFORE bday_render_leds() repainted them —
      // so the physical birthday LEDs alternated dark/lit frame after frame,
      // showing at roughly half brightness with visible flicker.
      if (i == BDAY_LED_3 || i == BDAY_LED_2 || i == BDAY_LED_1) continue;
      // Unused dose slot — off
      med_leds[i] = CRGB::Black;
      continue;
    }

    // Is this dose alarming right now?
    if (i == g_med_alm_active) {
      // Pulse orange — medicine time!
      uint8_t pulse = (uint8_t)(80 + 80 * sin(millis() / 350.0));
      med_leds[i] = CRGB(pulse, pulse / 4, 0);
      continue;
    }

    if (g_med[i].taken) {
      // GREEN = taken
      med_leds[i] = CRGB(0, bright, 0);
    } else {
      // Has this dose time passed without being taken?
      bool overdue = (g_hh > g_med[i].hh ||
                     (g_hh == g_med[i].hh && g_mm > g_med[i].mm));
      if (overdue) {
        // Pulse RED = overdue!
        uint8_t pulse = (uint8_t)(60 + 60 * sin(millis() / 500.0));
        med_leds[i] = CRGB(pulse, 0, 0);
      } else {
        // Future dose — dim blue
        uint8_t dim = bright / 2;
        med_leds[i] = CRGB(0, 0, dim);
      }
    }
  }

  // ── LED 7 = Kort Kursus (Short Course) indicator ────────
  // Only ONE physical LED for TWO possible independent courses (no spare LED
  // on this strip without stealing from the birthday indicators) — so this
  // shows the AGGREGATE across both slots: red pulse if EITHER slot has an
  // overdue dose (needs attention now), green only once ALL doses in ALL
  // active slots are taken, dim blue if on track but more doses are still
  // ahead today. Which course specifically needs attention is always spelled
  // out on the /medicine page and in the Pushover notification text.
  {
    bool any_active = false, all_taken = true, any_overdue = false;
    for (uint8_t s = 0; s < COURSE_SLOTS; s++) {
      Course& c = g_course[s];
      if (c.doses == 0 || c.days_left == 0) continue;
      any_active = true;
      for (uint8_t i = 0; i < c.doses; i++) {
        if (!c.taken[i]) {
          all_taken = false;
          if (g_hh > c.hh[i] || (g_hh == c.hh[i] && g_mm >= c.mm[i])) any_overdue = true;
        }
      }
    }
    if (!any_active) {
      med_leds[7] = CRGB::Black;
    } else if (all_taken) {
      med_leds[7] = CRGB(0, bright, 0);
    } else if (any_overdue) {
      bool flash_on = ((millis() / 500) % 2 == 0);
      med_leds[7] = flash_on ? CRGB(bright, 0, 0) : CRGB::Black;
    } else {
      med_leds[7] = CRGB(0, 0, bright / 2);   // taken so far, next dose still ahead today
    }
  }

  leds_show_all();
}

// Check medicine button (GPIO18) — debounced
void med_check_button(void) {
  bool btn_now = (digitalRead(BTN_MED) == LOW);
  if (btn_now && !g_med_btn_prev) {
    med_confirm();
    g_last_interaction_ms = millis();
  }
  g_med_btn_prev = btn_now;
}

// ── Wekkers NVS persistence ───────────────────────────────────────────────
// Every reboot (OTA, loadshedding, brownout) survives correctly — all three
// Wekkers reload their saved time/enabled/day-mask state, same save/load
// pattern as every other subsystem. g_wekker_fired_today[] is deliberately
// NOT saved — after a reboot the safe assumption is "not fired yet today",
// so a Wekker later the same day still rings rather than being silently
// skipped.
void wekker_save(void) {
  prefs.begin("wekker", false);
  for (uint8_t i = 0; i < NUM_ALARM_SLOTS; i++) {
    String k = String(i);
    prefs.putBool (("en" + k).c_str(), g_wekker_enabled[i]);
    prefs.putUChar(("hh" + k).c_str(), g_wekker_hh[i]);
    prefs.putUChar(("mm" + k).c_str(), g_wekker_mm[i]);
    prefs.putUChar(("dy" + k).c_str(), g_wekker_days[i]);
  }
  prefs.end();
}

void wekker_load(void) {
  prefs.begin("wekker", true);
  for (uint8_t i = 0; i < NUM_ALARM_SLOTS; i++) {
    String k = String(i);
    g_wekker_enabled[i] = prefs.getBool(("en" + k).c_str(), g_wekker_enabled[i]);
    uint8_t hh = prefs.getUChar(("hh" + k).c_str(), g_wekker_hh[i]);
    uint8_t mm = prefs.getUChar(("mm" + k).c_str(), g_wekker_mm[i]);
    uint8_t dy = prefs.getUChar(("dy" + k).c_str(), g_wekker_days[i]);
    if (hh < 24) g_wekker_hh[i] = hh;
    if (mm < 60) g_wekker_mm[i] = mm;
    g_wekker_days[i] = dy ? dy : 0x7F;   // never allow an all-zero day mask
  }
  prefs.end();
}

void settings_save(void) {
  prefs.begin("clock", false);
  prefs.putUChar("alm0hh",  g_alarm1.hh);
  prefs.putUChar("alm0mm",  g_alarm1.mm);
  prefs.putBool ("alm0en",  g_alarm1.enabled);
  prefs.putUChar("vol",     g_vol);
  prefs.putUChar("nvol",    g_night_vol);
  prefs.putUChar("avol",    g_alarm_vol);
  prefs.putUChar("brday",   g_bright_day);
  prefs.putUChar("brnight", g_bright_night);
  prefs.putBool ("chime",   g_chime_en);
  prefs.putUChar("eggmm",   g_egg_set_mm);
  prefs.putUChar("eggss",   g_egg_set_ss);
  prefs.putUChar("sndset",  g_sound_set);
  prefs.putBool ("hr12",    g_use_12hr);
  prefs.putBool ("tempF",   g_temp_degF);
  prefs.putShort("tz",      (int16_t)g_tz_offset_hours);
  prefs.putBool ("smkrun",  g_smoke_running);
  prefs.putBool ("smkpaus", g_smoke_paused);
  prefs.putULong("smkstart",g_smoke_start_unix);
  prefs.putULong("smkpu",   g_smoke_pause_unix);
  prefs.putULong("smkacc",  g_smoke_paused_accum);
  prefs.putString("smknote",g_smoke_note);
  prefs.putBool ("brEn",    g_braai_enabled);
  prefs.putBool ("brRun",   g_braai_running);
  prefs.putBool ("brPaus",  g_braai_paused);
  prefs.putULong("brStart", g_braai_start_unix);
  prefs.putULong("brPu",    g_braai_pause_unix);
  prefs.putULong("brAcc",   g_braai_paused_accum);
  prefs.putUShort("brTotal",g_braai_total_sec);
  prefs.putUShort("brTurn", g_braai_turn_sec);
  prefs.end();
}

void settings_load(void) {
  prefs.begin("clock", true);
  uint8_t v;
  v = prefs.getUChar("alm0hh", 6);   if(v < 24) g_alarm1.hh = v;
  v = prefs.getUChar("alm0mm", 30);  if(v < 60) g_alarm1.mm = v;
  g_alarm1.enabled = prefs.getBool("alm0en", true);
  v = prefs.getUChar("vol",    20);  if(v <= 30) g_vol       = v;
  v = prefs.getUChar("nvol",   8);   if(v <= 30) g_night_vol = v;
  v = prefs.getUChar("avol",   25);  if(v <= 30) g_alarm_vol = v;
  v = prefs.getUChar("brday",   180); g_bright_day   = v;
  v = prefs.getUChar("brnight", 40);  g_bright_night = v;
  g_chime_en          = prefs.getBool("chime", true);
  v = prefs.getUChar("eggmm",  5);   if(v < 100) g_egg_set_mm = v;
  v = prefs.getUChar("eggss",  0);   if(v < 60)  g_egg_set_ss = v;
  v = prefs.getUChar("sndset", 0);   if(v <= 4)  g_sound_set  = v;
  g_use_12hr          = prefs.getBool("hr12",  false);
  g_temp_degF         = prefs.getBool("tempF", false);
  g_tz_offset_hours   = prefs.getShort("tz", 2);
  g_smoke_running      = prefs.getBool("smkrun",  false);
  g_smoke_paused       = prefs.getBool("smkpaus", false);
  g_smoke_start_unix   = prefs.getULong("smkstart", 0);
  g_smoke_pause_unix   = prefs.getULong("smkpu",    0);
  g_smoke_paused_accum = prefs.getULong("smkacc",   0);
  g_smoke_note         = prefs.getString("smknote", "");
  g_braai_enabled      = prefs.getBool("brEn",    false);
  g_braai_running      = prefs.getBool("brRun",   false);
  g_braai_paused       = prefs.getBool("brPaus",  false);
  g_braai_start_unix   = prefs.getULong("brStart", 0);
  g_braai_pause_unix   = prefs.getULong("brPu",    0);
  g_braai_paused_accum = prefs.getULong("brAcc",   0);
  g_braai_total_sec    = prefs.getUShort("brTotal", 600);
  g_braai_turn_sec     = prefs.getUShort("brTurn",  60);
  prefs.end();
  g_egg_mm = g_egg_set_mm;
  g_egg_ss = g_egg_set_ss;
}

void factory_reset_defaults(void) {
  prefs.begin("clock", false);
  prefs.clear();
  prefs.end();
  // WiFiManager config also cleared:
  WiFiManager wm;
  wm.resetSettings();
}

// ── Smoker Uptimer — elapsed seconds, RTC-based so it survives a reboot ──────
uint32_t smoke_elapsed_sec(void) {
  if(!g_smoke_running) return 0;
  uint32_t now_unix = rtc.now().unixtime();
  uint32_t paused = g_smoke_paused_accum;
  if(g_smoke_paused && now_unix > g_smoke_pause_unix) {
    paused += (now_unix - g_smoke_pause_unix);
  }
  if(now_unix <= g_smoke_start_unix) return 0;
  uint32_t raw = now_unix - g_smoke_start_unix;
  return (raw > paused) ? (raw - paused) : 0;
}

// ── Braai Timer — elapsed/remaining seconds, same RTC-based survive-reboot
// pattern as the Smoker Uptimer above, but this one counts DOWN to zero.
uint32_t braai_elapsed_sec(void) {
  if(!g_braai_running) return 0;
  uint32_t now_unix = rtc.now().unixtime();
  uint32_t paused = g_braai_paused_accum;
  if(g_braai_paused && now_unix > g_braai_pause_unix) {
    paused += (now_unix - g_braai_pause_unix);
  }
  if(now_unix <= g_braai_start_unix) return 0;
  uint32_t raw = now_unix - g_braai_start_unix;
  return (raw > paused) ? (raw - paused) : 0;
}

uint32_t braai_remaining_sec(void) {
  uint32_t elapsed = braai_elapsed_sec();
  return (elapsed < g_braai_total_sec) ? (g_braai_total_sec - elapsed) : 0;
}

/******************************************************************************
 * DFPlayer Mini — direct UART2 driver (no library needed)
 ******************************************************************************/
//  Frame: 7E FF 06 CMD 00 ParamH ParamL CsumH CsumL EF
//
//  REVERTED 2026-06-21: the "clone-safe" zero-checksum variant tried briefly
//  here was rolled back. A 0x00,0x00 checksum is not a valid frame on these
//  clone boards either — it doesn't bypass anything, it just gets rejected.
//  The real calculated checksum below is the version that was actually
//  confirmed working on 2026-06-17/18 once combined with the TF-card-select
//  command and the gapless track-mapping table further down this file.

void dfp_send(uint8_t cmd, uint8_t paramH, uint8_t paramL) {
  uint8_t buf[10];
  buf[0] = 0x7E; buf[1] = 0xFF; buf[2] = 0x06;
  buf[3] = cmd;  buf[4] = 0x00;
  buf[5] = paramH; buf[6] = paramL;
  int16_t chk = 0 - (int16_t)(buf[1]+buf[2]+buf[3]+buf[4]+buf[5]+buf[6]);
  buf[7] = (uint8_t)(chk >> 8);
  buf[8] = (uint8_t)(chk & 0xFF);
  buf[9] = 0xEF;
  Serial2.write(buf, 10);
}

/******************************************************************************
 * DFPlayer logical → physical track mapping
 ******************************************************************************/
//  This particular module (clone chip) plays files by their PHYSICAL WRITE
//  ORDER on the SD card, not by the digits in the filename. Gapped numbering
//  (e.g. 0001, 0002, 0010, 0011...) caused "track not found" errors past the
//  total file count. Fix applied 2026-06-18: all files renumbered into one
//  gapless sequential block (0001.mp3 .. 0026.mp3) on the card.
//
//  The rest of the firmware still uses the original MEANINGFUL track numbers
//  (alarm=1, MED_TRACK=62, chime_track_for_set() formula, etc.) — this table
//  converts those logical numbers into the actual physical position on the
//  card. dfplayer_play() runs every track number through this before sending.
//
//  IMPORTANT: if you add, remove, or re-copy files on the SD card later, the
//  physical positions can shift again — re-verify with the standalone test
//  sketch and update this table to match.
//  Updated 2026-06-18: missing chime (logical 22) recorded and added —
//  card is now a complete, gapless 27-file sequence (0001.mp3-0027.mp3).
struct DfpTrackMap { uint16_t logical; uint8_t physical; };
static const DfpTrackMap DFP_TRACK_MAP[] = {
  {  1,  1},   // alarm fire sound
  {  2,  2},   // egg timer alert
  { 10,  3},   // chime set 0, quarter 0
  { 11,  4},   // chime set 0, quarter 1
  { 12,  5},   // chime set 0, quarter 2
  { 13,  6},   // chime set 0, quarter 3
  { 14,  7},   // volume up/down confirmation beep
  { 20,  8},   // chime set 1, quarter 0
  { 21,  9},   // chime set 1, quarter 1
  { 22, 10},   // chime set 1, quarter 2  ← previously missing, now recorded
  { 23, 11},   // chime set 1, quarter 3
  { 30, 12},   // chime set 2, quarter 0
  { 31, 13},   // chime set 2, quarter 1
  { 32, 14},   // chime set 2, quarter 2
  { 33, 15},   // chime set 2, quarter 3
  { 40, 16},   // chime set 3, quarter 0
  { 41, 17},   // chime set 3, quarter 1
  { 42, 18},   // chime set 3, quarter 2
  { 43, 19},   // chime set 3, quarter 3
  { 50, 20},   // chime set 4, quarter 0
  { 51, 21},   // chime set 4, quarter 1
  { 52, 22},   // chime set 4, quarter 2
  { 53, 23},   // chime set 4, quarter 3
  { 61, 24},   // PILL_TRACK  — "Pille geneem!"
  { 62, 25},   // MED_TRACK   — "Medisyne tyd Ouma!"
  { 63, 26},   // BDAY_TRACK  — "Veels geluk met jou verjaarsdag!"
  { 64, 27},   // DOC_TRACK   — "Doktersafspraak vandag!"
};
#define DFP_TRACK_MAP_SIZE (sizeof(DFP_TRACK_MAP)/sizeof(DFP_TRACK_MAP[0]))

uint8_t dfp_physical_track(uint16_t logical) {
  for(uint8_t i = 0; i < DFP_TRACK_MAP_SIZE; i++) {
    if(DFP_TRACK_MAP[i].logical == logical) return DFP_TRACK_MAP[i].physical;
  }
  Serial.printf("DFPlayer: no SD card mapping for logical track %u — recording missing?\n", logical);
  return 0;   // module will just reply with an error; harmless
}

void dfplayer_volume(uint8_t vol) {
  dfp_send(0x06, 0, vol);
  delay(30);   // settling delay between commands — the one change worth testing
               // in isolation on a clone chip; kept, but the real checksum and
               // the bounded flush loop below are restored alongside it.
}

void dfplayer_play(uint16_t t) {
  uint8_t phys = dfp_physical_track(t);
  if(phys == 0) {
    Serial.printf("DFPlayer PLAY skipped: logical=%u has no SD mapping\n", t);
    return;
  }
  Serial.printf("DFPlayer PLAY: logical=%u physical=%u\n", t, phys);

  // SAFETY: hard time limit on every flush loop below. A noisy/floating RX
  // line (e.g. a loose wire) could otherwise make available() stay true
  // forever and freeze the entire firmware — this guarantees it can't.
  uint32_t flush_start = millis();
  while(Serial2.available() && (millis() - flush_start < 50)) Serial2.read();
  dfp_send(0x03, 0, phys);

  // ── TEMPORARY DIAGNOSTIC (2026-06-23, v2) — removing the reply-read
  // entirely caused total silence, so the read itself is needed. This
  // version listens IMMEDIATELY (no fixed delay(100) gap beforehand) and
  // for a shorter window, in case the original fixed 100ms delay — sitting
  // idle, neither reading nor sending — was itself the problem, rather
  // than the read/log loop.
  uint32_t reply_start = millis();
  bool got_reply = false;
  while(millis() - reply_start < 60) {
    if(Serial2.available()) {
      got_reply = true;
      Serial.print(" reply bytes: ");
      uint32_t inner_start = millis();
      while(Serial2.available() && (millis() - inner_start < 50)) {
        Serial.printf("%02X ", Serial2.read());
      }
      Serial.println();
      break;
    }
  }
  if(!got_reply) {
    Serial.println(" (no immediate reply — module may still be processing, that's normal)");
  }
}

void dfplayer_stop(void) { dfp_send(0x16, 0, 0); }

/******************************************************************************
 * Bar3 — the single combined 30-LED chain (pills + meds + free/status)
 ******************************************************************************/
//  Mirrors pill_leds[]/med_leds[] into their slice of the physical chain, then
//  draws the free/status bar (indices 20-29) purely from current state flags.
//  This is now the ONLY place that decides status-indicator colours — the old
//  scattered leds[0]/leds[1] writes and full-matrix fill_solid() alarm flashes
//  throughout the security code have been removed; everything routes through
//  here instead. Saves power when away (no more full 256-LED flashing for
//  minutes/hours) and keeps the main clock display showing the time normally
//  even while an alarm or siren is sounding.
void compose_bar3(void) {
  for(uint8_t i = 0; i < PILL_NUM_LEDS; i++) bar3_leds[PILL_BASE + i] = pill_leds[i];
  for(uint8_t i = 0; i < MED_NUM_LEDS;  i++) bar3_leds[MED_BASE  + i] = med_leds[i];

  // ── Free bar [20][21] = security/alarm status, priority order ─────────────
  uint32_t ms = millis();
  CRGB s;
  if(g_panic_active) {
    s = ((ms / 100) % 2 == 0) ? CRGB(255, 0, 0) : CRGB(255, 255, 255);       // panic — fast
  } else if(g_sec_triggered) {
    s = ((ms / 150) % 2 == 0) ? CRGB(255, 0, 0) : CRGB(255, 255, 255);       // siren wailing
  } else if(g_sec_entry_delay) {
    s = ((ms / 300) % 2 == 0) ? CRGB(220, 0, 0) : CRGB::Black;               // entry countdown
  } else if(g_sec_exit_delay) {
    s = ((ms / 500) % 2 == 0) ? CRGB(255, 100, 0) : CRGB::Black;             // exit countdown
  } else if(g_disarm_lockout_ms > 0) {
    s = CRGB(0, 128, 0);                                                     // just disarmed (20% dimmer than before)
  } else if(g_alm_fire && !g_wekker_ringing) {
    s = ((ms / 300) % 2 == 0) ? CRGB(200, 0, 0) : CRGB::Black;               // clock alarm ringing
    // (g_alm_fire is also true while a Wekker rings — they share the flag
    // for the underlying ring/repeat mechanism — but Wekkers get their own
    // dedicated indicator below [29], so they're deliberately excluded here.
    // This keeps the top-2 slot solely for genuine security/wake-alarm
    // states: no more wondering whether a flash means an intruder or just
    // Ouma's 06:30 Wekker going off.)
  } else {
    s = g_z2_armed ? CRGB(255, 100, 0) : CRGB(0, 128, 0);                     // armed = solid orange, disarmed = green (20% dimmer than before)
  }
  bar3_leds[FREE_BASE + 0] = s;
  bar3_leds[FREE_BASE + 1] = s;

  // [22] = 12hr/24hr indicator   [23] = °C/°F indicator
  bar3_leds[FREE_BASE + 2] = g_use_12hr  ? CRGB(40, 40, 40) : CRGB::Black;
  bar3_leds[FREE_BASE + 3] = g_temp_degF ? CRGB(60, 30, 0)  : CRGB(0, 0, 40);

  // [24] = AM/PM (only meaningful in 12hr mode) — blue=AM, red=PM
  if(g_use_12hr) {
    bar3_leds[FREE_BASE + 4] = (g_hh >= 12) ? CRGB(180, 0, 0) : CRGB(0, 0, 150);
  } else {
    bar3_leds[FREE_BASE + 4] = CRGB::Black;
  }

  // [25] = DFPlayer boot health — green=responded, dim red=no response.
  // Lets you check at a glance whether DFPlayer is alive without any PC/serial.
  bar3_leds[FREE_BASE + 5] = g_dfp_alive ? CRGB(0, 60, 0) : CRGB(60, 0, 0);

  // [26] = Zone 1 (courtyard) armed/disarmed.
  // Independent of the main [20][21] status LED, which shows overall
  // alarm/siren/exit-delay state. This one ONLY answers "are the courtyard
  // PIRs actually watching right now" — useful because g_z1_active can in
  // principle be suspended on its own (see the courtyard-pause web route)
  // even while the rest of the system shows a different state.
  // UPDATED: watching/armed now shows the SAME green as the top-2 disarmed
  // indicator (was orange), and paused/disarmed now flashes red (was solid
  // green) so a paused courtyard reads as a warning, not an all-clear.
  if (g_z1_active) {
    bar3_leds[FREE_BASE + 6] = CRGB(0, 128, 0);                                 // watching/armed = green (matches top-2 disarmed shade)
  } else {
    bar3_leds[FREE_BASE + 6] = ((ms / 300) % 2 == 0) ? CRGB(200, 0, 0) : CRGB::Black;  // paused/disarmed = flashing red
  }

  // [27] = Internet reachability Henry's request.
  // WiFi-connected is NOT the same as internet-up (loadshedding: router on
  // UPS, fibre/ONT dead). g_inet_ok is refreshed every 30s by a bounded TCP
  // probe in wifi_task(). Blue = internet reachable, dim red = WiFi up but no
  // internet (or WiFi down). Distinct blue so it's not confused with the
  // green/orange security LEDs beside it. Complements the "NTP Yes/No" text on
  // the main status page but updates every 30s instead of every 6 hours.
  bar3_leds[FREE_BASE + 7] = g_inet_ok ? CRGB(0, 40, 120) : CRGB(50, 0, 0);

  // [28] = Kalender event countdown . Magenta so it's
  // distinct from the doctor LED's cyan. Same countdown language: soft pulse
  // at 3 days, brighter at 2, fast at 1, flashing on the day. Driven by
  // g_evt_today / g_evt_days_min, refreshed every second in evt_check().
  if (g_evt_days_min > 3) {
    bar3_leds[FREE_BASE + 8] = CRGB::Black;
  } else if (g_evt_today && !g_evt_acked) {
    bar3_leds[FREE_BASE + 8] = ((ms / 300) % 2 == 0) ? CRGB(200, 0, 180) : CRGB::Black;
  } else if (g_evt_today) {
    // Acknowledged — steady soft magenta instead of flashing
    bar3_leds[FREE_BASE + 8] = CRGB(60, 0, 54);
  } else {
    uint8_t p28 = (uint8_t)(30 + 30 * sin(ms / (g_evt_days_min == 1 ? 350.0 : (g_evt_days_min == 2 ? 700.0 : 1200.0))));
    bar3_leds[FREE_BASE + 8] = CRGB(p28, 0, (uint8_t)(p28 * 0.9));
  }

  // [29] = Wekkers ringing indicator (Henry's request) — deliberately amber,
  // nowhere near the red/white family used by Emergency/PIR/wake-alarm above,
  // so a glance at the status bar can never confuse "someone's Wekker is
  // going off" with "there is a security concern."
  bar3_leds[FREE_BASE + 9] = g_wekker_ringing
    ? (((ms / 300) % 2 == 0) ? CRGB(255, 170, 0) : CRGB::Black)
    : CRGB::Black;
}

void leds_show_all(void) {
  compose_bar3();

  // ── Independent per-strip brightness — NEW 2026-06-27 ─────────────────────
  // FastLED.setBrightness() is GLOBAL — it scales every strip registered with
  // FastLED.addLeds() identically. That's why fixing BAR3 (pills/meds/status)
  // to the day/night schedule also froze the MAIN matrix display, which is
  // meant to keep following the BH1750 ambient sensor as before.
  //
  // Fix: use FastLED's own per-controller show — CFastLED::operator[] returns
  // each registered controller in registration order, and .showLeds(bright)
  // pushes JUST that controller's buffer at JUST that brightness, leaving
  // leds[]/bar3_leds[] colour data completely untouched (no compounding, no
  // copies, no risk of double-scaling on the next frame).
  //
  //   controller 0 = leds[]      (main matrix)      → g_bright
  //                  (BH1750 lux-driven, see update_brightness_smooth())
  //   controller 1 = bar3_leds[] (pills/meds/status) → g_bar3_bright
  //                  (fixed day/night schedule, see update_bar3_brightness())
  //
  // Registration order is set once in setup() — see the two
  // FastLED.addLeds<...>() calls — and must stay leds[] first, bar3_leds[]
  // second for these indices to stay correct.
  FastLED[0].showLeds(g_bright);
  FastLED[1].showLeds(g_bar3_bright);
}

bool is_night_now(void) {
  if(NIGHT_START > NIGHT_END)
    return (g_hh >= NIGHT_START || g_hh < NIGHT_END);
  return (g_hh >= NIGHT_START && g_hh < NIGHT_END);
}

uint16_t chime_track_for_set(uint8_t ss, uint8_t q) {
  return (uint16_t)(ss * 10 + 10 + q);
}

/******************************************************************************
 * Birthday Calendar Functions
 ******************************************************************************/

// Returns days until next occurrence of day/month from today (0 = today, 1 = tomorrow, etc.)
int16_t days_until_bday(uint8_t bday_day, uint8_t bday_month) {
  // 29 Feb birthdays could NEVER match — dim[2] is 28,
  // so the walk below never lands on 29/2 and returned 999 forever (birthday
  // invisible, no announcement, ever). Leap-year babies now celebrate on
  // 1 March, the common convention. (2028 is the next actual 29 Feb; a fully
  // leap-aware walk needs year tracking — not worth it for a countdown LED.)
  if (bday_day == 29 && bday_month == 2) { bday_day = 1; bday_month = 3; }
  uint8_t cur_mm = g_month, cur_dd = g_day;
  static const uint8_t dim[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
  for (int16_t offset = 0; offset <= 365; offset++) {
    if (cur_dd == bday_day && cur_mm == bday_month) return offset;
    cur_dd++;
    if (cur_dd > dim[cur_mm]) { cur_dd = 1; cur_mm++; if (cur_mm > 12) cur_mm = 1; }
  }
  return 999;
}

void bday_save(void) {
  Preferences p;
  p.begin("bday", false);
  p.putUChar("count", g_bday_count);
  for (uint8_t i = 0; i < g_bday_count; i++) {
    String k = String(i);
    p.putUChar(("d"  + k).c_str(), g_bdays[i].day);
    p.putUChar(("m"  + k).c_str(), g_bdays[i].month);
    p.putString(("n" + k).c_str(), g_bdays[i].name);
  }
  p.putUChar("lastday", g_bday_last_day);
  p.putBool ("acked",   g_bday_acked);
  p.putBool ("played",  g_bday_played);
  p.end();
}

void bday_load(void) {
  Preferences p;
  p.begin("bday", true);
  g_bday_count    = p.getUChar("count", 0);
  if (g_bday_count > BDAY_MAX) g_bday_count = BDAY_MAX;
  for (uint8_t i = 0; i < g_bday_count; i++) {
    String k = String(i);
    g_bdays[i].day   = p.getUChar(("d" + k).c_str(), 1);
    g_bdays[i].month = p.getUChar(("m" + k).c_str(), 1);
    String nm = p.getString(("n" + k).c_str(), "");
    strncpy(g_bdays[i].name, nm.c_str(), 13);
    g_bdays[i].name[13] = '\0';
  }
  g_bday_last_day = p.getUChar("lastday", 255);
  g_bday_acked    = p.getBool ("acked",   false);
  g_bday_played   = p.getBool ("played",  false);
  p.end();
}

// Call once per second — checks for birthday, resets ack at midnight
void bday_check(void) {
  // Reset ack + played flag on new day
  if (g_day != g_bday_last_day) {
    g_bday_last_day = g_day;
    g_bday_acked    = false;
    g_bday_played   = false;
    g_bday_today    = false;
    g_bday_today_idx = 255;
    bday_save();
  }

  // Check if any birthday is today
  g_bday_today    = false;
  g_bday_today_idx = 255;
  for (uint8_t i = 0; i < g_bday_count; i++) {
    if (g_bdays[i].day == g_day && g_bdays[i].month == g_month) {
      g_bday_today     = true;
      g_bday_today_idx = i;
      break;
    }
  }

  // Play MP3 at 08:40 on the birthday day (was 00:00 — nobody's awake then!)
  // Uses alarm volume (g_alarm_vol) so it's coupled to the Alarm volume
  // setting, same as other "must hear" alerts (egg timer, wekkers, etc).
  if (g_bday_today && !g_bday_played && g_hh == 8 && g_mm == 40) {
    g_bday_played = true;
    dfplayer_volume(g_alarm_vol);
    dfplayer_play(BDAY_TRACK); voice_lock();
    bday_save();
  }
}

// Render birthday LEDs on med strip LEDs 4, 5, 6
// LED 4 = 3 days away (soft purple)
// LED 5 = 2 days away (medium purple)
// LED 6 = 1 day away or TODAY (fast flash or rainbow)
void bday_render_leds(void) {
  uint8_t bright = is_night_now() ? 15 : 80;

  // Default: all three off
  med_leds[BDAY_LED_3] = CRGB::Black;
  med_leds[BDAY_LED_2] = CRGB::Black;
  med_leds[BDAY_LED_1] = CRGB::Black;

  // Find closest upcoming birthday
  int16_t min_days = 999;
  for (uint8_t i = 0; i < g_bday_count; i++) {
    int16_t d = days_until_bday(g_bdays[i].day, g_bdays[i].month);
    if (d < min_days) min_days = d;
  }

  if (min_days > 3) return;  // nothing within 3 days

  uint8_t pulse = (uint8_t)(40 + 40 * sin(millis() / 600.0));
  bool flash_fast = ((millis() / 250) % 2 == 0);

  if (min_days == 3) {
    // 3 days away — soft purple on LED 4 only
    med_leds[BDAY_LED_3] = CRGB(pulse / 2, 0, pulse);
  }
  else if (min_days == 2) {
    // 2 days away — purple on LEDs 4 and 5
    med_leds[BDAY_LED_3] = CRGB(bright / 3, 0, bright / 2);
    med_leds[BDAY_LED_2] = CRGB(pulse / 2, 0, pulse);
  }
  else if (min_days == 1) {
    // Tomorrow! — all 3 LEDs, brighter purple pulse
    med_leds[BDAY_LED_3] = CRGB(bright / 2, 0, bright);
    med_leds[BDAY_LED_2] = CRGB(bright / 2, 0, bright);
    med_leds[BDAY_LED_1] = CRGB(pulse,      0, pulse * 2 > 255 ? 255 : pulse * 2);
  }
  else if (min_days == 0) {
    // TODAY! — all 3 flash rainbow if not acked, else steady gold
    if (!g_bday_acked) {
      uint8_t hue = (millis() / 20) % 256;
      med_leds[BDAY_LED_3] = CHSV(hue,        255, bright);
      med_leds[BDAY_LED_2] = CHSV(hue + 85,   255, bright);
      med_leds[BDAY_LED_1] = flash_fast ? CRGB(CHSV(hue + 170, 255, bright)) : CRGB::Black;
    } else {
      // Acknowledged — steady soft gold
      med_leds[BDAY_LED_3] = CRGB(bright, bright / 2, 0);
      med_leds[BDAY_LED_2] = CRGB(bright, bright / 2, 0);
      med_leds[BDAY_LED_1] = CRGB(bright, bright / 2, 0);
    }
  }
}

// Check birthday accept button (GPIO19) — debounced
void bday_check_button(void) {
  bool btn_now = (digitalRead(BTN_BDAY) == LOW);
  if (btn_now && !g_bday_btn_prev) {
    // Acknowledge birthday if today — uses alarm volume, same as the
    // automatic 08:40 announcement, so a manual replay isn't quieter than
    // the original announcement was.
    if (g_bday_today) {
      g_bday_acked  = true;
      g_bday_played = true;
      dfplayer_volume(g_alarm_vol);
      dfplayer_play(BDAY_TRACK); voice_lock();
      bday_save();
    }
    // Acknowledge doctor appointment if today
    if (g_doc_today) {
      g_doc_acked  = true;
      g_doc_played = true;
      dfplayer_volume(g_vol);
      dfplayer_play(DOC_TRACK); voice_lock();
      doc_save();
    }
    // Accept/dismiss Wekkers if it's currently ringing — added 2026-06-24
    if (g_wekker_ringing) {
      g_wekker_ringing = false;
      g_alm_fire       = false;
      dfplayer_stop();
      Serial.println("[wekker] accepted/dismissed by button");
    }
    // Acknowledge today's Kalender event(s) — silent (no voice track for
    // general events), just stops the status-bar LED flashing for the rest
    // of the day
    if (g_evt_today && !g_evt_acked) {
      g_evt_acked = true;
      evt_save();
    }
  }
  g_bday_btn_prev = btn_now;
}

/******************************************************************************
 * Doctor Appointment Calendar Functions
 ******************************************************************************/

void doc_save(void) {
  Preferences p;
  p.begin("docappt", false);
  p.putUChar("count", g_doc_count);
  for (uint8_t i = 0; i < g_doc_count; i++) {
    String k = String(i);
    p.putUChar(("d"  + k).c_str(), g_docs[i].day);
    p.putUChar(("m"  + k).c_str(), g_docs[i].month);
    p.putString(("n" + k).c_str(), g_docs[i].desc);
  }
  p.putUChar("lastday", g_doc_last_day);
  p.putBool ("acked",   g_doc_acked);
  p.putBool ("played",  g_doc_played);
  p.putBool ("warned",  g_doc_warned);
  p.end();
}

void doc_load(void) {
  Preferences p;
  p.begin("docappt", true);
  g_doc_count   = p.getUChar("count", 0);
  if (g_doc_count > DOC_MAX) g_doc_count = DOC_MAX;
  for (uint8_t i = 0; i < g_doc_count; i++) {
    String k = String(i);
    g_docs[i].day   = p.getUChar(("d" + k).c_str(), 1);
    g_docs[i].month = p.getUChar(("m" + k).c_str(), 1);
    String nm = p.getString(("n" + k).c_str(), "");
    strncpy(g_docs[i].desc, nm.c_str(), 15);
    g_docs[i].desc[15] = '\0';
  }
  g_doc_last_day = p.getUChar("lastday", 255);
  g_doc_acked    = p.getBool ("acked",   false);
  g_doc_played   = p.getBool ("played",  false);
  g_doc_warned   = p.getBool ("warned",  false);
  p.end();
  // Remove past appointments automatically
  doc_purge_past();
}

// Remove appointments whose date has already passed
// FIX 4 (2026-07-03): days_until_bday() can NEVER return a negative number —
// a date that passed yesterday wraps around to ~364 ("next year's occurrence").
// The old "d >= 0" test was therefore always true, nothing was ever purged,
// and past appointments would re-trigger the cyan LED countdown and voice
// reminders again on the same date NEXT YEAR. A wrapped value near the top of
// the range means "just passed", so anything above 300 days is purged. 300
// leaves generous headroom: real appointments are entered days/weeks ahead,
// never 10+ months ahead.
void doc_purge_past(void) {
  uint8_t new_count = 0;
  for (uint8_t i = 0; i < g_doc_count; i++) {
    int16_t d = days_until_bday(g_docs[i].day, g_docs[i].month); // reuse same helper
    if (d >= 0 && d < 300) {  // today or genuinely upcoming — keep it
      g_docs[new_count++] = g_docs[i];
    }
  }
  if (new_count != g_doc_count) {
    g_doc_count = new_count;
    doc_save();
  }
}

// Call once per second
void doc_check(void) {
  // Reset flags on new day
  if (g_day != g_doc_last_day) {
    g_doc_last_day  = g_day;
    g_doc_acked     = false;
    g_doc_played    = false;
    g_doc_warned    = false;  // reset evening warning for new day
    g_doc_today     = false;
    g_doc_today_idx = 255;
    doc_purge_past();
    doc_save();
  }

  // Check if any appointment is TODAY
  g_doc_today     = false;
  g_doc_today_idx = 255;
  for (uint8_t i = 0; i < g_doc_count; i++) {
    if (g_docs[i].day == g_day && g_docs[i].month == g_month) {
      g_doc_today     = true;
      g_doc_today_idx = i;
      break;
    }
  }

  // Check if any appointment is TOMORROW — for evening-before warning.
  // Also captures which one, so the Pushover message below can name it.
  bool    doc_tomorrow     = false;
  uint8_t doc_tomorrow_idx = 255;
  for (uint8_t i = 0; i < g_doc_count; i++) {
    if (days_until_bday(g_docs[i].day, g_docs[i].month) == 1) {
      doc_tomorrow     = true;
      doc_tomorrow_idx = i;
      break;
    }
  }

  // Evening-before warning at 20:00 — "Doktersafspraak môre!" Uses alarm
  // volume, same as Birthday/Medicine, so a quiet evening setting can't
  // silence it — missing this means missing the appointment entirely.
  // Also pushed to the phone (Henry's request: this one genuinely matters).
  if (doc_tomorrow && !g_doc_warned && g_hh == 20 && g_mm == 0) {
    g_doc_warned = true;
    dfplayer_volume(g_alarm_vol);
    dfplayer_play(DOC_TRACK); voice_lock();
    if (doc_tomorrow_idx < g_doc_count) {
      po_notify("Doktersafspraak môre! / Doctor tomorrow!",
        String(g_docs[doc_tomorrow_idx].desc) + "\nTyd: " + po_time(), 0);
    }
    doc_save();
  }

  // Morning-of reminder at 07:00 on appointment day — same alarm-volume
  // treatment, local only (the day-before warning above is the one that
  // reaches the phone).
  if (g_doc_today && !g_doc_played && g_hh == 7 && g_mm == 0) {
    g_doc_played = true;
    dfplayer_volume(g_alarm_vol);
    dfplayer_play(DOC_TRACK); voice_lock();
    doc_save();
  }
}

// Render doctor LED — pill_leds[7]
// 3 days before: slow cyan pulse
// 2 days before: medium cyan pulse
// Tomorrow:      fast cyan flash
// Today:         fast white flash (urgent!) until acked, then steady cyan
void doc_render_led(void) {
  uint8_t bright = is_night_now() ? 20 : 100;

  // Find closest upcoming appointment
  int16_t min_days = 999;
  for (uint8_t i = 0; i < g_doc_count; i++) {
    int16_t d = days_until_bday(g_docs[i].day, g_docs[i].month);
    if (d >= 0 && d < min_days) min_days = d;
  }

  if (min_days > 3) {
    pill_leds[DOC_LED] = CRGB::Black;
    return;
  }

  uint8_t pulse_slow = (uint8_t)(30 + 30 * sin(millis() / 1200.0));
  uint8_t pulse_mid  = (uint8_t)(40 + 40 * sin(millis() /  700.0));
  uint8_t pulse_fast = (uint8_t)(50 + 50 * sin(millis() /  350.0));
  bool    flash      = ((millis() / 300) % 2 == 0);

  if (min_days == 3) {
    pill_leds[DOC_LED] = CRGB(0, pulse_slow, pulse_slow);        // soft cyan pulse
  } else if (min_days == 2) {
    pill_leds[DOC_LED] = CRGB(0, pulse_mid, pulse_mid);          // medium cyan pulse
  } else if (min_days == 1) {
    pill_leds[DOC_LED] = CRGB(0, pulse_fast, pulse_fast);        // fast cyan flash
  } else if (min_days == 0) {
    if (!g_doc_acked) {
      // TODAY — urgent white flash
      pill_leds[DOC_LED] = flash ? CRGB(bright, bright, bright) : CRGB::Black;
    } else {
      // Acknowledged — steady cyan
      pill_leds[DOC_LED] = CRGB(0, bright / 2, bright);
    }
  }
}

/******************************************************************************
 * Kalender general events
 ******************************************************************************/
//  Same architecture as doctor appointments: day/month entries in NVS, purged
//  automatically once the date wraps past. Feeds the /calendar month grid,
//  BAR3 LED slot 28 (magenta countdown), and a 07:00 Pushover on the day.

void evt_save(void) {
  Preferences p;
  p.begin("events", false);
  p.putUChar("count", g_evt_count);
  for (uint8_t i = 0; i < g_evt_count; i++) {
    String k = String(i);
    p.putUChar (("d" + k).c_str(), g_events[i].day);
    p.putUChar (("m" + k).c_str(), g_events[i].month);
    p.putString(("n" + k).c_str(), g_events[i].desc);
  }
  p.putBool("acked", g_evt_acked);
  p.end();
}

void evt_load(void) {
  Preferences p;
  p.begin("events", true);
  g_evt_count = p.getUChar("count", 0);
  if (g_evt_count > EVT_MAX) g_evt_count = EVT_MAX;
  for (uint8_t i = 0; i < g_evt_count; i++) {
    String k = String(i);
    g_events[i].day   = p.getUChar(("d" + k).c_str(), 1);
    g_events[i].month = p.getUChar(("m" + k).c_str(), 1);
    String n = p.getString(("n" + k).c_str(), "");
    strncpy(g_events[i].desc, n.c_str(), sizeof(g_events[i].desc) - 1);
    g_events[i].desc[sizeof(g_events[i].desc) - 1] = '\0';
  }
  g_evt_acked = p.getBool("acked", false);
  p.end();
}

// Same wrapped-past-300 purge logic as doc_purge_past() — see that comment.
void evt_purge_past(void) {
  uint8_t new_count = 0;
  for (uint8_t i = 0; i < g_evt_count; i++) {
    int16_t d = days_until_bday(g_events[i].day, g_events[i].month);
    if (d >= 0 && d < 300) g_events[new_count++] = g_events[i];
  }
  if (new_count != g_evt_count) { g_evt_count = new_count; evt_save(); }
}

// Call once per second (from timekeeping, next to doc_check)
void evt_check(void) {
  // New day: purge yesterday's events, reset the notify + accept flags
  if (g_day != g_evt_last_day) {
    g_evt_last_day  = g_day;
    g_evt_notified  = false;
    g_evt_acked     = false;
    evt_purge_past();
  }

  // Refresh today/nearest state for the LED
  g_evt_today    = false;
  g_evt_days_min = 999;
  for (uint8_t i = 0; i < g_evt_count; i++) {
    int16_t d = days_until_bday(g_events[i].day, g_events[i].month);
    if (d == 0) g_evt_today = true;
    if (d >= 0 && d < g_evt_days_min) g_evt_days_min = d;
  }

  // 07:00 morning-of Pushover listing today's events (minute-latch safe)
  if (g_evt_today && !g_evt_notified && g_hh == 7 && g_mm == 0 && g_new_minute) {
    g_evt_notified = true;
    String body = "";
    for (uint8_t i = 0; i < g_evt_count; i++) {
      if (g_events[i].day == g_day && g_events[i].month == g_month) {
        if (body.length()) body += "\n";
        body += "- " + String(g_events[i].desc);
      }
    }
    po_notify("Kalender vandag", body + "\nTyd: " + po_time(), 0);
  }
}

// ── Notaboekie NVS persistence ────────────────────────────────────────
void note_save(void) {
  Preferences p;
  p.begin("notepad", false);
  p.putUChar("count", g_note_count);
  for (uint8_t i = 0; i < g_note_count; i++) {
    String k = String(i);
    p.putString(("t" + k).c_str(), g_notes[i].text);
    p.putBool  (("x" + k).c_str(), g_notes[i].done);
  }
  p.end();
}

void note_load(void) {
  Preferences p;
  p.begin("notepad", true);
  g_note_count = p.getUChar("count", 0);
  if (g_note_count > NOTE_MAX) g_note_count = NOTE_MAX;
  for (uint8_t i = 0; i < g_note_count; i++) {
    String k = String(i);
    String t = p.getString(("t" + k).c_str(), "");
    strncpy(g_notes[i].text, t.c_str(), sizeof(g_notes[i].text) - 1);
    g_notes[i].text[sizeof(g_notes[i].text) - 1] = '\0';
    g_notes[i].done = p.getBool(("x" + k).c_str(), false);
  }
  p.end();
}

void play_chime_with_vol(uint16_t track) {
  uint8_t v = is_night_now() ? g_night_vol : g_vol;
  dfplayer_volume(v);
  dfplayer_play(track);
}

// ── Temperature-based colour ──────────────────────────────────────────────────
// Below  0°C  : Pure Blue   (  0,   0, 255) — Vriesend / Freezing
//  0 – 18°C   : Cyan        (  0, 255, 255) — Koud / Cold
// 19 – 23°C   : Azure       (  0, 127, 255) — Aangenaam / Comfortable
// 24 – 28°C   : Rose        (255,   0, 127) — Warm
// 29°C +      : Red         (255,   0,   0) — Baie Warm / Hot
void temp_colour(float tc, uint8_t &r, uint8_t &g, uint8_t &b) {
  if      (tc <  0.0f)  { r =   0; g =   0; b = 255; }  // Pure Blue  — Vriesend
  else if (tc < 19.0f)  { r =   0; g = 255; b = 255; }  // Cyan       — Koud
  else if (tc < 24.0f)  { r =   0; g = 127; b = 255; }  // Azure      — Aangenaam
  else if (tc < 29.0f)  { r = 255; g =   0; b = 127; }  // Rose       — Warm
  else                  { r = 255; g =   0; b =   0; }  // Red        — Baie Warm
}

// Lights only the bottom colon dot as a decimal point (in the temp display colour)
void draw_decimal_dot(uint8_t r, uint8_t g, uint8_t b) {
  uint16_t dot = pix(1, 7, 1);   // row 1 — one up from bottom row
  if(dot < NUM_LEDS) leds[dot] = CRGB(r, g, b);
  // clear other col 7 positions to prevent ghost dots
  uint16_t dot2 = pix(1, 7, 5);
  uint16_t dot3 = pix(1, 7, 2);
  uint16_t dot4 = pix(1, 7, 6);
  uint16_t dot5 = pix(1, 7, 0);  // clear bottom row
  if(dot2 < NUM_LEDS) leds[dot2] = CRGB::Black;
  if(dot3 < NUM_LEDS) leds[dot3] = CRGB::Black;
  if(dot4 < NUM_LEDS) leds[dot4] = CRGB::Black;
  if(dot5 < NUM_LEDS) leds[dot5] = CRGB::Black;
}

void rtc_sync(void) {
  DateTime now = rtc.now();
  g_hh    = now.hour();
  g_mm    = now.minute();
  g_ss    = now.second();
  g_day   = now.day();
  g_month = now.month();
  g_year  = now.year();                       // Month-grid math
  g_dow   = (uint8_t)now.dayOfTheWeek();      // 0=Sun..6=Sat, cached so
  // the /calendar web handler (Core 0) never has to touch the I2C bus that
  // Core 1 owns — same reason rtc_dow() calls from web pages are a known
  // (pre-existing, rare) cross-core risk.
}

// Returns the day-of-week from DS3231 (0=Sun … 6=Sat)
uint8_t rtc_dow(void) {
  return (uint8_t)(rtc.now().dayOfTheWeek());
}

void ntp_sync_to_rtc(void) {
  // configTime sets the ESP32 internal clock from NTP
  configTime((long)g_tz_offset_hours * 3600L, 0, NTP_SERVER1, NTP_SERVER2);
  // Wait up to 10 seconds for sync
  struct tm ti;
  uint8_t tries = 0;
  while(!getLocalTime(&ti, 1000) && ++tries < 10) {
    delay(200);
  }
  if(tries < 10) {
    // Write the NTP time into the DS3231
    DateTime ntp_dt(
      (uint16_t)(ti.tm_year + 1900),
      (uint8_t)(ti.tm_mon + 1),
      (uint8_t)ti.tm_mday,
      (uint8_t)ti.tm_hour,
      (uint8_t)ti.tm_min,
      (uint8_t)ti.tm_sec
    );
    rtc.adjust(ntp_dt);
    rtc_sync();
    g_ntp_synced = true;
    g_last_ntp_sync = millis();
    if(g_boot_time_unix == 0) {
      g_boot_time_unix = (uint32_t)mktime(&ti);
    }
    Serial.println("NTP sync OK");
  } else {
    Serial.println("NTP sync failed — using RTC");
  }
}

// Lightweight internet reachability probe. Runs on Core 0
// from wifi_task(). A plain TCP connect to a well-known host:port with a SHORT
// hard timeout — no DNS-heavy HTTPS, no payload, just "can I open a socket to
// the outside world". Updates g_inet_ok for the BAR3 LED. The 2s connect cap
// keeps Core 0 responsive; even a total failure costs ~2s once every 30s.
// Reuses the Pushover host (we KNOW it must be reachable for the alarm to
// work, so it's the most meaningful thing to probe) on port 443.
void inet_check(void) {
  if (WiFi.status() != WL_CONNECTED) { g_inet_ok = false; return; }
  WiFiClient c;
  c.setTimeout(2000);
  bool ok = c.connect(PO_HOST, 443, 2000);   // 2s connect budget, ms in all cores
  c.stop();
  g_inet_ok = ok;
}

/******************************************************************************
 * AUTOMATIC WIFI RECONNECT
 ******************************************************************************/
//  If the router/ONT hasn't finished restarting yet after a power cut, a
//  single connection attempt at boot can fail — and without a retry, the
//  clock would then sit offline until someone manually reboots it (the
//  physical GPIO2 hold-3s button and /config → Restart Clock remain
//  available as a manual backup, but shouldn't normally be needed).
//
//  When WiFi credentials are already saved (the normal case, not first-time
//  setup), this skips the captive portal entirely and instead:
//    1) tries to connect (bounded ~15s)
//    2) if that fails, waits 60 seconds and tries again
//    3) if that still fails, keeps retrying every ~3 minutes, forever
//  No button press, no phone, no portal — it quietly keeps trying until the
//  router comes back, then carries on as normal. This function blocks, but
//  it runs on Core 0's dedicated wifi_task — the display, buttons, alarms
//  and LEDs on Core 1 are completely unaffected, even during a long retry.
//
//  NOTE: this does not fall back to the setup portal if the saved
//  credentials are simply wrong (e.g. a new router with a new password) —
//  it retries the old credentials forever. To reconfigure WiFi from
//  scratch, use the physical factory-reset combo (hold SET + UP together
//  at power-on) to clear saved credentials, which brings the setup portal
//  back on the next boot.
bool wifi_connect_with_retries(void) {
  WiFi.mode(WIFI_STA);
  WiFi.begin();   // no args = reconnect with the credentials ESP32 already has saved

  uint32_t attempt = 0;
  for (;;) {
    uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - t0) < WIFI_CONNECT_ATTEMPT_TIMEOUT_MS) {
      vTaskDelay(250 / portTICK_PERIOD_MS);
    }
    if (WiFi.status() == WL_CONNECTED) {
      if (attempt > 0) Serial.printf("WiFi reconnected automatically after %lu retr%s.\n",
                                      (unsigned long)attempt, attempt == 1 ? "y" : "ies");
      return true;
    }

    uint32_t wait_ms = (attempt == 0) ? WIFI_RETRY_FIRST_WAIT_MS : WIFI_RETRY_INTERVAL_MS;
    attempt++;
    Serial.printf("WiFi not connected (loadshedding?) — retry #%lu in %lus...\n",
                  (unsigned long)attempt, (unsigned long)(wait_ms / 1000UL));
    vTaskDelay(wait_ms / portTICK_PERIOD_MS);

    WiFi.disconnect();
    WiFi.begin();   // try again with the same saved credentials
  }
}

/******************************************************************************
 * WiFi background task (FreeRTOS)
 ******************************************************************************/
//  Runs WiFiManager captive portal on first boot, then NTP syncs.
//  Pinned to core 0 so display loop on core 1 is never blocked.

void wifi_task(void* pv) {
  WiFiManager wm;
  wm.setConfigPortalTimeout(180);   // give up after 3 min if no phone connects (first-time setup only)

  // Custom parameter: timezone offset
  WiFiManagerParameter tz_param("tz", "Timezone offset (e.g. 2 for SAST)", "2", 4);
  wm.addParameter(&tz_param);

  // If credentials are already saved, skip the captive portal and use
  // the automatic retry loop above instead — see the big comment there.
  // The portal is still used exactly as before for genuine first-time setup
  // (no saved credentials yet).
  bool have_saved_creds = (WiFi.SSID().length() > 0);
  bool connected;
  if (have_saved_creds) {
    Serial.println("Saved WiFi credentials found — connecting automatically (no setup portal)...");
    connected = wifi_connect_with_retries();   // blocks until connected — retries forever
  } else {
    Serial.println("No saved WiFi credentials — starting setup portal for first-time setup...");
    connected = wm.autoConnect("OumaRiaSetup", "");
  }

  if(connected) {
    Serial.print("WiFi connected: ");
    Serial.println(WiFi.localIP());

    // Start mDNS — access via http://oumariaclock.local
    // NOTE: network identifiers (mDNS name, OTA hostname, setup-AP name,
    // Pushover app name) kept internally consistent so
    // saved bookmarks, the Arduino IDE network port, and the Pushover app
    // registration all keep working. Only the human-facing branding was
    // renamed. Change these too if you want — just re-bookmark afterwards.
    if(MDNS.begin("oumariaclock")) {
      MDNS.addService("http", "tcp", 80);
      Serial.println("mDNS started — http://oumariaclock.local");
    } else {
      Serial.println("mDNS failed — use IP address");
    }

    // Save any new timezone from portal
    int tz = atoi(tz_param.getValue());
    if(tz >= -12 && tz <= 14) {
      g_tz_offset_hours = (int16_t)tz;
      settings_save();
    }

    // Initial NTP sync
    ntp_sync_to_rtc();

    // First internet probe immediately on connect —
    // previously the LED sat "no internet" red for the first ~30s of uptime
    // until the periodic check fired.
    g_last_inet_chk = millis();
    inet_check();

    // Setup OTA
    ArduinoOTA.setPassword(OTA_PASSWORD);
    ArduinoOTA.setHostname("OumaRiaClock");
    ArduinoOTA.onStart([]() {
      Serial.println("OTA Start");
      // Render-race fix. These OTA callbacks run on
      // Core 0 and push FastLED while loop() on Core 1 is ALSO rendering —
      // two cores driving the RMT peripheral concurrently can glitch or
      // crash mid-flash. g_ota_active tells loop() to stand down so Core 0
      // owns the LEDs exclusively for the duration of the update.
      g_ota_active = true;
      vTaskDelay(60 / portTICK_PERIOD_MS);   // let Core 1 finish its current frame
      fill_solid(leds, NUM_LEDS, CRGB(0, 0, 100));
      leds_show_all();
    });
    ArduinoOTA.onProgress([](unsigned int p, unsigned int t) {
      uint8_t pct = (uint8_t)((p * 100UL) / t);
      // Simple progress bar on panel 0: fill LEDs proportionally
      uint8_t bar = (uint8_t)((pct * 64UL) / 100UL);
      fill_solid(leds, 64, CRGB::Black);
      fill_solid(leds, bar, CRGB(0, 150, 255));
      leds_show_all();
    });
    ArduinoOTA.onEnd([]() {
      fill_solid(leds, NUM_LEDS, CRGB(0, 200, 0));
      leds_show_all();
      delay(500);
      // no need to clear g_ota_active — the ESP32 reboots right after onEnd
    });
    ArduinoOTA.onError([](ota_error_t e) {
      Serial.printf("OTA Error %u\n", (unsigned)e);
      g_ota_active = false;   // Failed update — hand the LEDs back to Core 1
    });
    ArduinoOTA.begin();

    // Setup web server
    web_setup();
    webServer.begin();

    // Main WiFi loop: periodic NTP resync + OTA + web
    for(;;) {
      ArduinoOTA.handle();
      webServer.handleClient();

      // ── Check for pending Pushover notification — safe on Core 0 ────────────
      // Pops the HIGHEST-priority pending message (Panic/Alarm always jumps
      // ahead of Braai/Smoker chatter) — see po_queue_pop() for details.
      {
        uint8_t p; String t, b, s; uint16_t rt, ex;
        if (po_queue_pop(p, t, b, s, rt, ex)) {
          po_send(t, b, p, s, rt, ex);   // runs safely on Core 0
        }
      }

      // Resync NTP every NTP_SYNC_INTERVAL seconds
      // FIX 7 (2026-07-03): g_last_ntp_sync was only updated inside
      // ntp_sync_to_rtc() ON SUCCESS. If the 6-hour resync FAILED (internet
      // down but WiFi up — loadshedding special: router on battery, fibre
      // not), this condition stayed true on every 10ms pass and each retry
      // blocked Core 0 for ~10-12s in getLocalTime() — web portal, Arm/
      // Disarm/Panic buttons and the Pushover drain all frozen back-to-back
      // until the internet returned. Recording the ATTEMPT time first means
      // a failed sync simply retries again in 6 hours (the battery-backed
      // DS3231 keeps perfectly good time in the meantime); a successful sync
      // overwrites the timestamp inside ntp_sync_to_rtc() as before.
      uint32_t now_s = millis() / 1000UL;
      if((now_s - g_last_ntp_sync / 1000UL) >= NTP_SYNC_INTERVAL) {
        g_last_ntp_sync = millis();   // mark the attempt, success or not
        ntp_sync_to_rtc();
      }

      // Internet reachability probe every 30s for the
      // BAR3 "internet" LED. Same Core 0 safety logic as the NTP fix —
      // timestamp is recorded before the (bounded 2s) probe runs, so a down
      // uplink can't spin this into back-to-back retries.
      if((now_s - g_last_inet_chk / 1000UL) >= INET_CHECK_INTERVAL) {
        g_last_inet_chk = millis();
        inet_check();
      }

      // Mid-session WiFi drop — e.g. the router loses power during
      // loadshedding a while AFTER the clock already connected fine (clock
      // itself stays up on its own supply). WiFi.reconnect() just kicks off
      // a background attempt with the same saved credentials and returns
      // immediately — safe to call from inside this loop without blocking
      // OTA/web/Pushover servicing. Checked once a minute while down.
      if(WiFi.status() != WL_CONNECTED) {
        if((now_s - g_last_wifi_retry / 1000UL) >= WIFI_HEALTH_CHECK_INTERVAL_S) {
          g_last_wifi_retry = millis();
          Serial.println("WiFi link down mid-session — attempting automatic reconnect...");
          WiFi.reconnect();
        }
      }
      vTaskDelay(10 / portTICK_PERIOD_MS);
    }
  } else {
    // With the automatic retry loop above, this branch is now only
    // reached on a genuine first boot (no saved credentials) where nobody
    // completed setup within the 180s portal window. Loadshedding recovery
    // no longer lands here — see wifi_connect_with_retries().
    Serial.println("WiFi not configured — running offline. Reconnect to 'OumaRiaSetup' to set up WiFi, or restart to try the portal again.");
    // Still try RTC
    rtc_sync();
  }
  vTaskDelete(NULL);
}

/******************************************************************************
 * Web portal pages
 ******************************************************************************/

String html_header(const char* title) {
  String h = "<!DOCTYPE html><html><head><meta charset='utf-8'>";
  h += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  h += "<title>"; h += title; h += " — Ouma Ria Smart Clock</title>";  // renamed 
  h += "<style>body{font-family:sans-serif;max-width:480px;margin:1rem auto;padding:0 1rem;background:#111;color:#eee}";
  h += "h1{color:#ffa500;margin-bottom:0.5rem}nav a{color:#0af;margin-right:1rem;text-decoration:none}";
  h += "input,select{width:100%;padding:8px;margin:6px 0 12px;background:#222;color:#eee;border:1px solid #555;border-radius:4px;box-sizing:border-box}";
  h += "button,input[type=submit]{background:#ffa500;color:#000;border:none;padding:10px 20px;border-radius:4px;cursor:pointer;font-size:1rem;width:100%}";
  h += ".stat{background:#1a1a1a;border:1px solid #333;border-radius:6px;padding:10px;margin:6px 0}";
  h += ".label{color:#888;font-size:0.8rem}.val{font-size:1.5rem;font-weight:bold;color:#ffa500}</style></head>";
  h += "<body style='padding-bottom:84px'><h1>&#9201; Ouma Ria Smart Clock</h1>";
  // Ouma UX: floating HUIS button on EVERY page — one big
  // 🏠 pinned bottom-right, always under the thumb even when scrolled deep in
  // the kalender or braaitimes. One edit here covers all pages, present and
  // future, since every page builds its header through this function. The
  // body gets bottom padding so content never hides behind the button.
  h += "<a href='/' style='position:fixed;bottom:18px;right:16px;width:60px;height:60px;"
       "background:#0a84ff;color:#fff;border-radius:50%;display:flex;align-items:center;"
       "justify-content:center;font-size:1.8rem;text-decoration:none;"
       "box-shadow:0 3px 12px rgba(0,0,0,0.6);z-index:999'>&#127968;</a>";
  h += "<nav><a href='/'>Status</a><a href='/bar3'>&#128247; Status Bar</a><a href='/ledmanual'>&#128161; LED Manual</a><a href='/buttons'>&#128280; Knoppies</a><a href='/alarms'>Alarms</a><a href='/wekkers'>Wekkers</a><a href='/medicine'>Medicine</a><a href='/egg'>&#9201; Egg Timer</a><a href='/smoker'>&#128293; Smoker</a><a href='/braai'>&#129385; Braai</a><a href='/braaitimes'>&#128203; Braai Times</a><a href='/birthdays'>&#127874; Verjaarsdae</a><a href='/docappts'>&#128203; Dokter</a><a href='/calendar'>&#128197; Kalender</a><a href='/notepad'>&#128221; Nota</a><a href='/alarm'>&#128274; Alarm</a><a href='/config'>Config</a><a href='/set'>Set time</a><a href='/ota'>OTA</a></nav><hr>";
  return h;
}

String html_footer(void) {
  return "<br><hr><small style='color:#555'>" + String(FIRMWARE_VERSION) + "</small></body></html>";
}

void web_setup(void) {

  // ── / : Status page ─────────────────────────────────────────────────────────
  webServer.on("/", HTTP_GET, []() {
    String page = html_header("Status");
    char buf[32];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", g_hh, g_mm, g_ss);
    page += "<div class='stat' style='text-align:center'><div class='label' style='text-align:center'>Current time</div>"
            "<div style='font-size:3.2rem;font-weight:bold;color:#ffa500;text-align:center;line-height:1.1;font-family:monospace'>"
            + String(buf) + "</div></div>";

    float temp = rtc.getTemperature();
    snprintf(buf, sizeof(buf), "%.1f &deg;C / %.1f &deg;F", temp, temp * 1.8f + 32.0f);
    page += "<div class='stat' style='text-align:center'><div class='label' style='text-align:center'>DS3231 temperature</div>"
            "<div style='font-size:2.2rem;font-weight:bold;color:#ffa500;text-align:center;line-height:1.1'>"
            + String(buf) + "</div></div>";

    // ── Day-of-week strip — NEW 2026-06-29, Henry's request ─────────────────
    // Added here (main Status page, under Time/Temp) rather than just on
    // /bar3 — this is the page that's open most often, and the day strip
    // is meant as a quick "what day is it" check first thing in the
    // morning, plus a cross-check against the physical day-marked pill
    // box. Doesn't need live polling like /bar3 — the day only changes
    // once every 24 hours, so a plain server-rendered row (correct at
    // whatever moment the page loads/reloads) is all that's needed here.
    // Same Monday=0...Sunday=6 convention as pill_render() uses.
    {
      uint8_t dow_today = (rtc_dow() + 6) % 7;
      const char* dow_letters[7] = {"M","T","W","T","F","S","S"};
      page += "<div style='background:#1a1a1a;border:1px solid #333;border-radius:10px;padding:12px;margin:10px 0;"
              "display:flex;justify-content:center;gap:10px'>";
      for(uint8_t d = 0; d < 7; d++) {
        bool is_today = (d == dow_today);
        page += "<div style='text-align:center'>";
        if(is_today) {
          page += "<div style='width:28px;height:28px;border-radius:50%;background:#39e07a;"
                  "border:2px solid #5fffa0;margin:0 auto 4px'></div>";
        } else {
          page += "<div style='width:28px;height:28px;border-radius:50%;background:#1a4a2a;"
                  "border:2px solid #2a5a3a;margin:0 auto 4px'></div>";
        }
        page += "<span style='color:#777;font-size:0.75em'>" + String(dow_letters[d]) + "</span>";
        page += "</div>";
      }
      page += "</div>";
    }

    // ── Diagnostics — compacted into one small line (was 4 separate boxes) ──
    // Kept for troubleshooting, but de-emphasised so it doesn't dominate the
    // top of the page on a phone. Tap targets below are the main focus now.
    {
      uint32_t up = millis() / 1000UL;
      snprintf(buf, sizeof(buf), "%ud %02uh %02um", up/86400, (up%86400)/3600, (up%3600)/60);
      page += "<p style='color:#666;font-size:0.78rem;margin:4px 0 10px'>";
      page += "Uptime " + String(buf) + " &nbsp;|&nbsp; NTP " + String(g_ntp_synced?"Yes":"No");
      page += " &nbsp;|&nbsp; WiFi " + WiFi.localIP().toString();
      // Live internet reachability, refreshed every 30s —
      // distinct from NTP (which only proves the internet worked at last 6h
      // sync) and from WiFi (which is only the router link).
      page += " &nbsp;|&nbsp; Internet " + String(g_inet_ok ? "<span style='color:#0f0'>OK</span>" : "<span style='color:#f00'>NO</span>");
      page += " &nbsp;|&nbsp; DFPlayer <span style='color:" + String(g_dfp_alive ? "#0f0" : "#f00") +
              "'>" + String(g_dfp_alive ? "OK" : "NO RESPONSE") + "</span></p>";
    }

    // ── Big tappable shortcuts — Egg Timer, Smoker & Braai + Alarm & Emergency ─
    // Added per request: easier to tap on a phone than the small top nav bar,
    // and means no scrolling to the bottom of Status for these. Always visible
    // regardless of the Braai Timer's own active/inactive tickbox — easy to
    // reach the page to switch it on in the first place.
    // Second row added — Alarm (to /alarm page) and
    // Emergency (fires sec_panic() via /alarm/panic). Emergency carries a
    // JS confirm() dialog: one accidental tap would otherwise blast the siren
    // and send a priority-2 Pushover — one extra deliberate tap is cheap
    // insurance, and still far quicker than navigating to the alarm page.
    page += "<div style='display:flex;gap:8px;margin:6px 0 8px'>";
    page += "<a href='/egg' style='flex:1;display:block;text-align:center;background:#FF8C00;"
            "color:#fff;font-weight:bold;font-size:0.95rem;padding:14px 4px;border-radius:8px;"
            "text-decoration:none'>&#9201; Egg Timer</a>";
    page += "<a href='/smoker' style='flex:1;display:block;text-align:center;background:#cc5500;"
            "color:#fff;font-weight:bold;font-size:0.95rem;padding:14px 4px;border-radius:8px;"
            "text-decoration:none'>&#128293; Smoker</a>";
    page += "<a href='/braai' style='flex:1;display:block;text-align:center;background:#a64500;"
            "color:#fff;font-weight:bold;font-size:0.95rem;padding:14px 4px;border-radius:8px;"
            "text-decoration:none'>&#129385; Braai</a>";
    page += "</div>";
    page += "<div style='display:flex;gap:8px;margin:0 0 14px'>";
    page += "<a href='/alarm' style='flex:1;display:block;text-align:center;background:#0055aa;"
            "color:#fff;font-weight:bold;font-size:0.95rem;padding:14px 4px;border-radius:8px;"
            "text-decoration:none'>&#128274; Alarm</a>";
    page += "<a href='/alarm/panic' onclick=\"return confirm('AKTIVEER NOODALARM?\\nSirene + Pushover na foon!\\n\\nACTIVATE EMERGENCY?')\" "
            "style='flex:1;display:block;text-align:center;background:#cc0000;"
            "color:#fff;font-weight:bold;font-size:0.95rem;padding:14px 4px;border-radius:8px;"
            "text-decoration:none'>&#128680; Emergency</a>";
    page += "</div>";

    // ── Combined Medicine tile ────────────────────────────
    // with its own page link — confusing, since pills ARE medicine. Now one
    // "Medisyne" tile with pille/medikasie/ander as rows inside it, one link
    // to /medicine, and the pill page reached FROM the medicine page (it
    // became the 'subdirectory'). All status logic and the underlying pill/
    // med systems (separate NVS, separate LEDs, separate buttons) unchanged.
    // ── Medicine tile : Pille row REMOVED ──────────────────
    // Merged the two tiles;  retires the daily-pills subsystem
    // entirely (PILLS_ENABLED 0) — Medicine's dosed schedule covers it, and
    // "Medikasie 100% but Pille NIE GENEEM" double-talk is gone.
    page += "<div class='stat'><div class='label'>Ouma se Medisyne vandag</div>";
#if PILLS_ENABLED
    // Row 1: daily pills (only exists if the subsystem is resurrected)
    page += "<div style='margin:4px 0'><span style='color:#888;font-size:0.85em'>Pille: </span>";
    if (!g_pills_active) {
      page += "<span class='val' style='color:#0f0'>No Daily Pills</span>";
    } else if (g_pill_taken) {
      page += "<span class='val' style='color:#0f0'>GENEEM / TAKEN</span>";
    } else {
      page += "<span class='val' style='color:#f00'>NIE GENEEM / NOT YET</span>";
    }
    page += "</div>";
#endif
    // Dosed medicine
    page += "<div style='margin:4px 0'><span style='color:#888;font-size:0.85em'>Medikasie: </span>";
    if (g_med_doses > 0) {
      uint8_t taken_count = 0;
      for(uint8_t i=0;i<g_med_doses;i++) if(g_med[i].taken) taken_count++;
      String med_col = (taken_count == g_med_doses) ? "#0f0" : "#f00";
      page += "<span class='val' style='color:" + med_col + "'>" + String(taken_count) + " of " + String(g_med_doses) + " doses taken (" + String(g_med_doses) + "x day)</span>";
    } else {
      page += "<span class='val' style='color:#0f0'>Geen Medikasie vir Vandag</span>";
    }
    page += "</div>";
    // Row 4: ad-hoc extra doses today (only when any logged) — 
    if (g_adhoc_count > 0) {
      page += "<div style='margin:4px 0'><span style='color:#888;font-size:0.85em'>Ekstra (ongeskeduleer): </span>";
      page += "<span class='val' style='color:#ffa500'>" + String(g_adhoc_count) + "x vandag</span></div>";
    }
    // Row 3: Kort Kursus / short course : both slots
    for (uint8_t s = 0; s < COURSE_SLOTS; s++) {
      Course& c = g_course[s];
      if (c.doses == 0 || c.days_left == 0) continue;
      uint8_t taken_count = 0;
      for (uint8_t i = 0; i < c.doses; i++) if (c.taken[i]) taken_count++;
      uint8_t day_num = c.days_total - c.days_left + 1;
      String ex_col = (taken_count == c.doses) ? "#0f0" : "#f00";
      page += "<div style='margin:4px 0'><span style='color:#888;font-size:0.85em'>" + String(c.name) + " (Dag " + String(day_num) + "/" + String(c.days_total) + "): </span>";
      page += "<span class='val' style='color:" + ex_col + "'>" + String(taken_count) + " of " + String(c.doses) + " geneem</span></div>";
    }
    page += "</div>";
    page += "<p><a href='/medicine' style='color:#0af'>Gaan na Medisyne bladsy / Go to medicine page</a></p>";
    /* ── ORIGINAL separate pill + medicine tiles, merged 2026-07-04 ────────────
    // Pill status on main page
    page += "<div class='stat'><div class='label'>Ouma se pille vandag</div>";
    ... (two tiles + '/pills' link — see v15 for the full original)
 ── end original ─────────────────────────────────────────────────────────── */
    // Birthday status
    if (g_bday_today && g_bday_today_idx < g_bday_count) {
      page += "<p style='color:#FFD700;font-size:1.1em'>&#127881; Vandag: <b>" + String(g_bdays[g_bday_today_idx].name) + "</b> se verjaarsdag!</p>";
    }
    page += "<p><a href='/birthdays' style='color:#FFD700'>&#127874; Gaan na Verjaarsdagkalender</a></p>";
    if (g_doc_today && g_doc_today_idx < g_doc_count) {
      page += "<p style='color:#00CFCF;font-size:1.1em'>&#128680; Vandag: <b>" + String(g_docs[g_doc_today_idx].desc) + "</b> afspraak!</p>";
    }
    page += "<p><a href='/docappts' style='color:#00CFCF'>&#128203; Gaan na Doktersafsprake</a></p>";
    page += "<p><a href='/calendar' style='color:#c837c8'>&#128197; Gaan na Kalender</a></p>";  // Page += "<p><a href='/notepad' style='color:#e0c020'>&#128221; Notaboekie (inkopies)</a></p>";  // 

    // ── Security alarm status ─────────────────────────────────────────────────
    String alm_col = (g_panic_active || g_sec_triggered) ? "#ff0000" :
                     (g_z2_armed    ? "#ff3300" :
                     (g_z1_active   ? "#00aaff" : "#888888"));
    String alm_txt = g_panic_active    ? "&#128680; PANIC — EMERGENCY!" :
                     g_sec_triggered   ? (g_trig_zone == 1
                       ? "&#128680; ALARM! INTRUDER IN COURTYARD!"
                       : "&#128680; ALARM! INTRUDER INSIDE!") :
                     g_sec_entry_delay ? "&#9888; ENTRY DELAY — DISARM NOW!" :
                     g_sec_exit_delay  ? "&#9201; EXIT DELAY — LEAVING..." :
                     g_z2_armed        ? "&#128274; AWAY — FULLY ARMED" :
                     g_z1_active       ? "&#127968; HOME — Courtyard Armed" :
                                         "&#128275; ALL DISARMED";
    page += "<div class='stat' style='border:2px solid " + alm_col + "'>";
    page += "<div class='label'>Security Alarm</div>";
    page += "<div class='val' style='color:" + alm_col + "'>" + alm_txt + "</div></div>";
    page += "<p><a href='/alarm' style='color:#ff6633'>&#128274; Go to Alarm page</a></p>";

    page += html_footer();
    webServer.send(200, "text/html", page);
  });

  // ── /set : Manual time set ───────────────────────────────────────────────────
  webServer.on("/set", HTTP_GET, []() {
    String page = html_header("Set time");
    page += "<form method='POST'>";
    page += "<label>Hour (0-23): <input name='hh' type='number' min='0' max='23' value='" + String(g_hh) + "'></label>";
    page += "<label>Minute (0-59): <input name='mm' type='number' min='0' max='59' value='" + String(g_mm) + "'></label>";
    page += "<label>Second (0-59): <input name='ss' type='number' min='0' max='59' value='0'></label>";
    page += "<label>Day (1-31): <input name='dd' type='number' min='1' max='31' value='" + String(g_day) + "'></label>";
    page += "<label>Month (1-12): <input name='mo' type='number' min='1' max='12' value='" + String(g_month) + "'></label>";
    page += "<label>Year: <input name='yr' type='number' min='2024' max='2099' value='" + String(rtc.now().year()) + "'></label>";
    page += "<input type='submit' value='Set time'></form>";
    page += html_footer();
    webServer.send(200, "text/html", page);
  });

  webServer.on("/set", HTTP_POST, []() {
    int hh = webServer.arg("hh").toInt();
    int mm = webServer.arg("mm").toInt();
    int ss = webServer.arg("ss").toInt();
    int dd = webServer.arg("dd").toInt();
    int mo = webServer.arg("mo").toInt();
    int yr = webServer.arg("yr").toInt();
    if(hh>=0&&hh<24&&mm>=0&&mm<60&&ss>=0&&ss<60&&dd>=1&&dd<=31&&mo>=1&&mo<=12&&yr>=2024) {
      rtc.adjust(DateTime((uint16_t)yr,(uint8_t)mo,(uint8_t)dd,(uint8_t)hh,(uint8_t)mm,(uint8_t)ss));
      rtc_sync();
    }
    webServer.sendHeader("Location", "/");
    webServer.send(302);
  });

  // ── /alarms : Alarm config ───────────────────────────────────────────────────
  webServer.on("/alarms", HTTP_GET, []() {
    String page = html_header("Alarms");
    page += "<form method='POST' action='/alarm_save'>";
    page += "<h3 style='color:#0af'>Alarm</h3>";
    page += "<label>Hour: <input name='hh' type='number' min='0' max='23' value='" + String(g_alarm1.hh) + "'></label>";
    page += "<label>Minute: <input name='mm' type='number' min='0' max='59' value='" + String(g_alarm1.mm) + "'></label>";
    page += "<label>Enabled: <input name='en' type='checkbox'" + String(g_alarm1.enabled?" checked":"") + " value='1'></label><br><br>";
    page += "<input type='submit' value='Save alarm'></form><hr>";
    page += html_footer();
    webServer.send(200, "text/html", page);
  });

  webServer.on("/alarm_save", HTTP_POST, []() {
    int hh = webServer.arg("hh").toInt();
    int mm = webServer.arg("mm").toInt();
    if(hh>=0&&hh<24) g_alarm1.hh = (uint8_t)hh;
    if(mm>=0&&mm<60) g_alarm1.mm = (uint8_t)mm;
    g_alarm1.enabled = (webServer.arg("en") == "1");
    settings_save();
    webServer.sendHeader("Location", "/alarms");
    webServer.send(302);
  });

  // ── /wekkers : three independent daily alarms, separate from the single
  // wake Alarm — each with its own time, day-of-week mask, and enabled
  // state. A firing Wekker repeats every 20 seconds (up to 6 times) until
  // acknowledged via BTN_BDAY, the same accept/dismiss button shared with
  // birthdays and doctor appointments.
  webServer.on("/wekkers", HTTP_GET, []() {
    String page = html_header("Wekkers");
    page += "<h2 style='color:#0af'>&#9200; Wekkers</h2>";
    page += "<p style='color:#888'>3 onafhanklike wekkers &mdash; herhaal elke 20 sekondes tot 6 keer, "
            "of totdat die aanvaar-knoppie dit stop. / 3 independent alarms &mdash; repeat every 20s "
            "up to 6 times, or until the accept button stops it.</p>";
    const char* t2_days_names[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
    for(uint8_t i = 0; i < NUM_ALARM_SLOTS; i++) {
      page += "<form method='POST' action='/wekkers_save'>";
      page += "<input type='hidden' name='slot' value='" + String(i) + "'>";
      page += "<h3 style='color:#0af'>Wekker " + String(i+1) + "</h3>";
      page += "<label>Hour: <input name='hh' type='number' min='0' max='23' value='" + String(g_wekker_hh[i]) + "'></label>";
      page += "<label>Minute: <input name='mm' type='number' min='0' max='59' value='" + String(g_wekker_mm[i]) + "'></label>";
      page += "<label>Enabled: <input name='en' type='checkbox'" + String(g_wekker_enabled[i]?" checked":"") + " value='1'></label><br><br>";
      page += "<label>Days: ";
      for(uint8_t d = 0; d < 7; d++) {
        bool checked = (g_wekker_days[i] >> d) & 1;
        page += "<label style='display:inline;margin-right:8px'><input type='checkbox' name='d" + String(d) + "'" +
                (checked?" checked":"") + " value='1'> " + t2_days_names[d] + "</label>";
      }
      page += "</label><br><br><input type='submit' value='Save Wekker " + String(i+1) + "'></form>";
      page += "<p style='color:#888;font-size:0.85em'>Already fired today: " + String(g_wekker_fired_today[i]?"Yes":"No") + "</p><hr>";
    }
    page += html_footer();
    webServer.send(200, "text/html", page);
  });

  webServer.on("/wekkers_save", HTTP_POST, []() {
    int slot = webServer.arg("slot").toInt();
    if(slot >= 0 && slot < NUM_ALARM_SLOTS) {
      int hh = webServer.arg("hh").toInt();
      int mm = webServer.arg("mm").toInt();
      if(hh>=0 && hh<24) g_wekker_hh[slot] = (uint8_t)hh;
      if(mm>=0 && mm<60) g_wekker_mm[slot] = (uint8_t)mm;
      g_wekker_enabled[slot] = (webServer.arg("en") == "1");
      uint8_t t2mask = 0;
      for(uint8_t d = 0; d < 7; d++) {
        String key = "d" + String(d);
        if(webServer.arg(key) == "1") t2mask |= (1 << d);
      }
      // Respect the actual submitted selection, including zero days — a
      // Wekker with no days ticked simply never matches rtc_dow() and stays
      // silent, which is exactly what unchecking every box should mean.
      // (Previously this silently forced an all-zero save back to "every
      // day", overriding an explicit choice — the 0x7F fallback still
      // protects wekker_load() against corrupted/uninitialized NVS on first
      // boot, which is a different, legitimate concern from a real save.)
      g_wekker_days[slot] = t2mask;
      g_wekker_fired_today[slot] = false;   // allow it to fire today if time already passed
      wekker_save();   // persist to NVS — survives reboot/power cut
    }
    webServer.sendHeader("Location", "/wekkers");
    webServer.send(302);
  });

  // ── /config : Clock config ───────────────────────────────────────────────────
  webServer.on("/config", HTTP_GET, []() {
    String page = html_header("Config");
    page += "<form method='POST'>";
    page += "<label>Day volume (0-30): <input name='vol' type='number' min='0' max='30' value='" + String(g_vol) + "'></label>";
    page += "<label>Night volume (0-30): <input name='nvol' type='number' min='0' max='30' value='" + String(g_night_vol) + "'></label>";
    page += "<label>Alarm volume (0-30): <input name='avol' type='number' min='0' max='30' value='" + String(g_alarm_vol) + "'></label>";
    page += "<p style='color:#888;font-size:0.85em;margin:-8px 0 14px'>Used by Alarm, Wekkers, the Egg Timer alert, Birthdays and Doctor Appointments — "
            "kept separate so a quiet day/night setting never makes these too soft to hear. "
            "Everyday medicine and Smoker/Braai reminders follow day/night volume.</p>";
    // Note MOVED here from below the brightness fields —
    // it rendered under "Night brightness" and read like it described
    // brightness, leaving the volume fields unexplained. Wording updated for
    // the egg timer now following alarm volume.
    page += "<label>Day brightness (0-255): <input name='brday' type='number' min='0' max='255' value='" + String(g_bright_day) + "'></label>";
    page += "<label>Night brightness (0-255): <input name='brnight' type='number' min='0' max='255' value='" + String(g_bright_night) + "'></label>";
    page += "<label>Sound set (0-4): <input name='snd' type='number' min='0' max='4' value='" + String(g_sound_set) + "'></label>";
    page += "<label>Timezone offset (e.g. 2 for SAST): <input name='tz' type='number' min='-12' max='14' value='" + String(g_tz_offset_hours) + "'></label>";
    page += "<label>12-hour display: <input name='hr12' type='checkbox'" + String(g_use_12hr?" checked":"") + " value='1'></label><br><br>";
    page += "<label>Temperature °F: <input name='tempF' type='checkbox'" + String(g_temp_degF?" checked":"") + " value='1'></label><br><br>";
    page += "<label>Chimes enabled: <input name='chime' type='checkbox'" + String(g_chime_en?" checked":"") + " value='1'></label><br><br>";
    page += "<input type='submit' value='Save config'></form>";
    page += "<p style='color:#888;font-size:0.85em;margin-top:14px'>Factory reset is physical-only now: "
            "hold SET + UP together for 3 seconds at power-on.</p>";
    // UPDATE: WiFi reconnect after loadshedding is now AUTOMATIC — the
    // clock retries by itself (60s, then every ~3 min, forever), no button
    // or page visit needed. This restart button and the Blue button
    // (Sw10, GPIO2, hold 3s) are kept as manual backups for anything the
    // automatic retry can't fix on its own (e.g. a genuine hang).
    page += "<hr><h3 style='color:#ffa500'>Restart Clock</h3>";
    page += "<p style='color:#888;font-size:0.85em'>WiFi reconnects automatically by itself after loadshedding — "
            "you shouldn't normally need this. Kept as a manual backup: reboots the clock immediately instead of "
            "waiting for the automatic retry. The RTC keeps the correct time either way. If this page isn't "
            "reachable at all (no WiFi), hold the Blue button (Sw10, GPIO2) for 3 seconds instead.</p>";
    page += "<form method='POST' action='/restart' onsubmit=\"return confirm('Restart the clock now?');\">"
            "<input type='submit' value='Restart Clock' style='background:#a00000'></form>";
    page += html_footer();
    webServer.send(200, "text/html", page);
  });

  // -- /restart : manual reboot (retry WiFi connect) — Henry's request ────────
  webServer.on("/restart", HTTP_POST, []() {
    webServer.send(200, "text/html", "<p>Restarting — reconnecting to WiFi...</p>");
    delay(500);
    ESP.restart();
  });

  webServer.on("/config", HTTP_POST, []() {
    int v = webServer.arg("vol").toInt();   if(v>=0&&v<=30) g_vol = (uint8_t)v;
    v = webServer.arg("nvol").toInt();      if(v>=0&&v<=30) g_night_vol = (uint8_t)v;
    v = webServer.arg("avol").toInt();      if(v>=0&&v<=30) g_alarm_vol = (uint8_t)v;
    v = webServer.arg("brday").toInt();     if(v>=0&&v<=255) g_bright_day   = (uint8_t)v;
    v = webServer.arg("brnight").toInt();   if(v>=0&&v<=255) g_bright_night = (uint8_t)v;
    v = webServer.arg("snd").toInt();       if(v>=0&&v<=4)  g_sound_set = (uint8_t)v;
    v = webServer.arg("tz").toInt();        if(v>=-12&&v<=14) g_tz_offset_hours = (int16_t)v;
    g_use_12hr  = (webServer.arg("hr12")  == "1");
    g_temp_degF = (webServer.arg("tempF") == "1");
    g_chime_en  = (webServer.arg("chime") == "1");
    settings_save();
    webServer.sendHeader("Location", "/config");
    webServer.send(302);
  });

  // ── /factoryreset (REMOVED 2026-07-01) ────────────────────────────────
  // The web button was one tap + one JS confirm() away from wiping every
  // birthday, doctor appt, Wekker, medicine schedule, config value, and the
  // saved WiFi network. Removed at Henry's request — factory reset is now
  // physical-only: hold BTN_SET + BTN_UP together for 3 seconds at boot.
  // See check_factory_reset() below.

  // ── /pills : Pill reminder status and config ─────────────────────────────────
#if PILLS_ENABLED   // Page + its three POST routes compiled out — daily-pills retired
  webServer.on("/pills", HTTP_GET, []() {
    String page = html_header("Pill Reminder");
    const char* day_names[] = {"Monday","Tuesday","Wednesday","Thursday",
                               "Friday","Saturday","Sunday"};
    uint8_t today = (rtc_dow() + 6) % 7;  // Monday=0 ... Sunday=6

    page += "<h3 style='color:#0af'>This Week</h3>";
    page += "<table style='width:100%;border-collapse:collapse'>";
    page += "<tr><th style='background:#e07b00;color:#fff;padding:8px'>Day</th>";
    page += "<th style='background:#c05a00;color:#fff;padding:8px'>Status</th></tr>";
    for (uint8_t i = 0; i < 7; i++) {
      String bg = (i == today) ? "#1a1a1a" : "#111";
      String border = (i == today) ? "2px solid #e07b00" : "1px solid #333";
      page += "<tr style='border:" + border + "'>";
      page += "<td style='padding:8px;background:" + bg + ";font-weight:bold'>";
      page += String(day_names[i]);
      if (i == today) page += " <span style='color:#e07b00'>(today)</span>";
      page += "</td><td style='padding:8px;background:" + bg + "'>";
      if (i < today) {
        page += "<span style='color:#888'>Done</span>";
      } else if (i == today) {
        if (!g_pills_active) {
          page += "<span style='color:#0f0;font-size:1.2rem;font-weight:bold'>No Pills for Today</span>";
        } else if (g_pill_taken) {
          page += "<span style='color:#0f0;font-size:1.2rem;font-weight:bold'>GENEEM / TAKEN</span>";
        } else {
          page += "<span style='color:#f00;font-size:1.2rem;font-weight:bold'>NIE GENEEM / NOT YET</span>";
        }
      } else {
        page += "<span style='color:#444'>Upcoming</span>";
      }
      page += "</td></tr>";
    }
    page += "</table><br>";

    if (!g_pills_active) {
      page += "<div class='stat' style='border:2px solid #1a7a1a'>";
      page += "<div class='val' style='color:#0f0'>No Pills for Today</div></div><br>";
    } else if (!g_pill_taken) {
      page += "<form method='POST' action='/pill_confirm'>";
      page += "<input type='submit' value='PILLE GENEEM! / PILLS TAKEN!' ";
      page += "style='background:#1a7a1a;font-size:1.1rem;padding:15px'></form><br>";
    } else {
      page += "<div class='stat'><div class='val' style='color:#0f0'>Baie dankie Ouma! Pills taken today.</div></div><br>";
    }

    // Pills active toggle
    page += "<h3 style='color:#0af'>Pills Active</h3>";
    page += "<form method='POST' action='/pill_active'>";
    page += "<label style='font-size:1rem'>Ouma must take daily pills: ";
    page += "<input name='active' type='checkbox'" + String(g_pills_active?" checked":"") + " value='1'></label><br><br>";
    page += "<input type='submit' value='Save' style='width:auto;padding:10px 30px'></form><br>";

    page += "<h3 style='color:#0af'>Pill Reminder Alarm</h3>";
    page += "<form method='POST' action='/pill_alarm'>";
    page += "<label>Hour (0-23): <input name='hh' type='number' min='0' max='23' value='" + String(g_pill_alm_hh) + "'></label>";
    page += "<label>Minute (0-59): <input name='mm' type='number' min='0' max='59' value='" + String(g_pill_alm_mm) + "'></label>";
    page += "<label>Enabled: <input name='en' type='checkbox'" + String(g_pill_alm_en?" checked":"") + " value='1'></label><br><br>";
    page += "<input type='submit' value='Save pill alarm'></form>";
    page += html_footer();
    webServer.send(200, "text/html", page);
  });

  webServer.on("/pill_confirm", HTTP_POST, []() {
    pill_confirm();
    webServer.sendHeader("Location", "/pills");
    webServer.send(302);
  });

  webServer.on("/pill_active", HTTP_POST, []() {
    g_pills_active = (webServer.arg("active") == "1");
    if (!g_pills_active) g_pill_taken = false;  // reset when deactivated
    pill_save();
    webServer.sendHeader("Location", "/pills");
    webServer.send(302);
  });

  webServer.on("/pill_alarm", HTTP_POST, []() {
    int hh = webServer.arg("hh").toInt();
    int mm = webServer.arg("mm").toInt();
    if (hh >= 0 && hh < 24) g_pill_alm_hh = (uint8_t)hh;
    if (mm >= 0 && mm < 60) g_pill_alm_mm = (uint8_t)mm;
    g_pill_alm_en = (webServer.arg("en") == "1");
    pill_save();
    webServer.sendHeader("Location", "/pills");
    webServer.send(302);
  });
#endif  // PILLS_ENABLED — end of retired /pills routes


  // ── /medicine : Medicine schedule ────────────────────────────────────────────
  webServer.on("/medicine", HTTP_GET, []() {
    String page = html_header("Medicine Schedule");
    // The  "Daaglikse Pille" button was removed along
    // with the retired daily-pills subsystem — Medicine is now the one and
    // only medication system.
    const char* dose_names[] = {"Dose 1","Dose 2","Dose 3","Dose 4"};

    // Today status
    page += "<h3 style='color:#0af'>Today's Medicine</h3>";
    if (g_med_doses == 0) {
      page += "<div class='stat' style='border:2px solid #1a7a1a'>";
      page += "<div class='label'>Medicine status</div>";
      page += "<div class='val' style='color:#0f0;font-size:1.4rem'>Geen Medikasie vir Vandag</div>";
      page += "<div class='label'>No active prescription. Set doses above when doctor gives a new script.</div>";
      page += "</div><br>";
    } else {
      page += "<table style='width:100%;border-collapse:collapse'>";
      page += "<tr><th style='background:#e07b00;color:#fff;padding:8px'>Dose</th>";
      page += "<th style='background:#c05a00;color:#fff;padding:8px'>Time</th>";
      page += "<th style='background:#e07b00;color:#fff;padding:8px'>Status</th></tr>";
      for (uint8_t i = 0; i < g_med_doses; i++) {
        char tbuf[8];
        snprintf(tbuf, sizeof(tbuf), "%02d:%02d", g_med[i].hh, g_med[i].mm);
        bool overdue = !g_med[i].taken &&
                       (g_hh > g_med[i].hh ||
                       (g_hh == g_med[i].hh && g_mm > g_med[i].mm));
        String status_col = g_med[i].taken ? "#0f0" : (overdue ? "#f00" : "#888");
        String status_txt = g_med[i].taken ? "GENEEM / TAKEN" :
                            (overdue ? "VERGEET! / MISSED!" : "Upcoming");
        page += "<tr><td style='padding:8px;background:#1a1a1a;font-weight:bold'>";
        page += String(dose_names[i]) + "</td>";
        page += "<td style='padding:8px;background:#1a1a1a'>" + String(tbuf) + "</td>";
        page += "<td style='padding:8px;background:#1a1a1a;color:" + status_col + ";font-weight:bold'>" + status_txt + "</td></tr>";
      }
      page += "</table><br>";
      // Confirm button for due dose
      uint8_t due = med_due_dose();
      if (due != 255 && !g_med[due].taken) {
        char tbuf[32];
        snprintf(tbuf, sizeof(tbuf), "Dose %d (%02d:%02d) geneem!", due+1, g_med[due].hh, g_med[due].mm);
        page += "<form method='POST' action='/med_confirm'>";
        page += "<input type='submit' value='" + String(tbuf) + "' ";
        page += "style='background:#1a7a1a;font-size:1rem;padding:12px'></form><br>";
      }
    }

    // Schedule settings
    page += "<h3 style='color:#0af'>Set Medicine Schedule</h3>";
    page += "<form method='POST' action='/med_save'>";
    page += "<label>Doses per day (0=no script):<br>";
    page += "<select name='doses' style='width:100%;padding:8px;margin:6px 0 12px;background:#222;color:#eee;border:1px solid #555;border-radius:4px'>";
    for (uint8_t i = 0; i <= MED_MAX_DOSES; i++) {
      page += "<option value='" + String(i) + "'" + String(g_med_doses==i?" selected":"") + ">" + String(i) + "x per day</option>";
    }
    page += "</select></label>";
    for (uint8_t i = 0; i < MED_MAX_DOSES; i++) {
      // Label says "Dose N time" only — no longer hardcoded to a time-of-day
      // name (was misleadingly always e.g. "Morning" for Dose 1, even though
      // the hour value below it has always been fully editable 07-19).
      page += "<label>Dose " + String(i+1) + " time:<br>";
      page += "<input name='hh" + String(i) + "' type='number' min='7' max='19' value='" + String(g_med[i].hh) + "' style='width:48%;display:inline-block'> : ";
      page += "<input name='mm" + String(i) + "' type='number' min='0' max='59' value='" + String(g_med[i].mm) + "' style='width:44%;display:inline-block'></label>";
    }
    page += "<br><input type='submit' value='Save medicine schedule'></form>";

    // ── Ekstra Medisyne Vandag (ongeskeduleer) ─────────────
    // A SEPARATE tool from the Kort Kursus scheduler below: for the everyday
    // "she needs an extra painkiller right now" case, with no time to set and
    // no multi-day course to configure. One tap logs it, for today's record.
    page += "<h3 style='color:#f66'>&#128138; Ekstra Medisyne Vandag / Extra Medicine Today (ongeskeduleer / unscheduled)</h3>";
    if (g_adhoc_count == 0) {
      page += "<div class='stat'><div class='val' style='color:#888'>Geen ekstra dosis vandag geneem nie</div></div>";
    } else {
      page += "<div class='stat'><div class='label'>Vandag geneem</div><div class='val' style='color:#0f0'>";
      for (uint8_t i = 0; i < g_adhoc_count; i++) {
        char hhmm[6]; snprintf(hhmm, sizeof(hhmm), "%02u:%02u", g_adhoc_hh[i], g_adhoc_mm[i]);
        page += String(hhmm) + (i + 1 < g_adhoc_count ? ", " : "");
      }
      page += "</div></div>";
    }
    if (g_adhoc_count < ADHOC_MAX_LOG) {
      page += "<form method='POST' action='/adhoc_log'>";
      page += "<input type='submit' value='Log Ekstra Dosis NOU / Log Extra Dose NOW' ";
      page += "style='background:#1a7a1a;font-size:1rem;padding:12px'></form>";
    } else {
      page += "<p style='color:#f90'>" + String(ADHOC_MAX_LOG) + " ekstra dosisse vandag alreeds gelog &mdash; kontak die dokter as meer nodig is.</p>";
    }
    page += "<p style='color:#888;font-size:0.8em'>Vir 'n herhalende kursus (bv. antibiotika 3x/dag), gebruik Kort Kursus hieronder eerder.</p>";

    // ── Kort Kursus / Short Course section ─────────────────
    // TWO independent slots — Ria's real case: two different antibiotics
    // for pneumonia, running concurrently on different schedules. Each slot has
    // its own name, dose count/times, day countdown, and auto-finish.
    page += "<h3 style='color:#f66'>&#9888; Kort Kursus (bv. Antibiotika) / Short Course (e.g. Antibiotics)</h3>";

    bool any_slot_active = false;
    for (uint8_t s = 0; s < COURSE_SLOTS; s++) {
      Course& c = g_course[s];
      if (c.doses == 0 || c.days_left == 0) continue;
      any_slot_active = true;
      uint8_t day_num = c.days_total - c.days_left + 1;
      uint8_t taken_count = 0;
      for (uint8_t i = 0; i < c.doses; i++) if (c.taken[i]) taken_count++;
      String col = (taken_count == c.doses) ? "#0f0" : "#f00";
      page += "<div class='stat' style='border:2px solid " + col + ";margin-bottom:6px'>";
      page += "<div class='label'>Gleuf " + String(s + 1) + " &mdash; " + String(c.name) + " &mdash; Dag " + String(day_num) + " van " + String(c.days_total) + "</div>";
      page += "<div class='val' style='color:" + col + "'>" + String(taken_count) + " of " + String(c.doses) + " doses vandag geneem</div></div>";
      page += "<table style='width:100%;border-collapse:collapse;font-size:0.85rem;margin:4px 0 8px'>";
      page += "<tr><th style='background:#e07b00;color:#fff;padding:5px'>Dosis</th><th style='background:#c05a00;color:#fff;padding:5px'>Tyd</th><th style='background:#e07b00;color:#fff;padding:5px'>Status</th></tr>";
      for (uint8_t i = 0; i < c.doses; i++) {
        char hhmm[6]; snprintf(hhmm, sizeof(hhmm), "%02u:%02u", c.hh[i], c.mm[i]);
        String st = c.taken[i] ? "<span style='color:#0f0'>GENEEM</span>" : "<span style='color:#f00'>NIE GENEEM</span>";
        page += "<tr><td style='padding:5px;background:#1a1a1a'>" + String(i + 1) + "</td>";
        page += "<td style='padding:5px;background:#1a1a1a;text-align:center'>" + String(hhmm) + "</td>";
        page += "<td style='padding:5px;background:#1a1a1a'>" + st + "</td></tr>";
      }
      page += "</table>";
      uint8_t cdue = course_due_dose(s);
      if (cdue != 255) {
        page += "<form method='POST' action='/course_confirm'><input type='hidden' name='s' value='" + String(s) + "'>";
        page += "<input type='submit' value='Gleuf " + String(s + 1) + " — DOSIS GENEEM! / DOSE TAKEN!' ";
        page += "style='background:#1a7a1a;font-size:0.95rem;padding:10px'></form><br>";
      }
      page += "<form method='POST' action='/course_stop'><input type='hidden' name='s' value='" + String(s) + "'>";
      page += "<input type='submit' value='Stop Gleuf " + String(s + 1) + " vroeg / Stop early' style='background:#801515'></form><br>";
    }
    if (!any_slot_active) {
      page += "<div class='stat'><div class='label'>LED 7 op medisyne-strook</div>";
      page += "<div class='val' style='color:#555'>Geen kort kursus tans aktief</div></div>";
    }

    // ── Start a new course in a free slot ─────────────────────────────────────
    // Find first free slot to pre-select in the form
    int8_t free_slot = -1;
    for (uint8_t s = 0; s < COURSE_SLOTS; s++) {
      if (g_course[s].doses == 0 || g_course[s].days_left == 0) { free_slot = s; break; }
    }
    if (free_slot >= 0) {
      Course& fs = g_course[free_slot];
      page += "<h4 style='color:#ccc;margin-top:16px'>Begin 'n nuwe kursus in Gleuf " + String(free_slot + 1) + " / Start new course in Slot " + String(free_slot + 1) + "</h4>";
      page += "<form method='POST' action='/course_start'>";
      page += "<input type='hidden' name='s' value='" + String(free_slot) + "'>";
      page += "Naam / Name (opsioneel): <input name='name' type='text' maxlength='23' placeholder='bv. Antibiotika' style='width:180px'><br><br>";
      page += "Dosisse per dag / Doses per day: <select name='doses'>";
      uint8_t sel_doses = (fs.doses > 0) ? fs.doses : 1;
      for (uint8_t n = 1; n <= COURSE_MAX_DOSES; n++)
        page += "<option value='" + String(n) + "'" + (n == sel_doses ? " selected" : "") + ">" + String(n) + "x</option>";
      page += "</select><br><br>";
      for (uint8_t i = 0; i < COURSE_MAX_DOSES; i++) {
        page += "Dosis " + String(i + 1) + " tyd: ";
        page += "<input name='hh" + String(i) + "' type='number' min='0' max='23' value='" + String(fs.hh[i]) + "' style='width:44%;display:inline-block'> : ";
        page += "<input name='mm" + String(i) + "' type='number' min='0' max='59' value='" + String(fs.mm[i]) + "' style='width:44%;display:inline-block'><br><br>";
      }
      uint16_t sel_days = (fs.days_total > 0) ? fs.days_total : 4;
      page += "Totale dae / Total days: <input name='days' type='number' min='1' max='250' value='" + String(sel_days) + "' style='width:60px'><br><br>";
      page += "<input type='submit' value='Begin Kursus / Start Course' style='background:#1a7a1a'></form>";
      page += "<p style='color:#888;font-size:0.8em'>Die klok tel outomaties af en stop die kursus self na die laaste dag &mdash; niemand hoef te onthou om dit af te skakel nie.</p>";
    } else {
      page += "<p style='color:#ffa500'>Albei gleufies tans aktief. Stop eers een voordat jy 'n nuwe kursus begin. / Both slots currently active. Stop one first.</p>";
    }

    page += html_footer();
    webServer.send(200, "text/html", page);
  });

  webServer.on("/med_confirm", HTTP_POST, []() {
    med_confirm();
    webServer.sendHeader("Location", "/medicine");
    webServer.send(302);
  });

  // Log an unscheduled extra dose right now — 
  webServer.on("/adhoc_log", HTTP_POST, []() {
    adhoc_log();
    webServer.sendHeader("Location", "/medicine");
    webServer.send(302);
  });

  // Start a new Kort Kursus in the slot passed as hidden field 's'
  webServer.on("/course_start", HTTP_POST, []() {
    int s = webServer.arg("s").toInt();
    if (s < 0 || s >= COURSE_SLOTS) s = 0;
    String n = webServer.arg("name"); n.trim();
    if (n.length() == 0) n = "Ekstra Kursus";
    int doses = webServer.arg("doses").toInt();
    int days  = webServer.arg("days").toInt();
    if (doses < 1) doses = 1;
    if (doses > COURSE_MAX_DOSES) doses = COURSE_MAX_DOSES;
    if (days  < 1) days  = 1;
    if (days  > 250) days = 250;
    Course& c = g_course[s];
    strncpy(c.name, n.c_str(), sizeof(c.name) - 1);
    c.name[sizeof(c.name) - 1] = '\0';
    c.doses = (uint8_t)doses;
    for (uint8_t i = 0; i < COURSE_MAX_DOSES; i++) {
      int hh = webServer.arg("hh" + String(i)).toInt();
      int mm = webServer.arg("mm" + String(i)).toInt();
      if (hh >= 0 && hh < 24) c.hh[i] = (uint8_t)hh;
      if (mm >= 0 && mm < 60) c.mm[i] = (uint8_t)mm;
      c.taken[i] = false;
    }
    c.days_total = (uint8_t)days;
    c.days_left  = (uint8_t)days;
    c.last_day   = g_day;
    med_save();
    po_notify("Kort Kursus begin / Course started (Gleuf " + String(s + 1) + ")",
      String(c.name) + " — " + String(doses) + "x/dag vir " + String(days) + " dae.\nTyd: " + po_time(), 0);
    Serial.println("Kort Kursus slot " + String(s) + " started: " + String(c.name));
    webServer.sendHeader("Location", "/medicine");
    webServer.send(302);
  });

  // Confirm a course dose taken — slot passed as hidden field 's'
  webServer.on("/course_confirm", HTTP_POST, []() {
    int s = webServer.arg("s").toInt();
    if (s < 0 || s >= COURSE_SLOTS) s = 0;
    uint8_t cdue = course_due_dose((uint8_t)s);
    if (cdue != 255) {
      g_course[s].taken[cdue] = true;
      if (g_course_alm_slot == s) { g_course_alm_slot = 255; g_course_alm_dose = 255; }
      med_save();
      dfplayer_volume(is_night_now() ? g_night_vol : g_vol);
      dfplayer_play(MED_TRACK); voice_lock();
    }
    webServer.sendHeader("Location", "/medicine");
    webServer.send(302);
  });

  // Stop a course slot early — slot passed as hidden field 's'
  webServer.on("/course_stop", HTTP_POST, []() {
    int s = webServer.arg("s").toInt();
    if (s < 0 || s >= COURSE_SLOTS) s = 0;
    g_course[s].doses = 0;
    g_course[s].days_left = 0;
    g_course[s].name[0] = '\0';
    med_save();
    webServer.sendHeader("Location", "/medicine");
    webServer.send(302);
  });

  webServer.on("/med_save", HTTP_POST, []() {
    uint8_t d = (uint8_t)webServer.arg("doses").toInt();
    if (d <= MED_MAX_DOSES) g_med_doses = d;
    for (uint8_t i = 0; i < MED_MAX_DOSES; i++) {
      int hh = webServer.arg("hh" + String(i)).toInt();
      int mm = webServer.arg("mm" + String(i)).toInt();
      if (hh >= MED_START_HH && hh <= MED_END_HH) g_med[i].hh = (uint8_t)hh;
      if (mm >= 0 && mm < 60)                      g_med[i].mm = (uint8_t)mm;
    }
    med_save();
    webServer.sendHeader("Location", "/medicine");
    webServer.send(302);
  });

  // -- /ota : OTA upload page ───────────────────────────────────────────────────
  webServer.on("/ota", HTTP_GET, []() {
    String page = html_header("OTA Update");
    page += "<p>You can also use Arduino IDE: <b>Sketch &rarr; Upload</b> and select the <b>OumaRiaClock</b> network port.</p>";
    page += "<form method='POST' action='/ota_upload' enctype='multipart/form-data'>";
    page += "<input type='file' name='firmware' accept='.bin'><br><br>";
    page += "<input type='submit' value='Upload firmware'></form>";
    page += html_footer();
    webServer.send(200, "text/html", page);
  });

  webServer.on("/ota_upload", HTTP_POST,
    []() { webServer.send(200, "text/html", "<p>Done. Rebooting...</p>"); delay(1000); ESP.restart(); },
    []() {
      HTTPUpload& up = webServer.upload();
      if(up.status == UPLOAD_FILE_START) {
        Serial.printf("OTA upload: %s\n", up.filename.c_str());
        Update.begin(UPDATE_SIZE_UNKNOWN);
      } else if(up.status == UPLOAD_FILE_WRITE) {
        Update.write(up.buf, up.currentSize);
      } else if(up.status == UPLOAD_FILE_END) {
        Update.end(true);
      }
    }
  );

  // ── Egg Timer web routes ───────────────────────────────────────────────────
  webServer.on("/egg", HTTP_GET, []() {
    String page = html_header("Egg Timer");
    page += "<h2 style='color:#FF8C00'>&#9201; Egg Timer</h2>";

    // Status card
    page += "<div style='background:#1a1a1a;border:1px solid #333;border-radius:8px;padding:16px;margin-bottom:12px'>";
    if(g_egg_run) {
      // Timer is running — show live countdown
      uint32_t rem = (uint32_t)g_egg_mm * 60 + g_egg_ss;
      char buf[8];
      snprintf(buf, sizeof(buf), "%02d:%02d", g_egg_mm, g_egg_ss);
      page += "<p style='font-size:3em;font-weight:bold;color:#FF8C00;text-align:center;margin:0;font-family:monospace'>";
      page += String(buf) + "</p>";
      page += "<p style='text-align:center;color:#aaa;margin:4px 0 12px'>Counting down...</p>";
      // Pause and Stop buttons
      page += "<div style='display:flex;gap:12px;justify-content:center'>";
      page += "<form method='POST' action='/egg_pause'>"
              "<input type='submit' value='Pause' style='background:#888;color:#fff;border:none;"
              "padding:10px 24px;border-radius:6px;font-size:1em;cursor:pointer;font-weight:bold'></form>";
      page += "<form method='POST' action='/egg_stop'>"
              "<input type='submit' value='Stop &amp; Reset' style='background:#CC2200;color:#fff;border:none;"
              "padding:10px 24px;border-radius:6px;font-size:1em;cursor:pointer;font-weight:bold'></form>";
      page += "</div>";
      // Auto-refresh every 5 seconds while running
      page += "<script>setTimeout(()=>location.reload(),5000)</script>";
    } else if(g_mode == 2 && !g_egg_run && (g_egg_mm > 0 || g_egg_ss > 0)) {
      // Paused mid-count
      char buf[8];
      snprintf(buf, sizeof(buf), "%02d:%02d", g_egg_mm, g_egg_ss);
      page += "<p style='font-size:3em;font-weight:bold;color:#888;text-align:center;margin:0;font-family:monospace'>";
      page += String(buf) + "</p>";
      page += "<p style='text-align:center;color:#aaa;margin:4px 0 12px'>Paused</p>";
      page += "<div style='display:flex;gap:12px;justify-content:center'>";
      page += "<form method='POST' action='/egg_resume'>"
              "<input type='submit' value='Resume' style='background:#FF8C00;color:#fff;border:none;"
              "padding:10px 24px;border-radius:6px;font-size:1em;cursor:pointer;font-weight:bold'></form>";
      page += "<form method='POST' action='/egg_stop'>"
              "<input type='submit' value='Stop &amp; Reset' style='background:#CC2200;color:#fff;border:none;"
              "padding:10px 24px;border-radius:6px;font-size:1em;cursor:pointer;font-weight:bold'></form>";
      page += "</div>";
    } else {
      // Idle — show set time and Start button
      char buf[8];
      snprintf(buf, sizeof(buf), "%02d:%02d", g_egg_set_mm, g_egg_set_ss);
      page += "<p style='font-size:3em;font-weight:bold;color:#444;text-align:center;margin:0;font-family:monospace'>";
      page += String(buf) + "</p>";
      page += "<p style='text-align:center;color:#aaa;margin:4px 0 12px'>Ready to start</p>";
      page += "<form method='POST' action='/egg_start' style='text-align:center'>"
              "<input type='submit' value='&#9654; Start' style='background:#FF8C00;color:#fff;border:none;"
              "padding:12px 32px;border-radius:6px;font-size:1.1em;cursor:pointer;font-weight:bold'></form>";
    }
    page += "</div>";

    // Set time form
    page += "<div style='background:#1a1a1a;border:1px solid #333;border-radius:8px;padding:16px'>";
    page += "<h3 style='color:#FF8C00;margin-top:0'>Set Time</h3>";
    page += "<form method='POST' action='/egg_set'>";
    page += "<table style='width:100%'><tr>";
    page += "<td style='padding:4px'><label style='color:#aaa'>Minutes</label><br>"
            "<input type='number' name='emm' min='0' max='99' value='" + String(g_egg_set_mm) + "' "
            "style='width:70px;background:#222;color:#fff;border:1px solid #444;border-radius:4px;"
            "padding:8px;font-size:1.2em;text-align:center'></td>";
    page += "<td style='padding:4px;font-size:2em;color:#aaa;vertical-align:bottom;padding-bottom:8px'>:</td>";
    page += "<td style='padding:4px'><label style='color:#aaa'>Seconds</label><br>"
            "<input type='number' name='ess' min='0' max='59' value='" + String(g_egg_set_ss) + "' "
            "style='width:70px;background:#222;color:#fff;border:1px solid #444;border-radius:4px;"
            "padding:8px;font-size:1.2em;text-align:center'></td>";
    page += "<td style='padding:4px;vertical-align:bottom'>"
            "<input type='submit' value='Set &amp; Start' style='background:#FF8C00;color:#fff;border:none;"
            "padding:10px 18px;border-radius:6px;font-size:1em;cursor:pointer;font-weight:bold'></td>";
    page += "</tr></table>";
    page += "</form>";
    page += "<p style='color:#888;font-size:0.85em;margin:8px 0 0'>"
            "Tip: you can also set and start the timer using the physical buttons on the clock.</p>";
    page += "</div>";

    // Quick presets
    page += "<div style='background:#1a1a1a;border:1px solid #333;border-radius:8px;padding:16px;margin-top:12px'>";
    page += "<h3 style='color:#FF8C00;margin-top:0'>Quick Presets</h3>";
    page += "<div style='display:flex;flex-wrap:wrap;gap:8px'>";
    // Preset buttons — each is a small form POST
    const char* presets[][3] = {
      {"3:00","3","0"},{"5:00","5","0"},{"7:00","7","0"},
      {"10:00","10","0"},{"15:00","15","0"},{"20:00","20","0"},
      {"30:00","30","0"},{"45:00","45","0"},
    };
    for(auto& p : presets) {
      page += "<form method='POST' action='/egg_set'>"
              "<input type='hidden' name='emm' value='" + String(p[1]) + "'>"
              "<input type='hidden' name='ess' value='" + String(p[2]) + "'>"
              "<input type='submit' value='" + String(p[0]) + "' "
              "style='background:#333;color:#FF8C00;border:1px solid #555;padding:8px 14px;"
              "border-radius:6px;font-size:0.95em;cursor:pointer;font-weight:bold'>"
              "</form>";
    }
    page += "</div></div>";
    page += html_footer();
    webServer.send(200, "text/html", page);
  });

  // Set time + start
  webServer.on("/egg_set", HTTP_POST, []() {
    uint8_t mm = (uint8_t)webServer.arg("emm").toInt();
    uint8_t ss = (uint8_t)webServer.arg("ess").toInt();
    if(mm > 99) mm = 99;
    if(ss > 59) ss = 59;
    g_egg_set_mm = mm;
    g_egg_set_ss = ss;
    g_egg_mm     = mm;
    g_egg_ss     = ss;
    g_egg_run    = true;
    g_mode       = 2;  // switch display to egg run mode
    settings_save();
    webServer.sendHeader("Location", "/egg");
    webServer.send(303);
  });

  // Start (using existing set time)
  webServer.on("/egg_start", HTTP_POST, []() {
    g_egg_mm  = g_egg_set_mm;
    g_egg_ss  = g_egg_set_ss;
    g_egg_run = true;
    g_mode    = 2;
    webServer.sendHeader("Location", "/egg");
    webServer.send(303);
  });

  // Pause
  webServer.on("/egg_pause", HTTP_POST, []() {
    g_egg_run = false;
    webServer.sendHeader("Location", "/egg");
    webServer.send(303);
  });

  // Resume
  webServer.on("/egg_resume", HTTP_POST, []() {
    g_egg_run = true;
    webServer.sendHeader("Location", "/egg");
    webServer.send(303);
  });

  // Stop and reset to set time
  webServer.on("/egg_stop", HTTP_POST, []() {
    g_egg_run = false;
    g_egg_mm  = g_egg_set_mm;
    g_egg_ss  = g_egg_set_ss;
    g_mode    = 0;  // back to clock mode
    webServer.sendHeader("Location", "/egg");
    webServer.send(303);
  });

  // ── Smoker Uptimer web routes ────────────────────────────────────────────
  // Counts UP — for tracking how long meat has been in the smoker. Survives
  // power loss because it's based on real RTC time, not millis().
  webServer.on("/smoker", HTTP_GET, []() {
    String page = html_header("Smoker Uptimer");
    page += "<h2 style='color:#FF8C00'>&#128293; Smoker Uptimer</h2>";

    uint32_t sec = smoke_elapsed_sec();
    uint8_t  eh  = (uint8_t)(sec / 3600);
    uint8_t  em  = (uint8_t)((sec % 3600) / 60);
    uint8_t  es  = (uint8_t)(sec % 60);
    char buf[12];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", eh, em, es);

    page += "<div style='background:#1a1a1a;border:1px solid #333;border-radius:8px;padding:16px;margin-bottom:12px'>";
    page += "<p style='font-size:3em;font-weight:bold;color:#FF8C00;text-align:center;margin:0;font-family:monospace'>";
    page += String(buf) + "</p>";

    if(g_smoke_running && !g_smoke_paused) {
      page += "<p style='text-align:center;color:#aaa;margin:4px 0 12px'>Running...</p>";
      if(g_smoke_note.length()) page += "<p style='text-align:center;color:#888;margin:0 0 12px'>" + g_smoke_note + "</p>";
      page += "<div style='display:flex;gap:12px;justify-content:center'>";
      page += "<form method='POST' action='/smoker_pause'>"
              "<input type='submit' value='Pause' style='background:#888;color:#fff;border:none;"
              "padding:10px 24px;border-radius:6px;font-size:1em;cursor:pointer;font-weight:bold'></form>";
      page += "<form method='POST' action='/smoker_stop'>"
              "<input type='submit' value='Stop &amp; Reset' style='background:#CC2200;color:#fff;border:none;"
              "padding:10px 24px;border-radius:6px;font-size:1em;cursor:pointer;font-weight:bold'></form>";
      page += "</div>";
      page += "<script>setTimeout(()=>location.reload(),5000)</script>";
    } else if(g_smoke_running && g_smoke_paused) {
      page += "<p style='text-align:center;color:#aaa;margin:4px 0 12px'>Paused</p>";
      if(g_smoke_note.length()) page += "<p style='text-align:center;color:#888;margin:0 0 12px'>" + g_smoke_note + "</p>";
      page += "<div style='display:flex;gap:12px;justify-content:center'>";
      page += "<form method='POST' action='/smoker_resume'>"
              "<input type='submit' value='Resume' style='background:#FF8C00;color:#fff;border:none;"
              "padding:10px 24px;border-radius:6px;font-size:1em;cursor:pointer;font-weight:bold'></form>";
      page += "<form method='POST' action='/smoker_stop'>"
              "<input type='submit' value='Stop &amp; Reset' style='background:#CC2200;color:#fff;border:none;"
              "padding:10px 24px;border-radius:6px;font-size:1em;cursor:pointer;font-weight:bold'></form>";
      page += "</div>";
    } else {
      page += "<p style='text-align:center;color:#aaa;margin:4px 0 12px'>Not running</p>";
      page += "<form method='POST' action='/smoker_start' style='text-align:center'>";
      page += "<input name='note' type='text' placeholder='e.g. 3kg pork leg' value='' "
              "style='background:#222;color:#fff;border:1px solid #444;border-radius:4px;"
              "padding:8px;font-size:1em;text-align:center;margin-bottom:10px;width:80%'><br>";
      page += "<input type='submit' value='&#9654; Start' style='background:#FF8C00;color:#fff;border:none;"
              "padding:12px 32px;border-radius:6px;font-size:1.1em;cursor:pointer;font-weight:bold'></form>";
    }
    page += "</div>";
    page += "<p style='color:#888;font-size:0.85em'>Keeps counting even through a power cut — "
            "it uses the clock's real time, not how long it's been switched on.</p>";
    page += html_footer();
    webServer.send(200, "text/html", page);
  });

  webServer.on("/smoker_start", HTTP_POST, []() {
    if(!g_smoke_running) {   // guard: don't let a stray/duplicate request reset an active session
      g_smoke_note          = webServer.arg("note");
      g_smoke_start_unix    = rtc.now().unixtime();
      g_smoke_pause_unix    = 0;
      g_smoke_paused_accum  = 0;
      g_smoke_paused        = false;
      g_smoke_running       = true;
      g_smoke_last_reminder = 0;
      settings_save();
    }
    webServer.sendHeader("Location", "/smoker");
    webServer.send(303);
  });

  webServer.on("/smoker_pause", HTTP_POST, []() {
    if(g_smoke_running && !g_smoke_paused) {
      g_smoke_pause_unix = rtc.now().unixtime();
      g_smoke_paused     = true;
      settings_save();
    }
    webServer.sendHeader("Location", "/smoker");
    webServer.send(303);
  });

  webServer.on("/smoker_resume", HTTP_POST, []() {
    if(g_smoke_running && g_smoke_paused) {
      uint32_t now_unix = rtc.now().unixtime();
      if(now_unix > g_smoke_pause_unix) g_smoke_paused_accum += (now_unix - g_smoke_pause_unix);
      g_smoke_paused = false;
      settings_save();
    }
    webServer.sendHeader("Location", "/smoker");
    webServer.send(303);
  });

  webServer.on("/smoker_stop", HTTP_POST, []() {
    g_smoke_running      = false;
    g_smoke_paused       = false;
    g_smoke_start_unix   = 0;
    g_smoke_pause_unix   = 0;
    g_smoke_paused_accum = 0;
    g_smoke_note         = "";
    g_smoke_last_reminder = 0;
    settings_save();
    webServer.sendHeader("Location", "/smoker");
    webServer.send(303);
  });

  // ── Braai Timer web routes (Ouma's idea!) ──────────────────────────────────
  // Counts DOWN from a selected total time, with a repeating turn-reminder at
  // a selected interval. Tickbox to enable/disable so it stays out of the way
  // when not braaiing. Lives at /braai, same RTC-survives-reboot pattern as
  // the Smoker Uptimer above.
  webServer.on("/braai", HTTP_GET, []() {
    String page = html_header("Braai Timer");
    page += "<h2 style='color:#FF8C00'>&#129385; Braai Timer</h2>";

    // Enable/disable tickbox
    page += "<form method='POST' action='/braai_enable'>";
    page += "<label style='font-size:1.05rem'><input name='en' type='checkbox'" +
            String(g_braai_enabled ? " checked" : "") +
            " value='1' onchange='this.form.submit()'> Braai Timer aktief / Active</label>";
    page += "</form><br>";

    if(!g_braai_enabled) {
      page += "<div style='background:#1a1a1a;border:1px solid #333;border-radius:8px;padding:16px'>";
      page += "<p style='color:#888;text-align:center;margin:0'>Timer is off. Tick the box above to use it.</p>";
      page += "</div>";
      page += html_footer();
      webServer.send(200, "text/html", page);
      return;
    }

    uint32_t rem = braai_remaining_sec();
    uint8_t  rm  = (uint8_t)(rem / 60);
    uint8_t  rs  = (uint8_t)(rem % 60);
    char buf[8];
    snprintf(buf, sizeof(buf), "%02d:%02d", rm, rs);

    page += "<div style='background:#1a1a1a;border:1px solid #333;border-radius:8px;padding:16px;margin-bottom:12px'>";
    if(g_braai_running && !g_braai_paused) {
      page += "<p style='font-size:3em;font-weight:bold;color:#FF8C00;text-align:center;margin:0;font-family:monospace'>" + String(buf) + "</p>";
      page += "<p style='text-align:center;color:#aaa;margin:4px 0 12px'>Counting down... turn every " +
              String(g_braai_turn_sec < 60 ? String(g_braai_turn_sec) + "s" : String(g_braai_turn_sec / 60.0, 1) + " min") + "</p>";
      page += "<div style='display:flex;gap:12px;justify-content:center'>";
      page += "<form method='POST' action='/braai_pause'><input type='submit' value='Pause' "
              "style='background:#888;color:#fff;border:none;padding:10px 24px;border-radius:6px;font-size:1em;cursor:pointer;font-weight:bold'></form>";
      page += "<form method='POST' action='/braai_stop'><input type='submit' value='Stop &amp; Reset' "
              "style='background:#CC2200;color:#fff;border:none;padding:10px 24px;border-radius:6px;font-size:1em;cursor:pointer;font-weight:bold'></form>";
      page += "</div>";
      page += "<script>setTimeout(()=>location.reload(),5000)</script>";
    } else if(g_braai_running && g_braai_paused) {
      page += "<p style='font-size:3em;font-weight:bold;color:#888;text-align:center;margin:0;font-family:monospace'>" + String(buf) + "</p>";
      page += "<p style='text-align:center;color:#aaa;margin:4px 0 12px'>Paused</p>";
      page += "<div style='display:flex;gap:12px;justify-content:center'>";
      page += "<form method='POST' action='/braai_resume'><input type='submit' value='Resume' "
              "style='background:#FF8C00;color:#fff;border:none;padding:10px 24px;border-radius:6px;font-size:1em;cursor:pointer;font-weight:bold'></form>";
      page += "<form method='POST' action='/braai_stop'><input type='submit' value='Stop &amp; Reset' "
              "style='background:#CC2200;color:#fff;border:none;padding:10px 24px;border-radius:6px;font-size:1em;cursor:pointer;font-weight:bold'></form>";
      page += "</div>";
    } else {
      page += "<p style='text-align:center;color:#aaa;margin:4px 0 12px'>Choose your time and turn interval, then Start.</p>";

      page += "<form method='POST' action='/braai_start'>";
      page += "<label style='color:#aaa'>Total cook time:<br>";
      page += "<select name='total' style='width:100%;padding:10px;margin:6px 0 14px;background:#222;color:#fff;border:1px solid #555;border-radius:4px;font-size:1.05rem'>";
      const uint16_t totals[] = {300,420,540,600,720,840,960,1080,1200};
      for(uint16_t t : totals) {
        page += "<option value='" + String(t) + "'" + String(t==g_braai_total_sec?" selected":"") + ">" +
                String(t/60) + " min</option>";
      }
      page += "</select></label>";

      page += "<label style='color:#aaa'>Turn every:<br>";
      page += "<select name='turn' style='width:100%;padding:10px;margin:6px 0 14px;background:#222;color:#fff;border:1px solid #555;border-radius:4px;font-size:1.05rem'>";
      const uint16_t turns[] = {30,45,60,90,120,180};
      const char* turn_labels[] = {"30 sec","45 sec","1 min","1.5 min","2 min","3 min"};
      for(uint8_t i = 0; i < 6; i++) {
        page += "<option value='" + String(turns[i]) + "'" + String(turns[i]==g_braai_turn_sec?" selected":"") + ">" +
                String(turn_labels[i]) + "</option>";
      }
      page += "</select></label>";

      page += "<input type='submit' value='&#9654; Start' style='background:#FF8C00;color:#fff;border:none;"
              "padding:12px 32px;border-radius:6px;font-size:1.1em;cursor:pointer;font-weight:bold;width:100%'>";
      page += "</form>";
    }
    page += "</div>";
    page += "<p style='color:#888;font-size:0.85em'>Beep every turn interval to flip the meat. "
            "A different sound plays when the total time is up. Keeps counting even through a power cut.</p>";
    page += "<p style='text-align:center;margin-top:14px'><a href='/braaitimes' style='display:inline-block;background:#FF8C00;color:#000;"
            "padding:10px 24px;border-radius:6px;font-weight:bold;text-decoration:none'>&#128203; Braai Times Reference</a></p>";
    page += html_footer();
    webServer.send(200, "text/html", page);
  });

  // ── /bar3data : JSON snapshot of all 30 BAR3 LEDs + 5 subsystem states ───
  // NEW 2026-06-29, Henry's request — Ouma can't always see the physical
  // 3x10 status bar clearly from 4m away (low vision), so this feeds a
  // phone-page mirror (/bar3) that shows the SAME colours live, plus a
  // plain-language "what's active right now" summary so neither of them
  // has to walk over to the clock or click through every separate page
  // (Wekkers/Alarm/Pills/Egg/Smoker) to see what wants attention.
  // Reads directly from bar3_leds[] (the real physical buffer) so this can
  // never drift out of sync with what's actually lit on the clock — no
  // separate colour logic duplicated here.
  webServer.on("/bar3data", HTTP_GET, []() {
    compose_bar3();   // make sure bar3_leds[] reflects the latest state before reading it
    String j = "{\"leds\":[";
    for(uint8_t i = 0; i < BAR3_NUM_LEDS; i++) {
      char hex[10];
      snprintf(hex, sizeof(hex), "\"#%02X%02X%02X\"", bar3_leds[i].r, bar3_leds[i].g, bar3_leds[i].b);
      j += hex;
      if(i < BAR3_NUM_LEDS - 1) j += ",";
    }
    j += "],";

    // ── Wekkers: active if currently ringing ──────────────────────────────
    j += "\"wekker_active\":" + String(g_wekker_ringing ? "true" : "false") + ",";

    // ── Security alarm: active if anything beyond plain armed/disarmed ────
    // FIX (2026-07-01): g_alm_fire removed from this flag. It used to be OR'd
    // in here, which meant the wake-up Alarm/Wekkers sounding through the
    // speaker made the SECURITY tile show "ACTIVE!" too — easily mistaken for
    // the piezo siren going off. Security tile now reflects only genuine
    // security events (panic/PIR triggered/entry/exit delay).
    bool alarm_active = g_panic_active || g_sec_triggered || g_sec_entry_delay ||
                         g_sec_exit_delay;
    j += "\"alarm_active\":" + String(alarm_active ? "true" : "false") + ",";
    j += "\"alarm_armed\":"  + String(g_z2_armed     ? "true" : "false") + ",";

    // ── Wake Alarm: the original hh:mm Alarm sounding through the speaker.
    // Separate tile (NEW 2026-07-01) so it's never confused with the
    // security siren. Wekkers has its own tile already, so this one is
    // suppressed while Wekkers is ringing to avoid a double-notification.
    bool wake_active = g_alm_fire && !g_wekker_ringing;
    j += "\"wake_active\":" + String(wake_active ? "true" : "false") + ",";

    // ── Pills: active if today's dose not yet taken (and pills are due today) ─
    bool pills_active = g_pills_active && !g_pill_taken && !g_pill_alm_fire;
    bool pills_firing  = g_pill_alm_fire;
    j += "\"pills_active\":" + String((pills_active || pills_firing) ? "true" : "false") + ",";

    // ── Egg timer ───────────────────────────────────────────────────────────
    j += "\"egg_active\":" + String(g_egg_run ? "true" : "false") + ",";
    j += "\"egg_mm\":" + String(g_egg_mm) + ",\"egg_ss\":" + String(g_egg_ss) + ",";

    // ── Smoker uptimer ──────────────────────────────────────────────────────
    j += "\"smoker_active\":" + String((g_smoke_running && !g_smoke_paused) ? "true" : "false") + ",";
    j += "\"smoker_paused\":" + String((g_smoke_running && g_smoke_paused)  ? "true" : "false") + ",";

    // ── Internet reachability feeds the mirror row ────
    j += "\"inet_ok\":" + String(g_inet_ok ? "true" : "false") + ",";

    // ── Birthday / Doctor countdown — NEW 2026-06-29, Henry's request ────────
    // The physical panel already shows these (purple flash = birthday,
    // cyan pulse-speed = doctor) but judging pulse SPEED or a subtle colour
    // shade from 4m away is hard. This gives the same underlying countdown
    // (days_until_bday(), already used by the real LEDs) as a plain
    // "how many days left" number, shown on the phone as a simple LED
    // headcount instead — 3 lit = 3 days left, 1 lit = tomorrow, etc.
    // min_days_left == 999 means nothing upcoming within the next year.
    int16_t bday_min = 999;
    for(uint8_t i = 0; i < g_bday_count; i++) {
      int16_t d = days_until_bday(g_bdays[i].day, g_bdays[i].month);
      if(d < bday_min) bday_min = d;
    }
    int16_t doc_min = 999;
    for(uint8_t i = 0; i < g_doc_count; i++) {
      int16_t d = days_until_bday(g_docs[i].day, g_docs[i].month);
      if(d < doc_min) doc_min = d;
    }
    j += "\"bday_days\":" + String(bday_min) + ",";
    j += "\"doc_days\":"  + String(doc_min) + ",";

    // ── Day-of-week — NEW 2026-06-29, Henry's request ("cockpit" row) ───────
    // Monday=0 ... Sunday=6, same convention pill_render() already uses.
    j += "\"today_dow\":" + String((uint8_t)((rtc_dow() + 6) % 7));

    j += "}";
    webServer.send(200, "application/json", j);
  });

  // ── /bar3 : live phone mirror of the physical 3x10 status bar ────────────
  // Pure visual mirror (dots only, matching the photo Henry took) + one
  // short Afrikaans line per row, plus a plain "what's active now" summary
  // block underneath. Polls /bar3data every second via JS — no page reload,
  // so it can sit open on a phone propped on the kitchen counter and just
  // stay current.
  webServer.on("/bar3", HTTP_GET, []() {
    String page = html_header("Status Bar");
    page += "<h2 style='color:#ffa500'>Status Bar — Phone Mirror</h2>";
    page += "<p style='color:#888;font-size:0.85em;margin-top:-6px'>Shows the same colours as the lights on the clock, live.</p>";
    page += "<p><a href='/ledmanual' style='display:inline-block;background:#222;color:#0af;border:1px solid #0af;"
            "padding:8px 18px;border-radius:6px;font-weight:bold;text-decoration:none;font-size:0.9em'>&#128161; What does each light mean?</a></p>";

    // ── Day-of-week row — NEW 2026-06-29, Henry's "cockpit" request ────────
    // 7 dots, Monday through Sunday, soft fluorescent green so it's easy
    // on the eyes — not a bright/harsh green. Today's dot is highlighted
    // brighter so Ouma can tell which day it is at a glance, "cockpit
    // style" all on the one screen. Pure phone-page display — does not
    // touch the real LED panel.
    page += "<div style='background:#1a1a1a;border:1px solid #333;border-radius:10px;padding:12px;margin:10px 0;"
            "display:flex;justify-content:center;gap:10px'>";
    const char* dow_letters[7] = {"M","T","W","T","F","S","S"};
    for(uint8_t d = 0; d < 7; d++) {
      page += "<div style='text-align:center'>";
      page += "<div id='dow" + String(d) + "' style='width:28px;height:28px;border-radius:50%;"
              "background:#1a4a2a;border:2px solid #2a5a3a;margin:0 auto 4px;transition:all 0.3s;"
              "display:flex;align-items:center;justify-content:center;font-size:0.75em;color:#0a2a1a'></div>";
      page += "<span style='color:#777;font-size:0.75em'>" + String(dow_letters[d]) + "</span>";
      page += "</div>";
    }
    page += "</div>";

    // ── Block A: the 3x10 dot grid, same layout as the photo ──────────────
    page += "<div style='background:#1a1a1a;border:1px solid #333;border-radius:14px;padding:18px;margin:10px 0;display:flex;justify-content:center'>";
    page += "<table id='dotgrid' style='border-collapse:separate;border-spacing:14px'>";
    for(uint8_t r = 0; r < 10; r++) {
      page += "<tr>";
      for(uint8_t c = 0; c < 3; c++) {
        uint8_t idx = c * 10 + r;   // columns run top-to-bottom in the photo
        page += "<td><div id='dot" + String(idx) + "' style='width:34px;height:34px;border-radius:50%;"
                "background:#222;border:2px solid #444;transition:background 0.2s'></div></td>";
      }
      page += "</tr>";
    }
    page += "</table></div>";

    // ── Birthday / Doctor countdown — replaces the old static text legend,
    // per Henry's request (2026-06-29). Same idea as the dot grid above:
    // 3 dots, lit cumulatively — 3 lit = 3 days left, 2 lit = 2 days left,
    // 1 lit = tomorrow, all 3 lit+pulsing = TODAY. Sourced from the exact
    // same days_until_bday() countdown already driving the real purple
    // (birthday) and cyan (doctor) panel LEDs — this is just an easier
    // way to read the same number from across the room.
    const char* cd_rows[2][2] = {
      {"cd_bday", "&#127874; Birthday"},
      {"cd_doc",  "&#128203; Doctor Appt"}
    };
    page += "<div style='background:#1a1a1a;border:1px solid #333;border-radius:8px;padding:14px;margin-bottom:14px'>";
    for(uint8_t i = 0; i < 2; i++) {
      page += "<div style='display:flex;align-items:center;justify-content:space-between;padding:8px 4px;"
              "border-bottom:1px solid #292929'>";
      page += "<span id='" + String(cd_rows[i][0]) + "_lbl' style='font-size:1.05em'>" + String(cd_rows[i][1]) + "</span>";
      page += "<div style='display:flex;gap:6px'>";
      for(uint8_t d = 0; d < 3; d++) {
        page += "<div id='" + String(cd_rows[i][0]) + "_dot" + String(d) + "' style='width:20px;height:20px;border-radius:50%;"
                "background:#222;border:2px solid #444;transition:background 0.2s'></div>";
      }
      page += "</div></div>";
    }
    page += "</div>";

    // ── Block B: plain-language "what's active right now" summary ────────
    // Layout matches Henry's annotated screenshot: label on the left, a
    // clean round dot on the right — red = needs attention, green = calm.
    // Same colour rule as the LED panel itself, so it's one rule to learn.
    page += "<div style='background:#1a1a1a;border:1px solid #333;border-radius:8px;padding:14px'>";
    page += "<b style='color:#ffa500'>What's active now:</b><br><br>";
    const char* rows[6][2] = {   // Internet row added (was 5 after Pills retired)
      {"sum_wekker", "&#9210; Wekkers"},
      {"sum_wake",   "&#128276; Wake Alarm"},
      {"sum_alarm",  "&#128274; Security"},
      {"sum_egg",    "&#9201; Egg Timer"},
      {"sum_smoker", "&#128293; Smoker"},
      {"sum_inet",   "&#127760; Internet"}
    };
    for(uint8_t i = 0; i < 6; i++) {
      page += "<div style='display:flex;align-items:center;justify-content:space-between;padding:8px 4px;"
              "border-bottom:1px solid #292929'>";
      page += "<span id='" + String(rows[i][0]) + "_lbl' style='font-size:1.05em'>" + String(rows[i][1]) + "</span>";
      page += "<div id='" + String(rows[i][0]) + "_dot' style='width:22px;height:22px;border-radius:50%;"
              "background:#1a7a1a;border:2px solid #333;flex-shrink:0'></div>";
      page += "</div>";
    }
    page += "</div>";

    // ── Live polling JS ─────────────────────────────────────────────────
    // Same colour rule everywhere on this page: GREEN = calm/normal,
    // RED = needs attention right now. One rule, easy to remember.
    page += "<script>"
      "const GREEN='#1a7a1a', RED='#cc2222';"
      "function setDot(id,active){document.getElementById(id+'_dot').style.background=active?RED:GREEN;}"
      "function renderCountdown(prefix,days,color,labelBase,flashOn){"
        "let lit = (days>=0 && days<=3) ? (days===0 ? 3 : days) : 0;"  // 3 days left->3 dots, 2->2, 1->1, today->3 (flashing)
        "for(let d=0; d<3; d++){"
          "let el = document.getElementById(prefix+'_dot'+d);"
          "if(!el) continue;"
          "let on = d < lit;"
          "el.style.background = on ? (days===0 && !flashOn ? '#222' : color) : '#222';"
        "}"
        "let lbl = document.getElementById(prefix+'_lbl');"
        "if(!lbl) return;"
        "if(days===0) lbl.innerHTML = labelBase + ' — TODAY!';"
        "else if(days===1) lbl.innerHTML = labelBase + ' — tomorrow';"
        "else if(days>=2 && days<=3) lbl.innerHTML = labelBase + ' — ' + days + ' days';"
        "else lbl.innerHTML = labelBase;"
      "}"
      "let flashState=false;"
      "function poll(){"
        "flashState = !flashState;"
        "fetch('/bar3data').then(r=>r.json()).then(d=>{"
          "for(let i=0;i<30;i++){"
            "let el=document.getElementById('dot'+i);"
            "if(el) el.style.background=d.leds[i];"
          "}"
          "setDot('sum_wekker', d.wekker_active);"
          "document.getElementById('sum_wekker_lbl').innerHTML = '&#9210; Wekkers' + (d.wekker_active?' — RINGING!':'');"
          "setDot('sum_wake', d.wake_active);"
          "document.getElementById('sum_wake_lbl').innerHTML = '&#128276; Wake Alarm' + (d.wake_active?' — RINGING!':'');"
          "setDot('sum_alarm', d.alarm_active);"
          "document.getElementById('sum_alarm_lbl').innerHTML = '&#128274; Security' + (d.alarm_active?' — TRIGGERED!':(d.alarm_armed?' (armed)':' (off)'));"
          "setDot('sum_egg', d.egg_active);"
          "let eggtxt = d.egg_active ? (' — ' + String(d.egg_mm).padStart(2,'0') + ':' + String(d.egg_ss).padStart(2,'0')) : '';"
          "document.getElementById('sum_egg_lbl').innerHTML = '&#9201; Egg Timer' + eggtxt;"
          "setDot('sum_smoker', d.smoker_active||d.smoker_paused);"
          "setDot('sum_inet', !d.inet_ok);"
          "document.getElementById('sum_inet_lbl').innerHTML = '&#127760; Internet' + (d.inet_ok?' — OK':' — AF / DOWN!');"
          "let smktxt = d.smoker_active ? ' — running' : (d.smoker_paused ? ' — paused' : '');"
          "document.getElementById('sum_smoker_lbl').innerHTML = '&#128293; Smoker' + smktxt;"
          "renderCountdown('cd_bday', d.bday_days, '#a060e0', '&#127874; Birthday', flashState);"
          "renderCountdown('cd_doc',  d.doc_days,  '#00CFCF', '&#128203; Doctor Appt', flashState);"
          "for(let dd=0; dd<7; dd++){"
            "let el = document.getElementById('dow'+dd);"
            "if(!el) continue;"
            "if(dd === d.today_dow){"
              "el.style.background = '#39e07a';"   // soft fluorescent green, bright = today
              "el.style.borderColor = '#5fffa0';"
              "el.style.color = '#0a2a1a';"
            "} else {"
              "el.style.background = '#1a4a2a';"   // dim, easy on the eye
              "el.style.borderColor = '#2a5a3a';"
              "el.style.color = '#0a2a1a';"
            "}"
          "}"
        "}).catch(e=>{});"
      "}"
      "poll(); setInterval(poll, 1000);"
      "</script>";

    page += html_footer();
    webServer.send(200, "text/html", page);
  });

  // -- /ledmanual : LED Status Bar Manual — self-contained, no external file --
  // NEW 2026-06-29, Henry's request — a phone-readable lookup of what every
  // one of the 30 status LEDs means, baked into firmware the same way as
  // /braaitimes, so Ouma can check it on her phone without needing the PC
  // or the printed booklet. Reachable from /bar3 directly with a button,
  // and from the main nav.
  webServer.on("/ledmanual", HTTP_GET, []() {
    String page = html_header("LED Manual");
    page += "<h2 style='color:#ffa500'>&#128161; LED Status Bar Manual</h2>";
    page += "<p style='color:#888;font-size:0.9em;margin-top:-6px'>What each light on the Status Bar means.</p>";

    page += "<div style='background:#1a1a1a;border:1px solid #333;border-radius:8px;padding:14px'>";

    // ── LEFT COLUMN — Dag-strook (day of week) ────────────────────────────
    // Was the Pills column. The daily-pills subsystem is retired; the
    // 10-LED left column is now purely the day-of-week strip (still a cross-
    // check against Ouma se dag-gemerkte pilhouer) plus the doctor LED.
    page += "<h3 style='color:#ffa500;margin:18px 0 6px'>LEFT COLUMN — Dag-strook / Day of week</h3>";

    page += "<div style='display:flex;align-items:center;gap:12px;padding:8px 0;border-bottom:1px solid #292929'>"
            "<div style='width:26px;height:26px;border-radius:50%;flex-shrink:0;background:#1a7a1a'></div>"
            "<div><b>Today, soft GREEN pulse</b><br><span style='color:#aaa;font-size:0.9em'>Vandag se dag — sagte groen polsslag</span></div></div>";
    page += "<div style='display:flex;align-items:center;gap:12px;padding:8px 0;border-bottom:1px solid #292929'>"
            "<div style='width:26px;height:26px;border-radius:50%;flex-shrink:0;background:#444;border:1px solid #666'></div>"
            "<div><b>Past days, dim white</b><br><span style='color:#aaa;font-size:0.9em'>Earlier this week (history)</span></div></div>";
    page += "<div style='display:flex;align-items:center;gap:12px;padding:8px 0;border-bottom:1px solid #292929'>"
            "<div style='width:26px;height:26px;border-radius:50%;flex-shrink:0;background:#111;border:1px solid #333'></div>"
            "<div><b>Future days, off</b><br><span style='color:#aaa;font-size:0.9em'>Later this week — not due yet</span></div></div>";
    page += "<div style='display:flex;align-items:center;gap:12px;padding:8px 0;border-bottom:1px solid #292929'>"
            "<div style='width:26px;height:26px;border-radius:50%;flex-shrink:0;background:#00CFCF'></div>"
            "<div><b>Cyan pulse (bottom slot)</b><br><span style='color:#aaa;font-size:0.9em'>Doctor appointment countdown — speed shows urgency (see /bar3 for day count)</span></div></div>";

    // ── MIDDLE COLUMN — Medicine & reminders ──────────────────────────────
    page += "<h3 style='color:#ffa500;margin:18px 0 6px'>MIDDLE COLUMN — Medicine &amp; reminders</h3>";

    page += "<div style='display:flex;align-items:center;gap:12px;padding:8px 0;border-bottom:1px solid #292929'>"
            "<div style='width:26px;height:26px;border-radius:50%;flex-shrink:0;background:#cc2222'></div>"
            "<div><b>RED pulsing</b><br><span style='color:#aaa;font-size:0.9em'>Medicine dose due, not yet taken</span></div></div>";
    page += "<div style='display:flex;align-items:center;gap:12px;padding:8px 0;border-bottom:1px solid #292929'>"
            "<div style='width:26px;height:26px;border-radius:50%;flex-shrink:0;background:#1a7a1a'></div>"
            "<div><b>GREEN</b><br><span style='color:#aaa;font-size:0.9em'>Dose taken / on track</span></div></div>";
    page += "<div style='display:flex;align-items:center;gap:12px;padding:8px 0;border-bottom:1px solid #292929'>"
            "<div style='width:26px;height:26px;border-radius:50%;flex-shrink:0;background:#0030a0'></div>"
            "<div><b>Soft blue</b><br><span style='color:#aaa;font-size:0.9em'>Dose info, no action needed</span></div></div>";
    page += "<div style='display:flex;align-items:center;gap:12px;padding:8px 0;border-bottom:1px solid #292929'>"
            "<div style='width:26px;height:26px;border-radius:50%;flex-shrink:0;background:#a060e0'></div>"
            "<div><b>PURPLE</b><br><span style='color:#aaa;font-size:0.9em'>Birthday countdown — more LEDs lit = closer (see /bar3 for day count)</span></div></div>";

    // ── RIGHT COLUMN — Status ──────────────────────────────────────────────
    page += "<h3 style='color:#ffa500;margin:18px 0 6px'>RIGHT COLUMN — Status</h3>";

    page += "<div style='display:flex;align-items:center;gap:12px;padding:8px 0;border-bottom:1px solid #292929'>"
            "<div style='width:26px;height:26px;border-radius:50%;flex-shrink:0;background:#008000'></div>"
            "<div><b>Solid GREEN (top 2)</b><br><span style='color:#aaa;font-size:0.9em'>Alarm disarmed — house quiet</span></div></div>";
    page += "<div style='display:flex;align-items:center;gap:12px;padding:8px 0;border-bottom:1px solid #292929'>"
            "<div style='width:26px;height:26px;border-radius:50%;flex-shrink:0;background:#FF8C00'></div>"
            "<div><b>Solid ORANGE/AMBER (top 2)</b><br><span style='color:#aaa;font-size:0.9em'>Alarm armed — away mode</span></div></div>";
    page += "<div style='display:flex;align-items:center;gap:12px;padding:8px 0;border-bottom:1px solid #292929'>"
            "<div style='width:26px;height:26px;border-radius:50%;flex-shrink:0;background:#cc2222'></div>"
            "<div><b>Flashing RED/WHITE (top 2)</b><br><span style='color:#aaa;font-size:0.9em'>Alarm is ringing / siren — needs attention NOW</span></div></div>";
    page += "<div style='display:flex;align-items:center;gap:12px;padding:8px 0;border-bottom:1px solid #292929'>"
            "<div style='width:26px;height:26px;border-radius:50%;flex-shrink:0;background:#FFAA00'></div>"
            "<div><b>Flashing AMBER (last slot)</b><br><span style='color:#aaa;font-size:0.9em'>A Wekker is ringing — separate from the security/wake-alarm indicator above, so it is never confused with an emergency</span></div></div>";
    page += "<div style='display:flex;align-items:center;gap:12px;padding:8px 0;border-bottom:1px solid #292929'>"
            "<div style='width:26px;height:26px;border-radius:50%;flex-shrink:0;background:#444'></div>"
            "<div><b>Dim grey</b><br><span style='color:#aaa;font-size:0.9em'>24-hour clock format is ON</span></div></div>";
    page += "<div style='display:flex;align-items:center;gap:12px;padding:8px 0;border-bottom:1px solid #292929'>"
            "<div style='width:26px;height:26px;border-radius:50%;flex-shrink:0;background:#111;border:1px solid #333'></div>"
            "<div><b>Off (black)</b><br><span style='color:#aaa;font-size:0.9em'>12-hour clock format</span></div></div>";
    page += "<div style='display:flex;align-items:center;gap:12px;padding:8px 0;border-bottom:1px solid #292929'>"
            "<div style='width:26px;height:26px;border-radius:50%;flex-shrink:0;background:#603000'></div>"
            "<div><b>Soft orange-brown</b><br><span style='color:#aaa;font-size:0.9em'>Temperature shown in &#176;F</span></div></div>";
    page += "<div style='display:flex;align-items:center;gap:12px;padding:8px 0;border-bottom:1px solid #292929'>"
            "<div style='width:26px;height:26px;border-radius:50%;flex-shrink:0;background:#000028;border:1px solid #333'></div>"
            "<div><b>Dim navy</b><br><span style='color:#aaa;font-size:0.9em'>Temperature shown in &#176;C</span></div></div>";
    page += "<div style='display:flex;align-items:center;gap:12px;padding:8px 0;border-bottom:1px solid #292929'>"
            "<div style='width:26px;height:26px;border-radius:50%;flex-shrink:0;background:#0000a0'></div>"
            "<div><b>Blue</b><br><span style='color:#aaa;font-size:0.9em'>AM (morning) — only shown in 12-hour mode</span></div></div>";
    page += "<div style='display:flex;align-items:center;gap:12px;padding:8px 0;border-bottom:1px solid #292929'>"
            "<div style='width:26px;height:26px;border-radius:50%;flex-shrink:0;background:#a00000'></div>"
            "<div><b>Red</b><br><span style='color:#aaa;font-size:0.9em'>PM (afternoon/evening) — only shown in 12-hour mode</span></div></div>";
    page += "<div style='display:flex;align-items:center;gap:12px;padding:8px 0;border-bottom:1px solid #292929'>"
            "<div style='width:26px;height:26px;border-radius:50%;flex-shrink:0;background:#1a3a1a'></div>"
            "<div><b>Dim green</b><br><span style='color:#aaa;font-size:0.9em'>Sound module (DFPlayer) responded OK at startup</span></div></div>";
    page += "<div style='display:flex;align-items:center;gap:12px;padding:8px 0;border-bottom:1px solid #292929'>"
            "<div style='width:26px;height:26px;border-radius:50%;flex-shrink:0;background:#3a0000'></div>"
            "<div><b>Dim red</b><br><span style='color:#aaa;font-size:0.9em'>Sound module did not respond — check wiring</span></div></div>";
    page += "<div style='display:flex;align-items:center;gap:12px;padding:8px 0;border-bottom:1px solid #292929'>"
            "<div style='width:26px;height:26px;border-radius:50%;flex-shrink:0;background:#008000'></div>"
            "<div><b>Green</b><br><span style='color:#aaa;font-size:0.9em'>Yard/courtyard sensors are watching (armed) — same green as the disarmed indicator above</span></div></div>";
    page += "<div style='display:flex;align-items:center;gap:12px;padding:8px 0;border-bottom:1px solid #292929'>"
            "<div style='width:26px;height:26px;border-radius:50%;flex-shrink:0;background:#cc2222'></div>"
            "<div><b>Flashing RED</b><br><span style='color:#aaa;font-size:0.9em'>Yard/courtyard sensors paused (e.g. mowing the lawn)</span></div></div>";
    // Internet reachability LED (slot 27)
    page += "<div style='display:flex;align-items:center;gap:12px;padding:8px 0;border-bottom:1px solid #292929'>"
            "<div style='width:26px;height:26px;border-radius:50%;flex-shrink:0;background:#0028aa'></div>"
            "<div><b>Blue (internet slot)</b><br><span style='color:#aaa;font-size:0.9em'>Internet reachable — Pushover will get through</span></div></div>";
    page += "<div style='display:flex;align-items:center;gap:12px;padding:8px 0;border-bottom:1px solid #292929'>"
            "<div style='width:26px;height:26px;border-radius:50%;flex-shrink:0;background:#3a0000'></div>"
            "<div><b>Dim red (internet slot)</b><br><span style='color:#aaa;font-size:0.9em'>WiFi up but no internet (fibre/uplink down) — checked every 30s</span></div></div>";
    // Kalender event LED (slot 28)
    page += "<div style='display:flex;align-items:center;gap:12px;padding:8px 0;border-bottom:1px solid #292929'>"
            "<div style='width:26px;height:26px;border-radius:50%;flex-shrink:0;background:#c800b4'></div>"
            "<div><b>Magenta (kalender slot)</b><br><span style='color:#aaa;font-size:0.9em'>Gebeurtenis binnekort — pols 3/2/1 dae, flits op die dag (sien /calendar)</span></div></div>";

    page += "<p style='text-align:center;margin-top:14px'>"
            "<a href='/bar3' style='display:inline-block;background:#ffa500;color:#000;"
            "padding:10px 24px;border-radius:6px;font-weight:bold;text-decoration:none'>&#128247; Back to Status Bar</a></p>";

    page += html_footer();
    webServer.send(200, "text/html", page);
  });

  // -- /braaitimes : Braai Times Reference — self-contained, no external file --
  // Baked straight into firmware (same pattern as every other page here) so it
  // always comes back after a factory restore / re-flash, with nothing to lose
  // separately if a phone or tablet goes missing.
  webServer.on("/braaitimes", HTTP_GET, []() {
    String page = html_header("Braai Times");
    page += "<h2 style='color:#FF8C00'>&#129385; Braai Times Reference</h2>";

    // -- Coal heat hand-test --
    page += "<div class='stat'>";
    page += "<div class='label'>Hand-toets — hou hand 10-12cm oor rooster</div>";
    page += "<table style='width:100%;border-collapse:collapse;margin-top:6px;font-size:0.9rem'>";
    page += "<tr><td style='padding:4px;color:#aaa'>Baie warm kole</td><td style='padding:4px;color:#fff;text-align:right'>2-3 sek &rarr; Steaks</td></tr>";
    page += "<tr><td style='padding:4px;color:#aaa'>Medium kole</td><td style='padding:4px;color:#fff;text-align:right'>5-7 sek &rarr; Wors / voorgekookte rib</td></tr>";
    page += "<tr><td style='padding:4px;color:#aaa'>Matig / koel kole</td><td style='padding:4px;color:#fff;text-align:right'>8-10 sek &rarr; Rou rib</td></tr>";
    page += "</table></div>";

    // -- Steaks & wors table --
    page += "<h3 style='color:#0af;margin-top:18px'>Dik Steaks &amp; Wors</h3>";
    page += "<p style='color:#888;font-size:0.8em;margin:2px 0 8px'>20-30mm steaks &middot; draai elke 1-1.5 min &middot; wors elke 2-3 min</p>";
    page += "<table style='width:100%;border-collapse:collapse;font-size:0.85rem'>";
    page += "<tr><th style='background:#e07b00;color:#fff;padding:6px;text-align:left'>Snit</th>"
            "<th style='background:#c05a00;color:#fff;padding:6px'>Rare</th>"
            "<th style='background:#e07b00;color:#fff;padding:6px'>Med Rare</th>"
            "<th style='background:#c05a00;color:#fff;padding:6px'>Medium</th>"
            "<th style='background:#e07b00;color:#fff;padding:6px'>Well Done</th></tr>";
    struct SteakRow { const char* name; const char* r; const char* mr; const char* m; const char* wd; };
    SteakRow steaks[] = {
      {"Fillet",          "4-5 min", "6-7 min",  "8-9 min",   "Nie aanbeveel"},
      {"Sirloin/Rump",    "5-6 min", "7-8 min",  "9-10 min",  "12-14 min"},
      {"Ribeye",          "Nie aanb.","7-8 min", "9-11 min",  "13-15 min"},
      {"T-Bone/Club",     "6-7 min", "8-9 min",  "10-12 min", "14-16 min"},
      {"Medium Wors",     "Nie veilig","Nie veilig","10-12 min","14-15 min"},
      {"Dik Boerewors",   "Nie veilig","Nie veilig","14-16 min","18-20 min"},
    };
    for (auto& s : steaks) {
      page += "<tr><td style='padding:6px;background:#1a1a1a;color:#ffa500;font-weight:bold'>" + String(s.name) + "</td>";
      page += "<td style='padding:6px;background:#1a1a1a;text-align:center'>" + String(s.r)  + "</td>";
      page += "<td style='padding:6px;background:#1a1a1a;text-align:center'>" + String(s.mr) + "</td>";
      page += "<td style='padding:6px;background:#1a1a1a;text-align:center'>" + String(s.m)  + "</td>";
      page += "<td style='padding:6px;background:#1a1a1a;text-align:center'>" + String(s.wd) + "</td></tr>";
    }
    page += "</table>";

    // -- Ribs table --
    page += "<h3 style='color:#0af;margin-top:18px'>Vark- &amp; Beesribbetjies</h3>";
    page += "<p style='color:#888;font-size:0.8em;margin:2px 0 8px'>Matige hitte sodat die vet reg uitbraai</p>";
    page += "<table style='width:100%;border-collapse:collapse;font-size:0.85rem'>";
    page += "<tr><th style='background:#e07b00;color:#fff;padding:6px;text-align:left'>Tipe</th>"
            "<th style='background:#c05a00;color:#fff;padding:6px'>Kole</th>"
            "<th style='background:#e07b00;color:#fff;padding:6px'>Tyd</th>"
            "<th style='background:#c05a00;color:#fff;padding:6px;text-align:left'>Wenk</th></tr>";
    struct RibRow { const char* name; const char* heat; const char* time; const char* note; };
    RibRow ribs[] = {
      {"Varkrib (voorgekook)", "Medium",      "15-20 min", "Draai elke 3-5 min"},
      {"Varkrib (rou)",        "Matig/koel",  "35-45 min", "Sous laaste 10 min"},
      {"Beesrib (rou, direk)", "Baie matig",  "45-60 min", "Eers lank op beenkant"},
      {"Beesrib (gestoom)",    "Warm/medium", "15-20 min", "Net vir geur & bros vet"},
    };
    for (auto& r : ribs) {
      page += "<tr><td style='padding:6px;background:#1a1a1a;color:#ffa500;font-weight:bold'>" + String(r.name) + "</td>";
      page += "<td style='padding:6px;background:#1a1a1a;text-align:center'>" + String(r.heat) + "</td>";
      page += "<td style='padding:6px;background:#1a1a1a;text-align:center'>" + String(r.time) + "</td>";
      page += "<td style='padding:6px;background:#1a1a1a;color:#aaa;font-size:0.8em'>" + String(r.note) + "</td></tr>";
    }
    page += "</table>";

    // -- Chicken table --------------------------------------------------------
    // Times cross-checked against SA braai guides (Daily Maverick/Tony Jackman,
    // PnP Fresh Living, MyKitchen): flattie 45-60 min turning every 5-10 min is
    // the consensus. The coal-heat hand-test above (8-10 sek) only applies to
    // the flattie — that's LOW heat. Pieces want 6-7 sek medium, wings/fillets
    // hotter still, which is why heat is listed per-cut in the table below
    // rather than as a single hand-test number for all chicken.
    page += "<h3 style='color:#0af;margin-top:18px'>Hoender / Chicken</h3>";
    page += "<p style='color:#888;font-size:0.8em;margin:2px 0 8px'>Gaar by 74&deg;C binne, of wanneer die sap heeltemal skoon loop — nooit pienk by die been nie</p>";
    page += "<table style='width:100%;border-collapse:collapse;font-size:0.85rem'>";
    page += "<tr><th style='background:#e07b00;color:#fff;padding:6px;text-align:left'>Snit</th>"
            "<th style='background:#c05a00;color:#fff;padding:6px'>Kole</th>"
            "<th style='background:#e07b00;color:#fff;padding:6px'>Tyd</th>"
            "<th style='background:#c05a00;color:#fff;padding:6px;text-align:left'>Wenk</th></tr>";
    struct ChickenRow { const char* name; const char* heat; const char* time; const char* note; };
    ChickenRow chicken[] = {
      {"Flattie / Spatchcock",   "Matig (8-10 sek)",  "45-60 min", "Draai elke 5-10 min, hou aan bedruip"},
      {"Stukke (been-in)",       "Medium (6-7 sek)",  "20-30 min", "Sny tot op been; dy-gewrig los = gaar"},
      {"Vlerkies",               "Medium-warm",       "15-20 min", "Draai gereeld — brand maklik"},
      {"Borsfilette (plat)",     "Medium-warm",       "8-12 min",  "Slaan eers plat vir egalige gaarmaak"},
      {"Sosaties",               "Medium-warm",       "12-15 min", "Draai gereeld, bedruip met marinade"},
    };
    for (auto& c : chicken) {
      page += "<tr><td style='padding:6px;background:#1a1a1a;color:#ffa500;font-weight:bold'>" + String(c.name) + "</td>";
      page += "<td style='padding:6px;background:#1a1a1a;text-align:center'>" + String(c.heat) + "</td>";
      page += "<td style='padding:6px;background:#1a1a1a;text-align:center'>" + String(c.time) + "</td>";
      page += "<td style='padding:6px;background:#1a1a1a;color:#aaa;font-size:0.8em'>" + String(c.note) + "</td></tr>";
    }
    page += "</table>";
    page += "<p style='color:#888;font-size:0.8em;margin:6px 0 0'>Suiker/heuning-marinades brand — smeer eers die laaste 10-15 min, soos met die ribsous hieronder.</p>";

    // -- Sticky rib sauce --
    page += "<h3 style='color:#0af;margin-top:18px'>Sticky Rib-Braaisous</h3>";
    page += "<div class='stat'><div class='label'>Smeer net laaste 5-10 min — anders brand dit swart</div>";
    page += "<ul style='margin:8px 0;padding-left:20px;color:#ddd;font-size:0.85rem'>";
    page += "<li>1 koppie Tamatiesous</li><li>&frac12; koppie Blatjang</li>";
    page += "<li>&frac14; koppie Worcestersous</li><li>2 el Appelkooskonfyt</li>";
    page += "<li>2 el Bruinsuiker of heuning</li><li>1 el Appelasyn</li>";
    page += "<li>1 tl Rookbraaisout</li><li>1 tl Knoffelpoeier</li>";
    page += "<li>&frac12; tl Cayenne (opsioneel)</li></ul>";
    page += "<div class='label' style='margin-top:6px'>Bring tot kookpunt oor lae hitte, prut 5 min, roer tot dik en blink.</div></div>";

    // -- Peri-Peri chicken marinade ---------------------------------------------
    page += "<h3 style='color:#0af;margin-top:18px'>Klassieke Peri-Peri Marinade (Hoender)</h3>";
    page += "<div class='stat'><div class='label'>Genoeg vir 1 flattie of &plusmn;1.5kg stukke</div>";
    page += "<ul style='margin:8px 0;padding-left:20px;color:#ddd;font-size:0.85rem'>";
    page += "<li>6-8 Bird's Eye rissies (peri-peri)</li><li>4 Knoffelhuisies</li>";
    page += "<li>&frac12; koppie Plantolie</li><li>&#8531; koppie Suurlemoensap</li>";
    page += "<li>&frac14; koppie Rooiwynasyn</li><li>1 el Gerookte Paprika</li>";
    page += "<li>1 tl Gedroogde Origanum</li><li>1 tl Suiker</li>";
    page += "<li>1 tl Sout &amp; Peper</li></ul>";
    page += "<div class='label' style='margin-top:6px'>Meng alles glad in 'n blender. Gooi &#8532; oor die hoender en marineer 4+ uur. "
            "Hou &#8531; terug om mee te bedruip op die braai. Laat rus 5 min onder foelie voor opdiening.</div></div>";

    // -- 3 golden rules --
    page += "<h3 style='color:#0af;margin-top:18px'>3 Goue Reëls</h3>";
    page += "<div class='stat'><ol style='margin:0;padding-left:20px;color:#ddd;font-size:0.85rem'>";
    page += "<li>Kamertemperatuur: 20 min voor die tyd uit yskas</li>";
    page += "<li>Seël die vet: staan Sirloin/Rump 1-2 min op vetrand eerste</li>";
    page += "<li>Rus die vleis: 5 min onder foelie voor jy sny</li>";
    page += "</ol></div>";

    page += html_footer();
    webServer.send(200, "text/html", page);
  });

  webServer.on("/braai_enable", HTTP_POST, []() {
    g_braai_enabled = (webServer.arg("en") == "1");
    settings_save();
    webServer.sendHeader("Location", "/braai");
    webServer.send(303);
  });

  webServer.on("/braai_start", HTTP_POST, []() {
    if(!g_braai_running) {   // guard: don't let a stray/duplicate request reset an active session
      uint16_t total = (uint16_t)webServer.arg("total").toInt();
      uint16_t turn  = (uint16_t)webServer.arg("turn").toInt();
      if(total >= 60 && total <= 3600) g_braai_total_sec = total;
      if(turn  >= 10 && turn  <= 600)  g_braai_turn_sec  = turn;
      g_braai_start_unix   = rtc.now().unixtime();
      g_braai_pause_unix   = 0;
      g_braai_paused_accum = 0;
      g_braai_paused       = false;
      g_braai_running      = true;
      g_braai_last_turn    = 0;
      g_braai_done_played  = false;
      settings_save();
    }
    webServer.sendHeader("Location", "/braai");
    webServer.send(303);
  });

  webServer.on("/braai_pause", HTTP_POST, []() {
    if(g_braai_running && !g_braai_paused) {
      g_braai_pause_unix = rtc.now().unixtime();
      g_braai_paused     = true;
      settings_save();
    }
    webServer.sendHeader("Location", "/braai");
    webServer.send(303);
  });

  webServer.on("/braai_resume", HTTP_POST, []() {
    if(g_braai_running && g_braai_paused) {
      uint32_t now_unix = rtc.now().unixtime();
      if(now_unix > g_braai_pause_unix) g_braai_paused_accum += (now_unix - g_braai_pause_unix);
      g_braai_paused = false;
      settings_save();
    }
    webServer.sendHeader("Location", "/braai");
    webServer.send(303);
  });

  webServer.on("/braai_stop", HTTP_POST, []() {
    g_braai_running      = false;
    g_braai_paused       = false;
    g_braai_start_unix   = 0;
    g_braai_pause_unix   = 0;
    g_braai_paused_accum = 0;
    g_braai_last_turn    = 0;
    g_braai_done_played  = false;
    settings_save();
    webServer.sendHeader("Location", "/braai");
    webServer.send(303);
  });

  // ── Birthday Calendar web routes ───────────────────────────────────────────
  webServer.on("/birthdays", HTTP_GET, []() {
    String page = html_header("Verjaarsdae / Birthdays");
    page += "<h2 style='color:#FFD700'>&#127874; Verjaarsdagkalender</h2>";
    page += "<p style='color:#aaa'>Voer tot 30 name en verjaarsdae in. Gebruik die web-portaal om te wysig.</p>";
    page += "<form method='POST' action='/bday_save'>";
    page += "<table style='width:100%;border-collapse:collapse'>";
    page += "<tr><th style='text-align:left;color:#FFD700;padding:4px'>#</th>";
    page += "<th style='text-align:left;color:#FFD700;padding:4px'>Naam</th>";
    page += "<th style='text-align:left;color:#FFD700;padding:4px'>Dag</th>";
    page += "<th style='text-align:left;color:#FFD700;padding:4px'>Maand</th></tr>";
    for (uint8_t i = 0; i < BDAY_MAX; i++) {
      String nm  = (i < g_bday_count) ? String(g_bdays[i].name) : "";
      String dd  = (i < g_bday_count) ? String(g_bdays[i].day)   : "";
      String mm  = (i < g_bday_count) ? String(g_bdays[i].month) : "";
      String bg  = (i % 2 == 0) ? "#1a1a2e" : "#16213e";
      page += "<tr style='background:" + bg + "'>";
      page += "<td style='padding:4px;color:#888'>" + String(i+1) + "</td>";
      page += "<td style='padding:2px'><input name='n" + String(i) + "' value='" + nm + "' maxlength='13' style='width:120px;background:#222;color:#fff;border:1px solid #444;padding:3px'></td>";
      page += "<td style='padding:2px'><input name='d" + String(i) + "' type='number' min='1' max='31' value='" + dd + "' style='width:55px;background:#222;color:#fff;border:1px solid #444;padding:3px'></td>";
      page += "<td style='padding:2px'><input name='m" + String(i) + "' type='number' min='1' max='12' value='" + mm + "' style='width:55px;background:#222;color:#fff;border:1px solid #444;padding:3px'></td>";
      page += "</tr>";
    }
    page += "</table><br>";
    page += "<input type='submit' value='Stoor Verjaarsdae' style='background:#FFD700;color:#000;padding:8px 20px;border:none;font-weight:bold;cursor:pointer'>";
    page += "</form>";
    // Status
    page += "<hr style='border-color:#333;margin:16px 0'>";
    if (g_bday_today && g_bday_today_idx < g_bday_count) {
      page += "<p style='color:#FFD700;font-size:1.2em'>&#127881; Vandag is <b>";
      page += String(g_bdays[g_bday_today_idx].name);
      page += "</b> se verjaarsdag! &#127881;</p>";
    } else {
      // Show who's coming up in the next 7 days
      page += "<p style='color:#aaa'>Komende verjaarsdae (volgende 7 dae):</p><ul>";
      bool any = false;
      for (uint8_t i = 0; i < g_bday_count; i++) {
        int16_t d = days_until_bday(g_bdays[i].day, g_bdays[i].month);
        if (d > 0 && d <= 7) {
          page += "<li style='color:#FFD700'>" + String(g_bdays[i].name) + " — oor " + String(d) + " dag(e)</li>";
          any = true;
        }
      }
      if (!any) page += "<li style='color:#888'>Geen verjaarsdae in die volgende 7 dae nie.</li>";
      page += "</ul>";
    }
    page += html_footer();
    webServer.send(200, "text/html", page);
  });

  webServer.on("/bday_save", HTTP_POST, []() {
    g_bday_count = 0;
    for (uint8_t i = 0; i < BDAY_MAX; i++) {
      String nm = webServer.arg("n" + String(i));
      String ds = webServer.arg("d" + String(i));
      String ms = webServer.arg("m" + String(i));
      nm.trim();
      if (nm.length() == 0 || ds.length() == 0 || ms.length() == 0) continue;
      uint8_t d = (uint8_t)ds.toInt();
      uint8_t m = (uint8_t)ms.toInt();
      if (d < 1 || d > 31 || m < 1 || m > 12) continue;
      g_bdays[g_bday_count].day   = d;
      g_bdays[g_bday_count].month = m;
      strncpy(g_bdays[g_bday_count].name, nm.c_str(), 13);
      g_bdays[g_bday_count].name[13] = '\0';
      g_bday_count++;
      if (g_bday_count >= BDAY_MAX) break;
    }
    bday_save();
    webServer.sendHeader("Location", "/birthdays");
    webServer.send(303);
  });

  // ── Doctor Appointment web routes ──────────────────────────────────────────
  // ── /buttons : Front-panel pushbutton quick reference ──────────
  // Completes the manual set (LED Manual for the lights, Braai Times for the
  // fire, this for the fingers). Descriptions match CURRENT firmware — the
  // photo-era notes had two stale entries: UP's dismiss is on RELEASE now
  // (hold = snooze, the  rework), and PILLS is retired since .
  webServer.on("/buttons", HTTP_GET, []() {
    String page = html_header("Knoppies");
    page += "<h2 style='color:#0af'>&#128280; Voorpaneel Knoppies / Front Panel Buttons</h2>";
    struct BtnRow { const char* sw; const char* col; const char* name; const char* desc; };
    static const BtnRow rows[] = {
      {"Sw1",  "#cc2222", "MODE",         "Blaai deur 6 skerms: Tyd, Alarm-Set, Egg-timer (Running), Egg-timer (Set), Temperatuur, Uptime — en terug na Tyd. Hou 1 sekonde in om tussen Celsius en Fahrenheit te wissel."},
      {"Sw2",  "#3355cc", "SET",          "Stel ure / timer minute. Begin of onderbreek die aftel-timer."},
      {"Sw3",  "#dddddd", "UP",           "Stel minute / timer sekondes. Alarm lui? Kort druk-en-LOS = stil. Hou &gt;&frac12; sek in = 9 min sluimer (snooze)."},
      {"Sw4",  "#dddddd", "ALARM AAN/AF", "Skakel die gekose alarm aan of af."},
      {"Sw5",  "#cc2222", "CHIMES",       "Kwartier-klokkespel aan of af. (Die klok bly stil in 'n minuut waar 'n stem-aankondiging praat.)"},
      {"Sw6",  "#3355cc", "VOL OP",       "Klank harder."},
      {"Sw7",  "#ddcc22", "VOL AF",       "Klank sagter."},
      {"Sw8",  "#ddcc22", "SOUND",        "Kies die klokkespel-melodie — 5 verskillende deuntjies."},
      {"Sw9",  "#cc2222", "RAINBOW",      "Wys die reënboog-vertoning op die hoofskerm. Enige ander knoppie skakel terug na die normale tydvertoning."},
      {"Sw10", "#3355cc", "WIFI RESET",   "Hou 3 sekondes in om die klok te herbegin (herprobeer WiFi-konneksie na kragonderbreking). Kort druk: as die rooker tans loop, wys dit vinnig hoe lank (UU:MM) op die skerm; anders doen kort druk niks. (Ou daaglikse-pille funksie is afgetree.)"},
      {"Sw11", "#ddcc22", "MEDICINE",     "Druk n&aacute; Ouma se medisyne geneem is — bevestig die dosis, oranje LED gaan groen."},
      {"Sw12", "#dddddd", "BIRTHDAY",     "Erken 'n verjaarsdag-, doktersafspraak- of kalendergebeurtenis-herinnering. Stop ook 'n Wekker wat lui."},
    };
    for (auto& r : rows) {
      page += "<div style='display:flex;align-items:center;gap:12px;padding:9px 0;border-bottom:1px solid #292929'>";
      page += "<div style='width:26px;height:26px;border-radius:50%;flex-shrink:0;background:" + String(r.col) + ";border:1px solid #555'></div>";
      page += "<div style='min-width:44px;color:#888;font-size:0.8em'>" + String(r.sw) + "</div>";
      page += "<div><b>" + String(r.name) + "</b><br><span style='color:#aaa;font-size:0.9em'>" + String(r.desc) + "</span></div></div>";
    }
    page += "<p style='color:#666;font-size:0.8em;margin-top:12px'>Wenk: as die WEKALARM lui, werk net die wit UP knop (los = stil, hou = sluimer) — al die ander knoppe word ge&iuml;gnoreer totdat dit stil is.</p>";
    page += html_footer();
    webServer.send(200, "text/html", page);
  });

  // ── /notepad : Notaboekie (Ouma's request) ─────────────────────────────────
  // "Small things she forgot while shopping." Tap ✓ to strike through when
  // bought, X to delete, one button to clear all completed. NVS-persisted.
  webServer.on("/notepad", HTTP_GET, []() {
    String page = html_header("Notaboekie");
    page += "<h2 style='color:#e0c020'>&#128221; Notaboekie</h2>";
    page += "<p style='color:#888;font-size:0.85em'>Klein goedjies om te onthou — merk &#10003; sodra gekoop.</p>";
    for (uint8_t i = 0; i < g_note_count; i++) {
      String txt = String(g_notes[i].text);
      if (g_notes[i].done)
        txt = "<span style='text-decoration:line-through;color:#666'>" + txt + "</span>";
      page += "<div style='padding:7px 0;border-bottom:1px solid #292929;font-size:1.05rem'>";
      page += "<form method='POST' action='/note_tog' style='display:inline'>"
              "<input type='hidden' name='i' value='" + String(i) + "'>"
              "<input type='submit' value='&#10003;' style='background:#1a7a1a;color:#fff;border:0;border-radius:4px;padding:3px 10px'></form> ";
      page += txt;
      page += " <form method='POST' action='/note_del' style='display:inline;float:right'>"
              "<input type='hidden' name='i' value='" + String(i) + "'>"
              "<input type='submit' value='X' style='background:#801515;color:#fff;border:0;border-radius:4px;padding:3px 10px'></form>";
      page += "</div>";
    }
    if (g_note_count == 0) page += "<p style='color:#888'>Niks op die lysie nie — alles onthou!</p>";
    if (g_note_count < NOTE_MAX) {
      page += "<form method='POST' action='/note_add' style='margin-top:12px'>";
      page += "<input name='t' type='text' maxlength='40' placeholder='bv. melk, broodmeel...' style='width:200px'> ";
      page += "<input type='submit' value='Voeg by'></form>";
    } else {
      page += "<p style='color:#ffa500'>Lysie vol (" + String(NOTE_MAX) + ") — vee eers iets uit.</p>";
    }
    page += "<form method='POST' action='/note_clear' style='margin-top:14px'>"
            "<input type='submit' value='Vee gedoen uit' style='background:#444;color:#fff;border:0;border-radius:6px;padding:8px 14px'></form>";
    page += html_footer();
    webServer.send(200, "text/html", page);
  });

  webServer.on("/note_add", HTTP_POST, []() {
    String t = webServer.arg("t");
    t.trim();
    if (g_note_count < NOTE_MAX && t.length() > 0) {
      strncpy(g_notes[g_note_count].text, t.c_str(), sizeof(g_notes[g_note_count].text) - 1);
      g_notes[g_note_count].text[sizeof(g_notes[g_note_count].text) - 1] = '\0';
      g_notes[g_note_count].done = false;
      g_note_count++;
      note_save();
    }
    webServer.sendHeader("Location", "/notepad");
    webServer.send(303);
  });

  webServer.on("/note_tog", HTTP_POST, []() {
    int i = webServer.arg("i").toInt();
    if (i >= 0 && i < g_note_count) { g_notes[i].done = !g_notes[i].done; note_save(); }
    webServer.sendHeader("Location", "/notepad");
    webServer.send(303);
  });

  webServer.on("/note_del", HTTP_POST, []() {
    int i = webServer.arg("i").toInt();
    if (i >= 0 && i < g_note_count) {
      for (uint8_t k = (uint8_t)i; k + 1 < g_note_count; k++) g_notes[k] = g_notes[k + 1];
      g_note_count--;
      note_save();
    }
    webServer.sendHeader("Location", "/notepad");
    webServer.send(303);
  });

  webServer.on("/note_clear", HTTP_POST, []() {
    uint8_t n = 0;
    for (uint8_t i = 0; i < g_note_count; i++)
      if (!g_notes[i].done) g_notes[n++] = g_notes[i];
    if (n != g_note_count) { g_note_count = n; note_save(); }
    webServer.sendHeader("Location", "/notepad");
    webServer.send(303);
  });

  // ── /calendar : Kalender ──────────────────────────
  // One month grid overlaying everything the clock knows about dates:
  // birthdays (🎂), doctor appointments (🩺), general events (📌), with the
  // weekly Wekkers shown as a legend line. Below: upcoming-45-days list and
  // the add/delete form for general events. Grid math uses g_year/g_dow
  // cached by rtc_sync() — no I2C from this Core 0 handler.
  webServer.on("/calendar", HTTP_GET, []() {
    String page = html_header("Kalender");
    static const char* MAAND[13] = {"","Januarie","Februarie","Maart","April","Mei","Junie",
                                    "Julie","Augustus","September","Oktober","November","Desember"};
    uint16_t yr = g_year; uint8_t mo = g_month;
    // days in this month (leap-aware)
    static const uint8_t dim[13] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
    uint8_t ndays = dim[mo];
    if (mo == 2 && ((yr % 4 == 0 && yr % 100 != 0) || yr % 400 == 0)) ndays = 29;
    // weekday of the 1st, Monday=0: today is g_dow (0=Sun..6=Sat) on day g_day
    // step back (g_day-1) days from today's weekday
    int8_t dow_today_mon0 = (int8_t)((g_dow + 6) % 7);
    int8_t first_mon0 = (int8_t)(((dow_today_mon0 - ((g_day - 1) % 7)) % 7 + 7) % 7);

    page += "<h2 style='color:#0af'>&#128197; " + String(MAAND[mo]) + " " + String(yr) + "</h2>";

    // Wekker legend line
    {
      String wk = "";
      static const char* WD[7] = {"So","Ma","Di","Wo","Do","Vr","Sa"};
      for (uint8_t i = 0; i < NUM_ALARM_SLOTS; i++) {
        if (!g_wekker_enabled[i]) continue;
        char t[8]; snprintf(t, sizeof(t), "%02u:%02u", g_wekker_hh[i], g_wekker_mm[i]);
        wk += "&#9200; " + String(t) + " (";
        bool first = true;
        for (uint8_t d = 0; d < 7; d++) {
          if ((g_wekker_days[i] >> d) & 1) { if(!first) wk += " "; wk += WD[d]; first = false; }
        }
        wk += ") &nbsp; ";
      }
      if (wk.length()) page += "<p style='color:#888;font-size:0.85em'>" + wk + "</p>";
    }

    // Month grid
    page += "<table style='width:100%;border-collapse:collapse;text-align:center;font-size:0.85rem'>";
    page += "<tr>";
    static const char* HDR[7] = {"Ma","Di","Wo","Do","Vr","Sa","So"};
    for (uint8_t h = 0; h < 7; h++)
      page += "<th style='background:#222;color:#0af;padding:4px'>" + String(HDR[h]) + "</th>";
    page += "</tr><tr>";
    for (int8_t b = 0; b < first_mon0; b++) page += "<td style='background:#141414'></td>";
    uint8_t col = first_mon0;
    for (uint8_t d = 1; d <= ndays; d++) {
      String marks = "";
      for (uint8_t i = 0; i < g_bday_count; i++)
        if (g_bdays[i].day == d && g_bdays[i].month == mo) { marks += "&#127874;"; break; }
      for (uint8_t i = 0; i < g_doc_count; i++)
        if (g_docs[i].day == d && g_docs[i].month == mo)   { marks += "&#129658;"; break; }
      for (uint8_t i = 0; i < g_evt_count; i++)
        if (g_events[i].day == d && g_events[i].month == mo) { marks += "&#128204;"; break; }
      String cellstyle = "padding:6px 2px;background:#1a1a1a;border:1px solid #292929";
      if (d == g_day) cellstyle += ";border:2px solid #0af;background:#10222e";
      page += "<td style='" + cellstyle + "'><b>" + String(d) + "</b>";
      if (marks.length()) page += "<br>" + marks;
      page += "</td>";
      if (++col == 7 && d < ndays) { page += "</tr><tr>"; col = 0; }
    }
    while (col > 0 && col < 7) { page += "<td style='background:#141414'></td>"; col++; }
    page += "</tr></table>";
    page += "<p style='color:#666;font-size:0.8em'>&#127874; verjaarsdag &nbsp; &#129658; dokter &nbsp; &#128204; gebeurtenis &nbsp; | vandag = blou raam</p>";

    // Upcoming 45 days — merged, sorted by distance
    page += "<h3 style='color:#0af;margin-top:16px'>Volgende 45 dae</h3>";
    {
      struct Up { int16_t d; String txt; };
      Up ups[BDAY_MAX + DOC_MAX + EVT_MAX];
      uint8_t n = 0;
      for (uint8_t i = 0; i < g_bday_count; i++) {
        int16_t dd = days_until_bday(g_bdays[i].day, g_bdays[i].month);
        if (dd >= 0 && dd <= 45) ups[n++] = { dd, "&#127874; " + String(g_bdays[i].name) };
      }
      for (uint8_t i = 0; i < g_doc_count; i++) {
        int16_t dd = days_until_bday(g_docs[i].day, g_docs[i].month);
        if (dd >= 0 && dd <= 45) ups[n++] = { dd, "&#129658; " + String(g_docs[i].desc) };
      }
      for (uint8_t i = 0; i < g_evt_count; i++) {
        int16_t dd = days_until_bday(g_events[i].day, g_events[i].month);
        if (dd >= 0 && dd <= 45) ups[n++] = { dd, "&#128204; " + String(g_events[i].desc) };
      }
      for (uint8_t a = 0; a + 1 < n; a++)          // tiny insertion-ish sort, n <= 60
        for (uint8_t b2 = a + 1; b2 < n; b2++)
          if (ups[b2].d < ups[a].d) { Up t = ups[a]; ups[a] = ups[b2]; ups[b2] = t; }
      if (n == 0) page += "<p style='color:#888'>Niks in die volgende 45 dae nie.</p>";
      for (uint8_t i = 0; i < n; i++) {
        String when = (ups[i].d == 0) ? "<b style='color:#0f0'>VANDAG</b>" :
                      (ups[i].d == 1) ? "<b style='color:#ffa500'>môre</b>" :
                      "oor " + String(ups[i].d) + " dae";
        page += "<div style='padding:5px 0;border-bottom:1px solid #292929'>" + ups[i].txt +
                " — " + when + "</div>";
      }
    }

    // Add / delete general events
    page += "<h3 style='color:#0af;margin-top:16px'>&#128204; Gebeurtenisse (" + String(g_evt_count) + "/" + String(EVT_MAX) + ")</h3>";
    for (uint8_t i = 0; i < g_evt_count; i++) {
      page += "<div style='padding:5px 0;border-bottom:1px solid #292929'>" +
              String(g_events[i].day) + "/" + String(g_events[i].month) + " — " + String(g_events[i].desc) +
              " <form method='POST' action='/evt_del' style='display:inline'>"
              "<input type='hidden' name='i' value='" + String(i) + "'>"
              "<input type='submit' value='X' style='background:#801515;color:#fff;border:0;border-radius:4px;padding:2px 8px'></form></div>";
    }
    if (g_evt_count < EVT_MAX) {
      page += "<form method='POST' action='/evt_add' style='margin-top:10px'>";
      page += "Dag <input name='d' type='number' min='1' max='31' style='width:52px'> ";
      page += "Maand <input name='m' type='number' min='1' max='12' style='width:52px'> ";
      page += "<input name='n' type='text' maxlength='23' placeholder='beskrywing' style='width:150px'> ";
      page += "<input type='submit' value='Voeg by'></form>";
    }
    page += html_footer();
    webServer.send(200, "text/html", page);
  });

  webServer.on("/evt_add", HTTP_POST, []() {
    int d = webServer.arg("d").toInt();
    int m = webServer.arg("m").toInt();
    String n = webServer.arg("n");
    if (g_evt_count < EVT_MAX && d >= 1 && d <= 31 && m >= 1 && m <= 12 && n.length() > 0) {
      g_events[g_evt_count].day   = (uint8_t)d;
      g_events[g_evt_count].month = (uint8_t)m;
      strncpy(g_events[g_evt_count].desc, n.c_str(), sizeof(g_events[g_evt_count].desc) - 1);
      g_events[g_evt_count].desc[sizeof(g_events[g_evt_count].desc) - 1] = '\0';
      g_evt_count++;
      evt_save();
    }
    webServer.sendHeader("Location", "/calendar");
    webServer.send(303);
  });

  webServer.on("/evt_del", HTTP_POST, []() {
    int i = webServer.arg("i").toInt();
    if (i >= 0 && i < g_evt_count) {
      for (uint8_t k = (uint8_t)i; k + 1 < g_evt_count; k++) g_events[k] = g_events[k + 1];
      g_evt_count--;
      evt_save();
    }
    webServer.sendHeader("Location", "/calendar");
    webServer.send(303);
  });

  webServer.on("/docappts", HTTP_GET, []() {
    String page = html_header("Doktersafsprake / Doctor Appointments");
    page += "<h2 style='color:#00CFCF'>&#128203; Doktersafsprake</h2>";
    page += "<p style='color:#aaa'>Voer tot 10 afsprake in. Verlede afsprake word outomaties verwyder.</p>";
    page += "<form method='POST' action='/doc_save'>";
    page += "<table style='width:100%;border-collapse:collapse'>";
    page += "<tr><th style='text-align:left;color:#00CFCF;padding:4px'>#</th>";
    page += "<th style='text-align:left;color:#00CFCF;padding:4px'>Beskrywing (Dokter/Kliniek)</th>";
    page += "<th style='text-align:left;color:#00CFCF;padding:4px'>Dag</th>";
    page += "<th style='text-align:left;color:#00CFCF;padding:4px'>Maand</th></tr>";
    for (uint8_t i = 0; i < DOC_MAX; i++) {
      String nm = (i < g_doc_count) ? String(g_docs[i].desc)  : "";
      String dd = (i < g_doc_count) ? String(g_docs[i].day)   : "";
      String mm = (i < g_doc_count) ? String(g_docs[i].month) : "";
      String bg = (i % 2 == 0) ? "#1a1a2e" : "#16213e";
      page += "<tr style='background:" + bg + "'>";
      page += "<td style='padding:4px;color:#888'>" + String(i+1) + "</td>";
      page += "<td style='padding:2px'><input name='n" + String(i) + "' value='" + nm + "' maxlength='15' style='width:150px;background:#222;color:#fff;border:1px solid #444;padding:3px' placeholder='bv. Dr Botha'></td>";
      page += "<td style='padding:2px'><input name='d" + String(i) + "' type='number' min='1' max='31' value='" + dd + "' style='width:55px;background:#222;color:#fff;border:1px solid #444;padding:3px'></td>";
      page += "<td style='padding:2px'><input name='m" + String(i) + "' type='number' min='1' max='12' value='" + mm + "' style='width:55px;background:#222;color:#fff;border:1px solid #444;padding:3px'></td>";
      page += "</tr>";
    }
    page += "</table><br>";
    page += "<input type='submit' value='Stoor Afsprake' style='background:#00CFCF;color:#000;padding:8px 20px;border:none;font-weight:bold;cursor:pointer'>";
    page += "</form>";
    page += "<hr style='border-color:#333;margin:16px 0'>";
    // Status
    if (g_doc_today && g_doc_today_idx < g_doc_count) {
      page += "<p style='color:#00CFCF;font-size:1.2em'>&#128680; Vandag: <b>";
      page += String(g_docs[g_doc_today_idx].desc);
      page += "</b> afspraak!</p>";
    }
    // Upcoming
    page += "<p style='color:#aaa'>Komende afsprake:</p><ul>";
    bool any = false;
    for (uint8_t i = 0; i < g_doc_count; i++) {
      int16_t d = days_until_bday(g_docs[i].day, g_docs[i].month);
      if (d >= 0 && d <= 30) {
        String when = (d == 0) ? "VANDAG!" : (d == 1) ? "More" : "Oor " + String(d) + " dae";
        page += "<li style='color:#00CFCF'>" + String(g_docs[i].desc) + " — " + String(g_docs[i].day) + "/" + String(g_docs[i].month) + " (" + when + ")</li>";
        any = true;
      }
    }
    if (!any) page += "<li style='color:#888'>Geen afsprake in die volgende 30 dae nie.</li>";
    page += "</ul>";
    page += html_footer();
    webServer.send(200, "text/html", page);
  });

  webServer.on("/doc_save", HTTP_POST, []() {
    g_doc_count = 0;
    for (uint8_t i = 0; i < DOC_MAX; i++) {
      String nm = webServer.arg("n" + String(i));
      String ds = webServer.arg("d" + String(i));
      String ms = webServer.arg("m" + String(i));
      nm.trim();
      if (nm.length() == 0 || ds.length() == 0 || ms.length() == 0) continue;
      uint8_t d = (uint8_t)ds.toInt();
      uint8_t m = (uint8_t)ms.toInt();
      if (d < 1 || d > 31 || m < 1 || m > 12) continue;
      g_docs[g_doc_count].day   = d;
      g_docs[g_doc_count].month = m;
      strncpy(g_docs[g_doc_count].desc, nm.c_str(), 15);
      g_docs[g_doc_count].desc[15] = '\0';
      g_doc_count++;
      if (g_doc_count >= DOC_MAX) break;
    }
    doc_save();
    webServer.sendHeader("Location", "/docappts");
    webServer.send(303);
  });

  // ── Security alarm web routes ─────────────────────────────────────────────
  // Bookmark http://<clock-ip>/alarm on your phone home screen
  webServer.on("/alarm", HTTP_GET, []() { sec_alarm_web_page(); });

  // Pushover self-test — sends a real test message
  // SYNCHRONOUSLY (we're already on Core 0 here) and shows the result of the
  // attempt right in the browser. Diagnoses the clock→Pushover leg with one
  // tap from the phone: no serial monitor, no siren, no alarm involved.
  webServer.on("/potest", HTTP_GET, []() {
    po_send("Toets / Test", "Pushover toets from Ouma Ria Smart Clock\nTyd: " + po_time(), 0);
    String page = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>"
      "<style>body{background:#111;color:#eee;font-family:sans-serif;padding:20px}"
      ".r{background:#1a1a1a;border:1px solid #444;border-radius:8px;padding:14px;margin:14px 0;font-size:1.1rem}"
      "a{color:#0af}</style></head><body>"
      "<h2>Pushover Toets</h2>"
      "<div class='r'><b>Resultaat:</b><br>" + g_po_last + "</div>"
      "<p>'HTTP/1.1 200 OK' = Pushover het die boodskap AANVAAR — dit moet nou op die foon wees.<br>"
      "'4xx' = Pushover het dit VERWERP (token/sleutel probleem).<br>"
      "'KONNEKSIE MISLUK' = die klok kon nie by api.pushover.net uitkom nie.</p>"
      "<p><a href='/potest'>Toets weer</a> &nbsp;|&nbsp; <a href='/alarm'>Alarm bladsy</a> &nbsp;|&nbsp; <a href='/'>Tuis</a></p>"
      "</body></html>";
    webServer.send(200, "text/html", page);
  });

  // Zone 2 — ARM (away mode) — POST and GET both work for phone bookmarks
  webServer.on("/alarm/arm", HTTP_POST, []() {
    sec_zone2_arm();
    webServer.sendHeader("Location", "/alarm");
    webServer.send(303);
  });
  webServer.on("/alarm/arm", HTTP_GET, []() {
    sec_zone2_arm();
    webServer.sendHeader("Location", "/alarm");
    webServer.send(303);
  });

  // Zone 2 — DISARM (also silences siren from either zone)
  // ── /alarm/dismiss : stop wake alarm from web (no physical buttons needed) ──
  webServer.on("/alarm/dismiss", HTTP_GET, []() {
    g_alm_fire = false;
    g_wekker_ringing = false;  // must be cleared here too, or the Wekker repeat
                               // loop keeps replaying the sound after a web dismiss
    g_force_redraw = true;   // ensure display updates immediately, not after 100ms
    dfplayer_stop();
    String page = "<!DOCTYPE html><html><head>"
      "<meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>"
      "<title>Alarm Dismissed</title>"
      "<style>body{font-family:sans-serif;background:#111;color:#0c0;text-align:center;padding:60px 20px}</style>"
      "</head><body><h2>&#9989; Alarm dismissed</h2></body></html>";
    webServer.send(200, "text/html", page);
  });

  webServer.on("/alarm/disarm", HTTP_POST, []() {
    sec_zone2_disarm();
    webServer.sendHeader("Location", "/alarm");
    webServer.send(303);
  });
  webServer.on("/alarm/disarm", HTTP_GET, []() {
    sec_zone2_disarm();
    webServer.sendHeader("Location", "/alarm");
    webServer.send(303);
  });

  // Zone 1 — suspend/resume courtyard sensors
  webServer.on("/alarm/z1off", HTTP_POST, []() {
    request_beep(BEEP_SINGLE);   // single beep = courtyard paused — deferred
    g_z1_active = false;
    Serial.println("Zone 1 (courtyard) SUSPENDED");
    // Silence siren if Zone 1 triggered it
    if (g_sec_triggered && g_trig_zone == 1) {
      g_sec_triggered  = false;
      g_siren_start_ms = 0;
      g_trig_zone      = 0;
      g_trig_sensor    = 0;
      noTone(SIREN_PIN);
      digitalWrite(SIREN_PIN, LOW);
      Serial.println("Zone 1 siren silenced by pause");
    }
    po_notify("Courtyard Sensors PAUSED",
      "Zone 1 sensors suspended.\nSiren silenced if it was active.\nRemember to resume!\nTime: " + po_time(), 0);
    webServer.sendHeader("Location", "/alarm");
    webServer.send(303);
  });

  webServer.on("/alarm/z1on", HTTP_POST, []() {
    request_beep(BEEP_DOUBLE);   // double beep = courtyard resumed — deferred
    g_z1_active = true;
    Serial.println("Zone 1 (courtyard) ENABLED");
    po_notify("Courtyard Sensors RESUMED",
      "Zone 1 courtyard sensors active again.\nCourtyard is now protected.\nTime: " + po_time(), 0);
    webServer.sendHeader("Location", "/alarm");
    webServer.send(303);
  });

  // Panic / Emergency button — immediate siren, no delay
  webServer.on("/alarm/panic", HTTP_POST, []() {
    sec_panic();
    webServer.sendHeader("Location", "/alarm");
    webServer.send(303);
  });
  webServer.on("/alarm/panic", HTTP_GET, []() {
    sec_panic();
    webServer.sendHeader("Location", "/alarm");
    webServer.send(303);
  });
}


/******************************************************************************
 * BRIGHTNESS CONTROL — two independent brightness values
 ******************************************************************************/
// THE PROBLEM THIS SOLVES: FastLED.setBrightness() is GLOBAL — one call
// scales every strip registered with FastLED.addLeds() identically. The
// MAIN matrix display (leds[]) and the small status BAR3 chain
// (pills/meds/status, bar3_leds[]) were sharing that single global value,
// so fixing one to a schedule automatically froze the other too.
//
// THE FIX: each strip gets its OWN brightness variable, applied
// independently via FastLED[0].showLeds()/FastLED[1].showLeds() in
// leds_show_all() — see that function for the actual per-strip push.
//
//   g_bright      → MAIN DISPLAY  → still BH1750 lux-driven, exactly as
//                    before this morning's change. Dims naturally with
//                    actual room light, same behaviour Henry has always had
//                    for the big display.
//   g_bar3_bright → BAR3 (pills/meds/status) → fixed day/night SCHEDULE
//                    (NIGHT_START/NIGHT_END, same pattern as g_vol/
//                    g_night_vol), NOT the light sensor. This is the part
//                    that changed: winter mornings at 07:00 are still dark
//                    outside but "daytime" by clock, and the sensor would
//                    incorrectly dim these status LEDs right when they need
//                    to be clearly visible. A fixed day value sidesteps
//                    that entirely — no more torch-on-sensor surprises
//                    either, since BAR3 no longer reads the sensor at all.
void update_brightness_smooth(void) {
  static float    smoothLux   = 100.0f;
  static uint32_t lastLuxRead = 0;
  static uint32_t lastFade    = 0;

  uint32_t now = millis();

  // ── Main display target — BH1750 lux curve (unchanged from before) ──────
  if(now - lastLuxRead >= 250) {
    lastLuxRead = now;
    float lux_f = lightMeter.readLightLevel();
    if(lux_f >= 0 && lux_f < 65000.0f) {
      smoothLux = (smoothLux * 0.70f) + (lux_f * 0.30f);
      float mapped = 5.0f +
        (constrain(smoothLux, 0.0f, 1000.0f) / 1000.0f) * 250.0f;
      g_bright_target = (uint8_t)mapped;
    }
  }

  // ── BAR3 target — fixed day/night schedule (the new behaviour) ──────────
  // +5% on the status chain per Henry — the pills/meds/
  // status LEDs render a touch brighter than the configured day/night values.
  // Applied here as a scale (not to the config defaults) so it works on top
  // of whatever brday/brnight are set to in /config, survives NVS, and the
  // main matrix stays purely BH1750 lux-driven as before.
  {
    uint16_t t = is_night_now() ? g_bright_night : g_bright_day;
    t = (t * 105) / 100;              // +5%
    if (t > 255) t = 255;
    g_bar3_bright_target = (uint8_t)t;
  }

  // Smooth fade ±1 every 20ms for BOTH — gentle transitions, no jumps at
  // sunrise/sunset-equivalent boundaries or right after boot.
  if(now - lastFade >= 20) {
    lastFade = now;
    if(g_bright < g_bright_target) g_bright++;
    else if(g_bright > g_bright_target) g_bright--;
    if(g_bar3_bright < g_bar3_bright_target) g_bar3_bright++;
    else if(g_bar3_bright > g_bar3_bright_target) g_bar3_bright--;
  }
}

/******************************************************************************
 * LED drawing helpers
 ******************************************************************************/

// ── LED pixel address for serpentine 8x8 panel ──────────────────────────────
// Standard WS2812B 8x8 matrix: row 0 at top, serpentine wiring
// Even rows (0,2,4,6): left to right  (col 0=left)
// Odd  rows (1,3,5,7): right to left  (col 0=right)
// IMPORTANT: if your display is mirrored/rotated, swap the serpentine below

uint16_t pix(uint8_t panel, uint8_t col, uint8_t row) {
  // FINAL CONFIRMED mapping — Oupa's digit test v6 + mirror fix
  // No serpentine, horizontal mirror: pos = row*8 + (7-col)
  //return (uint16_t)panel * 64 + (uint16_t)row * 8 + (uint16_t)(7 - col);
   // Rotate display 180 degrees
  return (uint16_t)panel * 64 +
         (uint16_t)(7 - row) * 8 +
         (uint16_t)col;
}

void fb_clear(void) { fill_solid(leds, NUM_LEDS, CRGB::Black); }

// Draw one 5x7 font glyph on a panel
// Font columns are vertical strips, font rows are horizontal bits
// Glyph is centred: 1 pixel margin on left, 1 on right, 0 top, 1 bottom
void draw_dg(uint8_t panel, uint8_t digit, uint8_t r, uint8_t g, uint8_t b) {
  if(digit >= 18) return;
  CRGB col(r, g, b);
  // Clear the panel first
  for(uint8_t c = 0; c < 8; c++)
    for(uint8_t rw = 0; rw < 8; rw++) {
      uint16_t idx = pix(panel, c, rw);
      if(idx < NUM_LEDS) leds[idx] = CRGB::Black;
    }
  // Draw 5x7 glyph
  // display_col = fx+1 (1px left margin, cols 1-5)
  // display_row = fy+1 (1px top margin, rows 1-7)
  // bit (6-fy): bit6=top row, bit0=bottom row
  for(uint8_t fx = 0; fx < 5; fx++) {
    uint8_t col_data = FONT5[digit][fx];
    for(uint8_t fy = 0; fy < 7; fy++) {
      uint8_t  display_col = fx + 1;
      uint8_t  display_row = fy + 1;
      uint16_t idx = pix(panel, display_col, display_row);
      if(idx < NUM_LEDS) {
        leds[idx] = (col_data & (1 << (6 - fy))) ? col : CRGB::Black;
      }
    }
  }
}

// Colon — two dots on panel 2 left edge (col 0)
// Rows 2 and 5 give a nice spread in the 7-row font height
// Panel 2 col 0 is the natural centre of the visual gap
void draw_colon_dots(bool on) {

  // Original bright blue colon
  CRGB c = on ? CRGB(0, 0, 255) : CRGB::Black;

  // After rotation fix:
  // panel 1 col 7 becomes visual center gap
  uint16_t dot1 = pix(1, 7, 5);
  uint16_t dot2 = pix(1, 7, 2);

  if(dot1 < NUM_LEDS) leds[dot1] = c;
  if(dot2 < NUM_LEDS) leds[dot2] = c;
}
//void draw_colon_dots(bool on) {
 // CRGB c = on ? CRGB(200, 200, 200) : CRGB::Black;
  // pix() mirrors col: col=0 = rightmost physical LED
  // Colon on right edge of panel 1 = col 0
  // Rows 2 and 5 = evenly spread upper and lower dots
 // uint16_t dot1 = pix(1, 0, 5);
 // uint16_t dot2 = pix(1, 0, 2);
  //if(dot1 < NUM_LEDS) leds[dot1] = c;
 // if(dot2 < NUM_LEDS) leds[dot2] = c;
//}

/******************************************************************************
 * Scroll text helper (reused from the Nano build, no-PROGMEM version)
 ******************************************************************************/

static void splash_draw_col(uint8_t vc, uint8_t glyph, uint8_t glyph_col) {
  if(vc >= 32 || glyph >= 18) return;
  uint8_t panel    = vc / 8;
  uint8_t pc       = (vc % 8) + 1;  // col 1-6 within panel
  if(pc > 7) pc = 7;
  uint8_t col_data = FONT5[glyph][glyph_col];
  for(uint8_t row = 0; row < 7; row++) {
    uint16_t idx = pix(panel, pc, row + 1);
    if(idx < NUM_LEDS)
      leds[idx] = (col_data & (1 << (6-row))) ? CRGB(180,180,180) : CRGB::Black;
  }
}

static void splash_clear_col(uint8_t vc) {
  if(vc >= 32) return;
  uint8_t panel = vc / 8;
  uint8_t pc    = (vc % 8) + 1;
  if(pc > 7) pc = 7;
  for(uint8_t row = 0; row < 8; row++) {
    uint16_t idx = pix(panel, pc, row);
    if(idx < NUM_LEDS) leds[idx] = CRGB::Black;
  }
}

// Scroll any array of glyph indices across the display
void scroll_glyphs(const uint8_t* glyphs, uint8_t n, uint16_t speed_ms = 50) {
  const uint8_t CHAR_W    = 6;
  uint8_t total_cols = n * CHAR_W;
  fb_clear();
  FastLED.setBrightness(96);  // NOTE: no-op since leds_show_all() now uses per-controller
                               // showLeds(g_bright)/(g_bar3_bright) — global setBrightness()
                               // is bypassed. Splash brightness is effectively whatever
                               // g_bright currently is. Harmless, left as-is.
  for(int16_t offset = 0; offset < (int16_t)(total_cols + 32); offset++) {
    fb_clear();
    for(uint8_t vc = 0; vc < 32; vc++) {
      int16_t src = (int16_t)vc + offset - 32;
      if(src < 0 || src >= (int16_t)total_cols) { splash_clear_col(vc); continue; }
      uint8_t gi = (uint8_t)(src / CHAR_W);
      uint8_t gc = (uint8_t)(src % CHAR_W);
      if(gc < 5) splash_draw_col(vc, glyphs[gi], gc);
      else       splash_clear_col(vc);
    }
    leds_show_all();
    delay(speed_ms);
  }
}

void splash_screen(void) {
  // Scrolls "V <SPLASH_MAJOR>.<SPLASH_MINOR>" — e.g. "V 2.5" (UPDATED 2026-07-03:
  // Now driven by the SPLASH_MAJOR/MINOR defines next to FIRMWARE_VERSION so
  // the two can't drift apart again. Digits 0-9 index FONT5 directly.)
  /* dead code removed 2026-07-03 — unused leftover from an earlier splash:
  const uint8_t glyphs[] = {
    11,                    // space
    14, 11,                // E S (we use dash/space placeholders — simplified)
    1, 13, 0,              // 1 . 0
    11                     // space
  };
  */
  const uint8_t gl2[] = {11, 12, SPLASH_MAJOR, 13, SPLASH_MINOR, 11};  // space V <maj> . <min> space
  scroll_glyphs(gl2, 6, 55);
  FastLED.setBrightness(g_bright);
}

/******************************************************************************
 * Factory reset — hold BTN_SET + BTN_UP at boot for 3 seconds
 ******************************************************************************/

void check_factory_reset(void) {
  if(digitalRead(BTN_SET) != LOW || digitalRead(BTN_UP) != LOW) return;
  uint8_t held = 0;
  for(uint8_t i = 0; i < 30; i++) {
    if(digitalRead(BTN_SET) != LOW || digitalRead(BTN_UP) != LOW) {
      fb_clear(); leds_show_all(); return;
    }
    uint8_t br = (i < 15) ? i * 12 : (30 - i) * 12;
    fill_solid(leds, NUM_LEDS, CRGB(0, 0, br + 20));
    FastLED.setBrightness(120); leds_show_all();  // setBrightness here is a no-op (see leds_show_all notes); colour value already encodes the pulse
    delay(100);
    held++;
  }
  if(held < 30) return;
  factory_reset_defaults();
  for(uint8_t f = 0; f < 4; f++) {
    fill_solid(leds, NUM_LEDS, (f & 1) ? CRGB::Black : CRGB(0,160,0));
    leds_show_all(); delay(200);
  }
  fb_clear(); leds_show_all();
  delay(500);
  ESP.restart();
}

/******************************************************************************
 * Button reading — returns bitmask of just-pressed buttons (edge detect)
 ******************************************************************************/

// Button hold-to-repeat: returns 1 if this press/hold should fire an event
// FIX 6 (2026-07-03): the first-call branch used to "return true" — but the
// handlers are written as (edge || (held && do_repeat())), and the press EDGE
// has ALREADY fired the action on the previous scan. do_repeat()'s first call
// (10ms later, button still held) then fired it a SECOND time — every single
// tap of SET/UP/CVUP/CVDN stepped by 2 and played the confirm ding twice.
// First call now just registers the button and returns false; the edge owns
// the first fire, do_repeat() owns the repeats after the 700ms hold delay.
bool do_repeat(uint8_t bmask) {
  if(g_hold_btn != bmask) {
    g_hold_btn = bmask;
    g_hold_ms  = 0;
    return false;  // edge already fired this press — don't fire again
  }
  g_hold_ms += 10;
  if(g_hold_ms < 700) return false;             // 700ms initial delay
  uint16_t interval = (bmask & (BMASK_CVUP|BMASK_CVDN)) ? 333 : 150;
  return (g_hold_ms % interval) < 10;
}

// Increment helpers
void do_increment_alm(bool is_hh) {
  if(is_hh) { g_alarm1.hh = (g_alarm1.hh + 1) % 24; }
  else       { g_alarm1.mm = (g_alarm1.mm + 1) % 60; }
  settings_save();
}

void do_increment_vol(bool up) {
  if(is_night_now()) {
    if(up && g_night_vol < 30) g_night_vol++;
    else if(!up && g_night_vol > 0) g_night_vol--;
  } else {
    if(up && g_vol < 30) g_vol++;
    else if(!up && g_vol > 0) g_vol--;
  }
}

uint16_t read_buttons_raw(void) {
  uint16_t raw = 0;
  if(digitalRead(BTN_MODE)   == LOW) raw |= BMASK_MODE;
  if(digitalRead(BTN_SET)    == LOW) raw |= BMASK_SET;
  if(digitalRead(BTN_UP)     == LOW) raw |= BMASK_UP;
  if(digitalRead(BTN_ALM)    == LOW) raw |= BMASK_ALM;
  if(digitalRead(BTN_CHIME)  == LOW) raw |= BMASK_CHIME;
  if(digitalRead(BTN_CVUP)   == LOW) raw |= BMASK_CVUP;
  if(digitalRead(BTN_CVDN)   == LOW) raw |= BMASK_CVDN;
  if(digitalRead(BTN_SOUND)  == LOW) raw |= BMASK_SOUND;   // needs external pullup
  if(digitalRead(BTN_ALMSEL) == LOW) raw |= BMASK_ALMSEL;  // needs external pullup
  return raw;
}

static uint32_t mode_hold_start = 0;
static bool mode_long_fired = false;

void check_inputs(void) {
  // Settle window: the first call arms a timer BUTTON_SETTLE_MS in the
  // future, measured from when button scanning actually starts (not from
  // power-on, which may already be several seconds earlier if WiFi setup
  // blocked first) — every GPIO gets this long to properly settle through
  // its pull-up before any hold/long-press logic is allowed to trust it.
  static uint32_t settle_until = 0;
  if (settle_until == 0) settle_until = millis() + BUTTON_SETTLE_MS;
  if (!TIME_REACHED(millis(), settle_until)) return;   // rollover-safe (was millis() < settle_until)

  uint16_t raw  = read_buttons_raw();
  uint16_t edge = raw & ~btn_prev;   // newly pressed this scan
  uint16_t held = raw & btn_prev;    // still held

  if(!raw) {
    g_hold_btn = 0;
    g_hold_ms  = 0;
    mode_hold_start = 0;
    mode_long_fired = false;
  }

  // Any button = exit rainbow idle
  if(raw) {
    g_last_interaction_ms = millis();
    g_rainbow_active = false;
  }

  // ── BTN_MODE long-press detection ────────────────────────────────────────
  if(raw & BMASK_MODE) {
    if(mode_hold_start == 0) mode_hold_start = millis();
    if(!mode_long_fired && (millis() - mode_hold_start > 1000)) {
      // Long press: toggle °C/°F temperature display
      g_temp_degF = !g_temp_degF;
      settings_save();
      mode_long_fired = true;
      // Flash display white briefly to confirm
      fill_solid(leds, NUM_LEDS, CRGB(80,80,80));
      leds_show_all(); delay(150);
      // (2026-08-XX): MODE previously gave NO audio feedback at all —
      // felt "dead" even though it was working (just silent). Confirm ding
      // added here and on every other button below that had none.
      dfplayer_volume(is_night_now() ? g_night_vol : g_vol);
      dfplayer_play(14);
    }
  } else if(edge & BMASK_MODE) {
    // Short press released and no long press fired
  }

  // BTN_MODE short press: only cycle mode if NOT long-press
  if((edge & BMASK_MODE) && !mode_long_fired) {
    g_mode = (g_mode + 1) % 6;
    dfplayer_volume(is_night_now() ? g_night_vol : g_vol);   // Was silent
    dfplayer_play(14);
  }

  // ── Egg timer done — any button clears it early ──────────────────────────
  if(g_egg_done && edge) {
    g_egg_done = false;
    dfplayer_stop();
  }

  // ── Dismiss alarm / snooze — REWRITTEN 2026-07-03 (FIX 2 + FIX 3) ─────────
  // WHY THE OLD CODE COULDN'T SNOOZE (kept commented below):
  //  (a) The dismiss branch fired on the press EDGE — the alarm was already
  //      dismissed on the very first 10ms scan of the press, before any hold
  //      time could accumulate. Hold-to-snooze could never win the race.
  //  (b) Even without (a): this block ends in "return", which skipped the
  //      g_hold_ms += 10 accumulator at the bottom of check_inputs() — so
  //      g_hold_ms stayed 0 during an alarm and "g_hold_ms > 500" was
  //      unreachable anyway. Snooze was doubly dead.
  //  (c) The early return also skipped "btn_prev = raw", so edge was
  //      recomputed as "newly pressed" on EVERY scan while held.
  // NEW BEHAVIOUR: we timestamp the UP press ourselves.
  //   * Hold BTN_UP  > 500ms  → SNOOZE (9 min), purple flash confirm.
  //   * Release      ≤ 500ms  → DISMISS (fires on release, not press, so a
  //                             short press can no longer pre-empt the hold).
  // Dismissing/snoozing here ALSO clears g_wekker_ringing, not just
  // g_alm_fire — the two are separate flags (Alarm and Wekkers share
  // g_alm_fire for status/chime purposes, but each has its own repeat
  // logic), so both must be cleared or the Wekker repeat loop keeps going.
  if(g_alm_fire) {
    static uint32_t up_press_ms = 0;

    if(edge & BMASK_UP) up_press_ms = millis();   // press started — start timing

    // Held past the threshold → SNOOZE (only once per press)
    if((raw & BMASK_UP) && up_press_ms != 0 &&
       (millis() - up_press_ms > 500) && !g_snooze_active) {
      g_alm_fire       = false;
      g_wekker_ringing = false;   // stop Wekker repeats too
      g_snooze_active  = true;
      dfplayer_stop();
      // Calculate snooze time
      g_snooze_mm = g_mm + SNOOZE_MINUTES;
      g_snooze_hh = g_hh;
      if(g_snooze_mm >= 60) { g_snooze_mm -= 60; g_snooze_hh = (g_snooze_hh + 1) % 24; }
      // Flash purple for snooze confirm
      fill_solid(leds, NUM_LEDS, CRGB(80, 0, 120));
      leds_show_all(); delay(300);
      up_press_ms = 0;
    }

    // Released within the threshold → DISMISS
    if(!(raw & BMASK_UP) && (btn_prev & BMASK_UP) && up_press_ms != 0 &&
       (millis() - up_press_ms <= 500)) {
      g_alm_fire       = false;
      g_wekker_ringing = false;   // stop Wekker repeats too
      dfplayer_stop();
      up_press_ms = 0;
    }

    btn_prev = raw;   // (c) keep edge detection sane while the alarm rings
    return;           // swallow all other buttons while alarm fires
  }
  /* ── ORIGINAL dismiss/snooze code, replaced 2026-07-03 — for reference ─────
  if(g_alm_fire) {
    if(edge & BMASK_UP) {
      // Check if held > 500ms = snooze; short press = dismiss
      // We detect via hold: start counting in held
    }
    if((held & BMASK_UP) && g_hold_ms > 500 && !g_snooze_active) {
      // SNOOZE
      g_alm_fire     = false;
      g_snooze_active = true;
      dfplayer_stop();
      // Calculate snooze time
      g_snooze_mm = g_mm + SNOOZE_MINUTES;
      g_snooze_hh = g_hh;
      if(g_snooze_mm >= 60) { g_snooze_mm -= 60; g_snooze_hh = (g_snooze_hh + 1) % 24; }
      // Flash purple for snooze confirm
      fill_solid(leds, NUM_LEDS, CRGB(80, 0, 120));
      leds_show_all(); delay(300);
    }
    if((edge & BMASK_UP) && !g_snooze_active) {
      g_alm_fire = false;
      dfplayer_stop();
    }
    return; // swallow all other buttons while alarm fires
  }
 ── end original ─────────────────────────────────────────────────────────── */

  // ── BTN_SET ──────────────────────────────────────────────────────────────
  // Was completely silent — pressing SET while in clock mode (g_mode 0,
  // 4, or 5) did nothing at all, not even a beep, so it felt broken even
  // though it was working correctly (SET simply has no job in those modes).
  // Ding now confirms every press was received, same pattern already used by
  // CVUP/CVDN below.
  if((edge & BMASK_SET) || ((held & BMASK_SET) && do_repeat(BMASK_SET))) {
    if(g_mode == 1)      { do_increment_alm(true); }
    else if(g_mode == 2) { // start / pause egg timer
      if(edge & BMASK_SET) {
        if(g_egg_run) g_egg_run = false;
        else { g_egg_run = true; g_egg_mm = g_egg_set_mm; g_egg_ss = g_egg_set_ss; }
      }
    }
    else if(g_mode == 3) {
      g_egg_set_mm = (g_egg_set_mm + 1) % 100;
      settings_save();
    }
    dfplayer_volume(is_night_now() ? g_night_vol : g_vol);
    dfplayer_play(14);
  }

  // ── BTN_UP ───────────────────────────────────────────────────────────────
  // Same fix as SET above — was silent outside modes 1 and 3.
  if((edge & BMASK_UP) || ((held & BMASK_UP) && do_repeat(BMASK_UP))) {
    if(g_mode == 1)      { do_increment_alm(false); }
    else if(g_mode == 3) {
      g_egg_set_ss = (g_egg_set_ss + 1) % 60;
      settings_save();
    }
    dfplayer_volume(is_night_now() ? g_night_vol : g_vol);
    dfplayer_play(14);
  }

  // ── BTN_ALM: toggle the alarm on/off ──────────────────────────────────────
  if(edge & BMASK_ALM) {
    g_alarm1.enabled = !g_alarm1.enabled;
    settings_save();
    dfplayer_volume(is_night_now() ? g_night_vol : g_vol);   // Was silent
    dfplayer_play(14);
  }

  // ── BTN_ALMSEL: manually triggers the rainbow display (Henry's request) ──
  // Only turns rainbow ON — any OTHER button already exits it via the
  // "any button = exit rainbow" block above, so pressing anything else
  // switches back to the normal clock display. Only fires from the normal
  // clock display, and never while an alarm is actively sounding.
  if(edge & BMASK_ALMSEL) {
    if (g_mode == 0 && !g_alm_fire) g_rainbow_active = true;
    dfplayer_volume(is_night_now() ? g_night_vol : g_vol);
    dfplayer_play(14);
  }

  // ── BTN_CHIME ────────────────────────────────────────────────────────────
  if(edge & BMASK_CHIME) {
    g_chime_en = !g_chime_en;
    settings_save();
    dfplayer_volume(is_night_now() ? g_night_vol : g_vol);   // Was silent
    dfplayer_play(14);
  }

  // ── BTN_CVUP / BTN_CVDN ──────────────────────────────────────────────────
  if((edge & BMASK_CVUP) || ((held & BMASK_CVUP) && do_repeat(BMASK_CVUP))) {
    do_increment_vol(true);
    dfplayer_volume(is_night_now() ? g_night_vol : g_vol);
    dfplayer_play(14);
    settings_save();
  }
  if((edge & BMASK_CVDN) || ((held & BMASK_CVDN) && do_repeat(BMASK_CVDN))) {
    do_increment_vol(false);
    dfplayer_volume(is_night_now() ? g_night_vol : g_vol);
    dfplayer_play(14);
    settings_save();
  }

  // ── BTN_SOUND: cycle sound set ───────────────────────────────────────────
  if(edge & BMASK_SOUND) {
    g_sound_set  = (g_sound_set + 1) % 5;
    g_show_timer = 2;
    g_show_set   = true;
    play_chime_with_vol(chime_track_for_set(g_sound_set, 0));
    settings_save();
  }

  // Update hold tracking for repeating buttons
  // FIX 6b (2026-07-03): g_hold_ms was incremented BOTH here and inside
  // do_repeat() — double-counting made the 700ms initial hold delay actually
  // ~350ms and halved the repeat intervals. do_repeat() is called every 10ms
  // scan while the button is held, so its own += 10 is the correct single
  // count; this one is removed. The else-reset stays.
  if(held & (BMASK_SET|BMASK_UP|BMASK_CVUP|BMASK_CVDN)) { /* g_hold_ms += 10; ← moved into do_repeat() only */ }
  else { g_hold_btn = 0; g_hold_ms = 0; }

  btn_prev = raw;
}

/******************************************************************************
 * Timekeeping update — called every loop pass
 ******************************************************************************/

void timekeeping_update(void) {
  uint32_t now_ms = millis();

  // Colon blink every 500ms
  if(now_ms - g_last_half_ms >= 500) {
    g_last_half_ms = now_ms;
    g_colon = !g_colon;
  }

  // 1-second tick
  if(now_ms - g_last_sec_ms >= 1000) {
    g_last_sec_ms = now_ms;

    // Software increment (RTC re-syncs every 30s so drift is near zero)
    if(++g_ss >= 60) {
      g_ss = 0;
      if(++g_mm >= 60) {
        g_mm = 0;
        if(++g_hh >= 24) { g_hh = 0; rtc_sync(); } // force sync at midnight
      }
    }

    // ── MINUTE-CHANGE LATCH — fixes the ss==0 fragility ───────────────────
    // Every alarm/wekker/snooze check and the midnight resets used to demand
    // g_ss == 0 EXACTLY. Any blocking delay straddling that second (600ms
    // button flash, smoker beeps) or the 30s RTC resync nudging g_ss past 0
    // silently skipped the event — worst case: midnight second 0 missed →
    // g_wekker_fired_today[] never reset → ALL Wekkers dead for the whole
    // day. This latch fires exactly once per wall-clock minute regardless of
    // which second we land on, including minute changes that arrive via
    // rtc_sync(). All once-per-minute logic below now keys off g_new_minute.
    //
    // ── Boot-time double-fire fix ──────────────────────────────────────────
    // g_alm_armed/g_alm_fire and g_wekker_fired_today[] all live in plain
    // RAM, not flash — a reboot always resets them, regardless of whether
    // that minute's alarm/wekker had already fired seconds before the
    // reboot. Previously last_min_handled always started at the 255
    // sentinel, forcing g_new_minute TRUE on literally the first tick after
    // every boot — correct for the case where the crash happened just
    // BEFORE a minute rolled over (nothing fired yet, so firing once now is
    // right), but WRONG for a reboot landing mid-minute, e.g. a brownout
    // cycling power once or twice right as loadshedding kicks in/out — an
    // event could already have fired seconds earlier in that same minute,
    // then fire AGAIN the instant the clock comes back up.
    // Fix: only trust "boot = fresh minute" when the RTC shows we're still
    // in the first couple of seconds of it — a clean "power died right at
    // the minute boundary" case, matching the review's own example
    // (07:59:59 loses power, 08:00:02 boot). Otherwise assume we booted
    // into the MIDDLE of a minute whose events may already have run before
    // the crash, and mark that minute as already-handled so nothing fires
    // a second time. Trade-off, chosen deliberately to match the review's
    // stated priority: this can rarely cause a genuinely-missed event (if
    // the crash landed a few seconds before its target time and the reboot
    // took a few seconds too) but can never cause a DUPLICATE fire, which
    // is the worse failure mode for a bug lasting years unattended.
    {
      static uint8_t last_min_handled = 255;
      static bool     s_min_latch_init = false;
      if (!s_min_latch_init) {
        s_min_latch_init  = true;
        last_min_handled  = (g_ss < 3) ? 255 : g_mm;   // see comment above
      }
      g_new_minute = (g_mm != last_min_handled);
      if (g_new_minute) last_min_handled = g_mm;
    }

    // Re-arm alarms at start of each new minute
    if(g_new_minute) g_alm_armed = true;

    // Temperature flip Henry's request: TWICE per
    // minute now (windows starting at ~ss 5 and ~ss 35), 8 seconds each.
    // Trigger is a >= window with a half-minute key instead of an exact
    // second match, so a skipped second (the old ss==5 fragility) just fires
    // one second later instead of not at all. g_last_flip_mm now stores the
    // half-minute key (mm*2 + half, 0-119) rather than the bare minute.
    // Only triggers in clock mode (mode 0), not during alarm/rainbow.
    {
      uint8_t half = (g_ss >= 35) ? 1 : ((g_ss >= 5) ? 0 : 255);
      if(half != 255 && g_mode == 0 && !g_alm_fire && !g_rainbow_active) {
        uint8_t key = g_mm * 2 + half;
        if(key != g_last_flip_mm) {
          g_temp_flip_active = true;
          g_temp_flip_end_ms = millis() + 8000;  // show for 8 seconds (was 6)
          g_last_flip_mm     = key;
        }
      }
    }

    // Pill reminder: check for new day at midnight and check alarm
    if(g_hh == 0 && g_mm == 0 && g_new_minute) pill_check_reset();   // was && g_ss == 0
    if(g_hh == 0 && g_mm == 0 && g_new_minute) med_check_reset();    // was && g_ss == 0
    pill_check_alarm();
    med_check_alarms();
    course_check();   // Kort Kursus — own day-rollover check inside, runs every second
    bday_check();
    doc_check();
    evt_check();   // Kalender events

    // Snooze check
    if(g_snooze_active && g_hh == g_snooze_hh && g_mm == g_snooze_mm && g_new_minute) {  // was && g_ss == 0
      g_snooze_active = false;
      g_alm_fire = true;
      g_alm_fire_ms = millis();
      dfplayer_volume(g_alarm_vol);
      dfplayer_play(1);
    }

    // Egg timer countdown
    if(g_egg_run) {
      if(g_egg_ss > 0) {
        g_egg_ss--;
      } else if(g_egg_mm > 0) {
        g_egg_mm--;
        g_egg_ss = 59;
      } else {
        // Egg timer reached zero!
        g_egg_run   = false;
        g_egg_done  = true;
        g_egg_done_ms = millis();
        g_egg_mm    = g_egg_set_mm;   // reset for next use
        g_egg_ss    = g_egg_set_ss;
        // Egg-done is a "must hear once" alert, so it follows the Config
        // "Alarm volume" — a soft day volume can't silently overcook the
        // eggs. (Smoker/braai reminders deliberately stay on ambient
        // day/night volume: they repeat every 30 min and can run overnight.)
        dfplayer_volume(g_alarm_vol);
        dfplayer_play(2); voice_lock();  // play alert sound — chime yields
        // Also push to the phone — Ria was outside in the garden and didn't
        // hear the audible alert. Plain priority 0: one notification, no
        // repeat-until-acknowledged needed for a kitchen timer.
        po_notify("Kombuis Tydhouer / Kitchen Timer",
          "Klaar! / Done!\nTyd: " + po_time(), 0);
      }
    }

    // Smoker Uptimer — wood/coal reminder beep every 30 minutes while running
    if(g_smoke_running && !g_smoke_paused) {
      uint32_t boundary = smoke_elapsed_sec() / 1800UL;   // which half-hour block
      if(boundary > 0 && boundary != g_smoke_last_reminder) {
        g_smoke_last_reminder = boundary;
        Serial.printf("Smoker: %lu min elapsed — wood/coal reminder beep\n", (unsigned long)(boundary * 30));
        dfplayer_volume(g_vol);
        dfplayer_play(14); delay(250);
        dfplayer_play(14); delay(250);
        dfplayer_play(14);
        po_notify("Smoker", "Hout/kole byvoeg? " + String((unsigned long)(boundary * 30)) + " min verstryk.", 0, "cosmic");
      }
    }

    // Braai Timer (Ouma's idea!) — turn reminder at the selected interval,
    // then a distinct "done" sound when the selected total time is reached.
    if(g_braai_enabled && g_braai_running && !g_braai_paused) {
      uint32_t elapsed = braai_elapsed_sec();
      if(elapsed < g_braai_total_sec) {
        uint32_t boundary = elapsed / (uint32_t)g_braai_turn_sec;   // which turn-interval block
        if(boundary > 0 && boundary != g_braai_last_turn) {
          g_braai_last_turn = boundary;
          Serial.printf("Braai: turn reminder at %lus elapsed\n", (unsigned long)elapsed);
          dfplayer_volume(g_vol);
          dfplayer_play(MED_TRACK); voice_lock();   // 0025.mp3 — confirmed ding-dong chime, not a
                                       // voice clip; nicer to hear repeatedly than a beep
          po_notify("Braai", "Draai die vleis om! / Turn the meat!", 0, "bike");
        }
      } else if(!g_braai_done_played) {
        // Total cook time reached — play a DIFFERENT sound than the turn cue
        // so Ouma can tell "turn it" apart from "it's done" by ear alone.
        g_braai_done_played = true;
        Serial.println("Braai: total time reached — done!");
        dfplayer_volume(g_vol);
        dfplayer_play(2);   // egg-timer alert track — distinct from the turn cue
        po_notify("Braai", "Klaar! / Done! Tyd is op.", 0, "magic");
        g_braai_running = false;
        settings_save();
      }
    }

    // Clear the voice lock once its minute has passed
    if(g_mm != g_voice_lock_mm) g_voice_lock_mm = 61;

    // Chime trigger (only on the xx:00 xx:15 xx:30 xx:45 beat)
    // && g_mm != g_voice_lock_mm — the chime now YIELDS in any minute
    // where a voice announcement played (medisyne on the hour, the hardcoded
    // 07:00/20:00 dokter reminders, the 00:00 birthday song). Previously the
    // chime request was consumed AFTER those plays in the same loop pass and
    // cut every one of them off after a few milliseconds.
    if(g_chime_en && (g_mm != g_chime_done_mm) && !g_alm_fire && (g_mm != g_voice_lock_mm)) {
      uint8_t quarter = 0xFF;
      if     (g_mm == 15) quarter = 0;
      else if(g_mm == 30) quarter = 1;
      else if(g_mm == 45) quarter = 2;
      else if(g_mm ==  0) quarter = 3;
      if(quarter != 0xFF) {
        g_chime_track   = quarter;
        g_chime_req     = true;
        g_chime_done_mm = g_mm;
      }
    }
    if(g_mm == 1) g_chime_done_mm = 61;

    // ── TEMPORARY DIAGNOSTIC (2026-06-23) — heartbeat so we can watch the
    // clock approach an alarm time live in the Serial Monitor, not just see
    // the result. Safe to remove once the alarm issue is resolved.
    Serial.printf("[tick] %02d:%02d:%02d  armed=%d  alm_fire=%d\n",
                   g_hh, g_mm, g_ss, g_alm_armed, g_alm_fire);

    // Alarm check — fires every day now, no day-of-week mask
    if(g_alm_armed && !g_alm_fire) {
      bool match_en   = g_alarm1.enabled;
      bool match_hh   = (g_hh == g_alarm1.hh);
      bool match_mm   = (g_mm == g_alarm1.mm);
      if(match_en && match_hh && match_mm && g_new_minute) {  // was && g_ss == 0
        g_alm_fire  = true;
        g_alm_fire_ms = millis();
        g_alm_armed = false;

        uint32_t t0 = millis();
        dfplayer_volume(g_alarm_vol);
        uint32_t t1 = millis();
        dfplayer_play(1);
        uint32_t t2 = millis();
        Serial.printf("[alarm] fired hh=%d mm=%d | volume took %lums | play took %lums\n",
                      g_hh, g_mm, (unsigned long)(t1 - t0), (unsigned long)(t2 - t1));
      }
    }

    // Wekkers — three independent daily alarms, each checked every second.
    // Repeats every WEKKER_REPEAT_MS until accepted (BTN_BDAY) or
    // WEKKER_MAX_REPEATS is reached, since the alert sound is short and
    // easy to miss on a single play. Midnight reset uses the once-per-minute
    // latch (g_new_minute), not an exact-second check, so a blocking delay
    // straddling midnight can never leave fired_today[] stuck — that would
    // silently kill every Wekker for the rest of the day.
    if(g_hh == 0 && g_mm == 0 && g_new_minute) {
      for(uint8_t i = 0; i < NUM_ALARM_SLOTS; i++) g_wekker_fired_today[i] = false;
    }
    if(!g_wekker_ringing) {
      // IMPORTANT: must use Sun=0..Sat=6 here, matching the bit layout the
      // /wekkers checkbox page uses (t2_days_names[] = Sun,Mon,...,Sat, bit d
      // = that index directly). The Monday=0 convention used elsewhere
      // (birthdays) does NOT match the Wekkers day-mask bits — using it here
      // would make a single ticked day silently fail to fire.
      uint8_t wk_dow = rtc_dow();   // Sunday=0 ... Saturday=6, matches g_wekker_days bit layout
      for(uint8_t i = 0; i < NUM_ALARM_SLOTS; i++) {
        bool wk_day_match = (g_wekker_days[i] >> wk_dow) & 1;
        if(g_wekker_enabled[i] && !g_wekker_fired_today[i] && wk_day_match
           && g_hh == g_wekker_hh[i] && g_mm == g_wekker_mm[i] && g_new_minute) {
          g_wekker_fired_today[i] = true;
          Serial.printf("[wekker %d] firing now (hh=%d mm=%d dow=%d)\n", i+1, g_hh, g_mm, wk_dow);
          g_alm_fire          = true;
          g_alm_fire_ms       = millis();
          g_wekker_ringing      = true;
          g_wekker_repeat_count = 1;
          g_wekker_last_play_ms = millis();
          dfplayer_volume(g_alarm_vol);
          dfplayer_play(2);   // peep-peep-peep — short, hence the repeat below
          break;
        }
      }
    }

    // Show-set countdown
    if(g_show_timer > 0) { if(--g_show_timer == 0) g_show_set = false; }

    // RTC re-sync every 30 seconds
    if(++g_rtc_tick >= 30) { g_rtc_tick = 0; rtc_sync(); }

    // Brightness update every 8 seconds
    // Brightness now called from loop() every 200ms — not here
  }

  // Rainbow is now a manual, button-triggered mode only (BTN_ALMSEL in
  // check_inputs()) — the old 5-minute automatic idle trigger was removed to
  // save power on a modest 5V/8A buck converter: a full-brightness rainbow
  // across all 256 LEDs draws meaningfully more current than the normal
  // digit display, and it was firing on its own with nobody watching.

  // Temperature flip expiry — cancel after 3 seconds
  if(g_temp_flip_active && TIME_REACHED(now_ms, g_temp_flip_end_ms)) {   // rollover-safe (was now_ms >= ...)
    g_temp_flip_active = false;
  }
  // Also cancel temp flip if user switches mode or alarm fires
  if(g_temp_flip_active && (g_mode != 0 || g_alm_fire || g_rainbow_active)) {
    g_temp_flip_active = false;
  }
}

/******************************************************************************
 * DISPLAY RENDER — draws the current mode to the main matrix, called every loop
 ******************************************************************************/

void render(void) {
  fb_clear();

  // ── Alarm firing: NO full-screen flash (saves power when away) ───────────
  // Status now shown on the free/status bar via compose_bar3(); clock display
  // keeps showing the time normally underneath. Sound + bar LED is enough.

  // ── Egg timer done: NO full-screen flash — sound is enough ───────────────
  if(g_egg_done) {
    if(millis() - g_egg_done_ms >= 10000) g_egg_done = false;  // auto-clear bookkeeping
  }

  // ── Rainbow idle ─────────────────────────────────────────────────────────
  if(g_rainbow_active) {
    fill_rainbow(leds, NUM_LEDS, g_rainbow_hue++, 3);
    leds_show_all(); return;
  }

  // ── Sound-set preview ────────────────────────────────────────────────────
  if(g_show_set) {
    for(uint8_t p = 0; p < 3; p++) draw_dg(p, 11, 200, 200, 0);
    draw_dg(3, g_sound_set, 200, 200, 0);
    leds_show_all(); return;
  }

  // ── Smoker quick-peek overlay ────────────────────────────────────────────
  // Short tap on Sw10 (BTN_PILL) while the smoker is running flashes elapsed
  // HH:MM on the matrix for SMOKE_PEEK_MS, then auto-clears back to whatever
  // was showing before — same colour as the Smoker web page (#FF8C00).
  if(g_smoke_peek_active) {
    if(TIME_REACHED(millis(), g_smoke_peek_end_ms) || !g_smoke_running) {   // rollover-safe (was millis() >= ...)
      g_smoke_peek_active = false;
    } else {
      uint32_t sec = smoke_elapsed_sec();
      uint8_t  eh  = (uint8_t)((sec / 3600) > 99 ? 99 : (sec / 3600));
      uint8_t  em  = (uint8_t)((sec % 3600) / 60);
      draw_dg(0, eh / 10, 255, 140, 0);
      draw_dg(1, eh % 10, 255, 140, 0);
      draw_dg(2, em / 10, 255, 140, 0);
      draw_dg(3, em % 10, 255, 140, 0);
      draw_colon_dots(true);
      leds_show_all(); return;
    }
  }

  // ── Mode 0: Clock ────────────────────────────────────────────────────────
  if(g_mode == 0) {
    // Temperature flip: 5s after each minute tick, show °C/°F for 3 seconds
    if(g_temp_flip_active) {
      float temp_c = rtc.getTemperature();
      uint8_t tr, tg, tb;
      temp_colour(temp_c, tr, tg, tb);
      if(g_temp_degF) {
        // °F mode — no decimal (Fahrenheit from DS3231 not accurate enough for .x)
        int16_t disp = (int16_t)(temp_c * 1.8f + 32.0f);
        if(disp < 0) disp = 0; if(disp > 99) disp = 99;
        draw_dg(0, (uint8_t)(disp / 10), tr, tg, tb);
        draw_dg(1, (uint8_t)(disp % 10), tr, tg, tb);
        draw_dg(2, 17,                    tr, tg, tb);  // °
        draw_dg(3, 16,                    tr, tg, tb);  // F
        draw_colon_dots(false);
      } else {
        // °C mode — decimal: [tens][units].[tenths][C]
        float tc_clamped = (temp_c < 0.0f) ? 0.0f : (temp_c > 39.9f) ? 39.9f : temp_c;
        uint8_t tens   = (uint8_t)(tc_clamped) / 10;
        uint8_t units  = (uint8_t)(tc_clamped) % 10;
        uint8_t tenths = (uint8_t)((tc_clamped - (int)tc_clamped) * 10.0f + 0.5f) % 10;
        draw_dg(0, tens,   tr, tg, tb);
        draw_dg(1, units,  tr, tg, tb);
        draw_dg(2, tenths, tr, tg, tb);
        draw_dg(3, 15,     tr, tg, tb);  // C
        draw_decimal_dot(tr, tg, tb);
      }
    } else {
      uint8_t dh = g_hh;
      if(g_use_12hr) {
        dh = dh % 12;
        if(dh == 0) dh = 12;
      }
      draw_dg(0, dh / 10, THR[0], THG[0], THB[0]);
      draw_dg(1, dh % 10, THR[0], THG[0], THB[0]);
      draw_dg(2, g_mm / 10, THR[0], THG[0], THB[0]);
      draw_dg(3, g_mm % 10, THR[0], THG[0], THB[0]);
      draw_colon_dots(g_colon);
      // AM/PM indicator moved to free/status bar (see compose_bar3) — main
      // matrix no longer carries this dot, keeping the digits clean.
    }
  }

  // ── Mode 1: Alarm set ────────────────────────────────────────────────────
  else if(g_mode == 1) {
    // Slot-number indicator dot removed 2026-06-24 — only one alarm now.
    draw_dg(0, g_alarm1.hh / 10, THR[1], THG[1], THB[1]);
    draw_dg(1, g_alarm1.hh % 10, THR[1], THG[1], THB[1]);
    draw_dg(2, g_alarm1.mm / 10, THR[1], THG[1], THB[1]);
    draw_dg(3, g_alarm1.mm % 10, THR[1], THG[1], THB[1]);
    draw_colon_dots(g_alarm1.enabled);
  }

  // ── Mode 2: Egg timer running ────────────────────────────────────────────
  else if(g_mode == 2) {
    // Progress bar across bottom row of all panels
    uint32_t total_s  = (uint32_t)g_egg_set_mm*60 + g_egg_set_ss;
    uint32_t remain_s = (uint32_t)g_egg_mm*60 + g_egg_ss;
    if(total_s > 0) {
      uint8_t filled = (uint8_t)((32UL * (total_s - remain_s)) / total_s);
      for(uint8_t i = 0; i < 32; i++) {
        uint8_t panel = i / 8;
        uint8_t col   = i % 8;
        uint8_t row   = 7;
        uint16_t idx  = (uint16_t)panel*64 + (row & 1 ? row*8+(7-col) : row*8+col);
        if(idx < NUM_LEDS) leds[idx] = (i < filled) ? CRGB(0,200,80) : CRGB(30,30,30);
      }
    }
    draw_dg(0, g_egg_mm / 10, THR[2], THG[2], THB[2]);
    draw_dg(1, g_egg_mm % 10, THR[2], THG[2], THB[2]);
    draw_dg(2, g_egg_ss / 10, THR[2], THG[2], THB[2]);
    draw_dg(3, g_egg_ss % 10, THR[2], THG[2], THB[2]);
    draw_colon_dots(g_colon);
  }

  // ── Mode 3: Egg timer set ────────────────────────────────────────────────
  else if(g_mode == 3) {
    draw_dg(0, g_egg_set_mm / 10, 255, 120, 0);
    draw_dg(1, g_egg_set_mm % 10, 255, 120, 0);
    draw_dg(2, g_egg_set_ss / 10, 255, 120, 0);
    draw_dg(3, g_egg_set_ss % 10, 255, 120, 0);
    draw_colon_dots(true);
  }

  // ── Mode 4: Temperature (DS3231 built-in sensor) ─────────────────────────
  else if(g_mode == 4) {
    float temp_c = rtc.getTemperature();
    uint8_t tr, tg, tb;
    temp_colour(temp_c, tr, tg, tb);
    if(g_temp_degF) {
      // °F — no decimal (conversion not accurate enough for .x)
      int16_t disp = (int16_t)(temp_c * 1.8f + 32.0f);
      if(disp < 0) disp = 0; if(disp > 99) disp = 99;
      draw_dg(0, (uint8_t)(disp / 10), tr, tg, tb);
      draw_dg(1, (uint8_t)(disp % 10), tr, tg, tb);
      draw_dg(2, 17,                    tr, tg, tb);  // °
      draw_dg(3, 16,                    tr, tg, tb);  // F
      draw_colon_dots(false);
    } else {
      // °C with one decimal place: [tens][units].[tenths][C]
      float tc_clamped = (temp_c < 0.0f) ? 0.0f : (temp_c > 39.9f) ? 39.9f : temp_c;
      uint8_t tens   = (uint8_t)(tc_clamped) / 10;
      uint8_t units  = (uint8_t)(tc_clamped) % 10;
      uint8_t tenths = (uint8_t)((tc_clamped - (int)tc_clamped) * 10.0f + 0.5f) % 10;
      draw_dg(0, tens,   tr, tg, tb);
      draw_dg(1, units,  tr, tg, tb);
      draw_dg(2, tenths, tr, tg, tb);  // decimal digit
      draw_dg(3, 15,     tr, tg, tb);  // C
      draw_decimal_dot(tr, tg, tb);
    }
  }

  // ── Mode 5: Uptime ───────────────────────────────────────────────────────
  else if(g_mode == 5) {
    uint32_t up_s = millis() / 1000UL;
    uint8_t  days = (uint8_t)(up_s / 86400UL);
    uint8_t  hrs  = (uint8_t)((up_s % 86400UL) / 3600UL);
    if(days > 99) days = 99;
    if(hrs  > 99) hrs  = 99;
    draw_dg(0, days / 10, THR[4], THG[4], THB[4]);
    draw_dg(1, days % 10, THR[4], THG[4], THB[4]);
    draw_dg(2, hrs  / 10, THR[4], THG[4], THB[4]);
    draw_dg(3, hrs  % 10, THR[4], THG[4], THB[4]);
    // Use dot separator centred on panel 1 right edge
    uint16_t dotidx = pix(1, 7, 3);
    if(dotidx < NUM_LEDS) leds[dotidx] = CRGB(0, 60, 180);
  }

  leds_show_all();
}

/******************************************************************************
 * SECURITY ALARM SYSTEM — two zones, arm/disarm/panic
 ******************************************************************************/
//  ZONE 1  — Outdoor courtyard (PIR1 Courtyard A, PIR2 Courtyard B, PIR3
//            Courtyard C — PIR1 currently parked, no free GPIO, see the Pin
//            Definitions section)
//    • Always armed 24/7 — protects courtyard even when you are home
//    • No entry delay — immediate siren (you are safe inside)
//    • Arm/disarm via /alarm page Zone 1 toggle
//
//  ZONE 2  — Indoor (PIR4 front door/entrance, PIR5 passage/lounge)
//    • Armed only when you leave (AWAY mode)
//    • 30s exit delay after arming (time to walk out)
//    • 30s entry delay after motion (time to disarm before siren)
//    • Disarm at any time — including during exit delay — works immediately
//
//  The g_disarm_requested flag ensures disarm is honoured even during the
//  exit-delay countdown, which would otherwise block the disarm check.

// ── Arm / Disarm functions ───────────────────────────────────────────────────

void sec_zone2_arm(void) {
  g_z1_active        = true;   // ARM always re-enables courtyard sensors
                                // even if they were suspended — no gaps in coverage
  g_z2_armed         = true;
  g_sec_triggered    = false;
  g_sec_exit_delay   = true;
  g_sec_entry_delay  = false;
  g_sec_arm_ms       = millis();
  g_sec_entry_ms     = 0;
  g_siren_start_ms   = 0;
  g_disarm_requested = false;
  request_beep(BEEP_DOUBLE);   // two beeps = ARMED — deferred, doesn't block the HTTP response
  Serial.println("ARMED — Zone 1 (courtyard) + Zone 2 (indoor) active. Exit delay started.");
  po_notify("Alarm Armed", "Away mode active. All sensors armed.\nTime: " + po_time());
}

void sec_zone2_disarm(void) {
  g_disarm_requested = true;   // flag picked up on next loop() pass
  Serial.println("ZONE 2 DISARM requested");
}

void sec_full_disarm(void) {
  // Stop siren
  noTone(SIREN_PIN);
  digitalWrite(SIREN_PIN, LOW);

  // Deferred the same way as Arm's beep (see request_beep() above): calling
  // beep_triple() directly would block for ~480ms on whichever core runs
  // this function, and /alarm/disarm calls sec_full_disarm() straight from
  // a Core 0 web handler — stalling webServer.handleClient() for that long
  // before the HTTP response goes back to the phone/browser. Flag set here
  // (near-instant), loop() on Core 1 does the actual beeping a few ms later.
  request_beep(BEEP_TRIPLE);   // three beeps = DISARMED / all clear — deferred, doesn't block the HTTP response
  // Clear ALL alarm state — PIR and panic
  g_z2_armed          = false;
  g_sec_triggered     = false;
  g_sec_exit_delay    = false;
  g_sec_entry_delay   = false;
  g_sec_arm_ms        = 0;
  g_sec_entry_ms      = 0;
  g_siren_start_ms    = 0;
  g_disarm_requested  = false;
  g_panic_requested   = false;
  g_panic_active      = false;   // stop panic siren independently
  g_panic_start_ms    = 0;
  g_trig_zone         = 0;
  g_trig_sensor       = 0;

  // Set lockout timestamp — blocks PIR re-triggering for DISARM_LOCKOUT_MS
  // This stops the AM312 (which holds HIGH for ~2s) from immediately re-arming
  g_disarm_lockout_ms = millis();

  // Status LED now handled entirely by compose_bar3() — no main-matrix writes here
  Serial.println("DISARMED — all clear. Lockout active for 8s.");
  po_notify("Alarm Disarmed", "All clear. Welcome home.\nTime: " + po_time());
}

void sec_panic(void) {
  request_beep(BEEP_SINGLE);   // single beep = panic acknowledged — deferred,
                                // so this function returns immediately and the
                                // HTTP response goes straight back to the phone
  g_panic_requested = true;
  Serial.println("PANIC BUTTON pressed!");
}

// ── PIR check — call every loop() pass ──────────────────────────────────────

void sec_alarm_check_pir(void) {

  // ── Panic button — independent 15-min siren ──────────────────────────────
  if (g_panic_requested) {
    g_panic_active    = true;
    g_panic_start_ms  = millis();
    g_panic_requested = false;
    Serial.println("PANIC — SIREN ON for 15 minutes");
    // Also sound through the clock's own onboard speaker, independent of the
    // external siren wiring — this always plays regardless of whether the
    // clock is in its usual spot with the siren connected, or has been moved
    // elsewhere in the house without it (Henry's request).
    dfplayer_volume(g_alarm_vol);
    dfplayer_play(2); voice_lock();
    po_notify("PANIC — EMERGENCY", "Panic button pressed! Ouma needs help!\nTime: " + po_time(), 2);
    return;
  }

  // ── Process disarm — stops everything including panic ────────────────────
  if (g_disarm_requested) {
    sec_full_disarm();
    return;
  }

  // ── Post-disarm lockout — ignore PIRs for DISARM_LOCKOUT_MS ─────────────
  // Fixes AM312 holding HIGH after movement stops immediate re-trigger
  if (g_disarm_lockout_ms > 0) {
    if (millis() - g_disarm_lockout_ms < (uint32_t)DISARM_LOCKOUT_MS) {
      return;
    }
    // Lockout time expired — but check if any PIR is still physically HIGH
    // If so, extend lockout another DISARM_LOCKOUT_MS rather than immediately retriggering
    bool any_pir_high = false;
    for (uint8_t i = 0; i < PIR_COUNT; i++) {
      if (pir_read(i)) { any_pir_high = true; break; }
    }
    if (any_pir_high) {
      // PIR still HIGH — extend lockout, stay green
      g_disarm_lockout_ms = millis();
      Serial.println("PIR still HIGH after lockout — extending 8s");
      return;
    }
    // All PIRs LOW — safe to resume
    g_disarm_lockout_ms = 0;
    Serial.println("Lockout expired, all PIRs LOW — monitoring resumed");
  }

  // ── Panic siren is independent — PIR logic must not interfere ───────────
  if (g_panic_active) return;

  // ── ZONE 1: Courtyard — always on, immediate siren ───────────────────────
  // The "&& !g_sec_exit_delay" guard is essential: without it, walking out
  // through the courtyard during your own 30s exit delay would trip Zone 1
  // on yourself, setting g_sec_triggered=true — which short-circuits every
  // later pass of this function (everything below checks !g_sec_triggered),
  // so the code that clears g_sec_exit_delay after 30s (further down) would
  // never run, leaving the orange "exit delay" LED flashing forever instead
  // of switching to solid armed-red. Zone 1 simply ignores motion during the
  // walk-out window (you're expected to be there), then resumes full,
  // immediate coverage the instant exit delay ends — no change to Zone 1's
  // behaviour at any other time.
  if (g_z1_active && !g_sec_triggered && !g_sec_exit_delay) {
    for (uint8_t i = 0; i < ZONE1_COUNT; i++) {
      if (pir_read(i)) {
        Serial.printf("ZONE 1 triggered: %s\n", PIR_NAMES[i]);
        g_sec_triggered  = true;
        g_trig_zone      = 1;
        g_trig_sensor    = i;
        g_siren_start_ms = millis();
        dfplayer_volume(g_alarm_vol);
        dfplayer_play(2); voice_lock();
        po_notify("ALARM — INTRUDER IN COURTYARD", "Sensor: " + String(PIR_NAMES[i]) + "\nTime: " + po_time(), 2);
        return;
      }
    }
  }

  // ── ZONE 2: Indoor — only when armed ─────────────────────────────────────
  if (!g_z2_armed) {
    return;   // Zone 2 disarmed — home safe
  }

  // Exit delay
  if (g_sec_exit_delay) {
    if (millis() - g_sec_arm_ms >= (uint32_t)ALARM_EXIT_DELAY_S * 1000UL) {
      g_sec_exit_delay = false;
      Serial.println("Exit delay done — Zone 2 armed and monitoring");
    }
    return;
  }

  // Already triggered — handled by sec_alarm_siren_update()
  if (g_sec_triggered) return;

  // Entry delay
  if (g_sec_entry_delay) {
    if (millis() - g_sec_entry_ms >= (uint32_t)ALARM_ENTRY_DELAY_S * 1000UL) {
      g_sec_triggered   = true;
      g_sec_entry_delay = false;
      g_trig_zone       = 2;
      g_siren_start_ms  = millis();
      Serial.println("Entry delay expired — ZONE 2 SIREN ON");
      dfplayer_volume(g_alarm_vol);
      dfplayer_play(2); voice_lock();
      po_notify("ALARM — INTRUDER INSIDE", "Sensor: " + String(PIR_NAMES[g_trig_sensor]) + "\nTime: " + po_time(), 2);
    }
    return;
  }

  // Armed and quiet — check Zone 2 sensors
  for (uint8_t i = ZONE1_COUNT; i < PIR_COUNT; i++) {
    if (pir_read(i)) {
      Serial.printf("ZONE 2 motion: %s — entry delay started\n", PIR_NAMES[i]);
      g_sec_entry_delay = true;
      g_sec_entry_ms    = millis();
      g_trig_sensor     = i;
      po_notify("Motion Detected — Entry Delay", "Sensor: " + String(PIR_NAMES[i]) + "\n30 seconds to disarm!\nTime: " + po_time(), 1);
      break;
    }
  }
}

// ── Siren — call every loop() pass ──────────────────────────────────────────

void sec_alarm_siren_update(void) {
  static bool siren_was_on = false;

  // ── During disarm lockout — siren is already stopped ─────────────────────
  if (g_disarm_lockout_ms > 0 &&
      millis() - g_disarm_lockout_ms < (uint32_t)DISARM_LOCKOUT_MS) {
    noTone(SIREN_PIN);
    digitalWrite(SIREN_PIN, LOW);
    siren_was_on = false;
    return;
  }
  // Only disarm button stops it early. PIR state has no effect on panic.
  if (g_panic_active) {
    if (millis() - g_panic_start_ms >= (uint32_t)PANIC_SIREN_TIMEOUT_S * 1000UL) {
      // 15 minutes up — stop panic siren, restore normal home mode
      g_panic_active   = false;
      g_panic_start_ms = 0;
      noTone(SIREN_PIN);
      digitalWrite(SIREN_PIN, LOW);
      siren_was_on = false;
      Serial.println("Panic siren auto-stopped after 15 minutes");
      return;
    }
    // Panic siren wailing — faster sweep than PIR alarm so it sounds different
    siren_was_on = true;
    uint32_t t = millis() % 800UL;
    uint16_t freq = (t < 400)
      ? (uint16_t)(1000 + (1400UL * t) / 400UL)    // sweep up 1000→2400
      : (uint16_t)(2400 - (1400UL * (t - 400UL)) / 400UL);  // sweep down
    tone(SIREN_PIN, freq);
    return;
  }

  // ── PIR alarm siren ───────────────────────────────────────────────────────
  if (!g_sec_triggered) {
    if (siren_was_on) {
      noTone(SIREN_PIN);
      digitalWrite(SIREN_PIN, LOW);
      siren_was_on = false;
    }
    return;
  }

  // PIR auto-reset after ALARM_SIREN_TIMEOUT_S (3 min default — saves power when away)
  if (g_siren_start_ms > 0 &&
      (millis() - g_siren_start_ms >= (uint32_t)ALARM_SIREN_TIMEOUT_S * 1000UL)) {
    Serial.printf("PIR siren auto-reset after %d minutes\n", ALARM_SIREN_TIMEOUT_S / 60);
    noTone(SIREN_PIN);
    digitalWrite(SIREN_PIN, LOW);
    g_sec_triggered   = false;
    g_sec_entry_delay = false;
    g_siren_start_ms  = 0;
    g_trig_zone       = 0;
    g_trig_sensor     = 0;
    siren_was_on      = false;
    return;
  }

  // PIR siren wailing — slower sweep than panic
  siren_was_on = true;
  uint32_t t = millis() % 1200UL;
  uint16_t freq = (t < 600)
    ? (uint16_t)(800  + (1600UL * t) / 600UL)
    : (uint16_t)(2400 - (1600UL * (t - 600UL)) / 600UL);
  tone(SIREN_PIN, freq);
}

// ── Web page — ARM/DISARM control ───────────────────────────────────────────

void sec_alarm_web_page(void) {
  uint32_t now_ms  = millis();
  String   detail  = "";

  // ── Wake-up clock alarm — separate from security system ──────────────────
  // Shows a big dismiss banner if the clock alarm (g_alm_fire) is currently ringing
  String wake_banner = "";
  if (g_alm_fire) {
    wake_banner =
      "<div style='background:#ff0000;border-radius:10px;padding:16px;margin-bottom:12px;"
      "animation:flashred 0.6s infinite'>"
      "<h2 style='margin:4px 0;color:#fff'>&#9200; WEKKER LUI!</h2>"
      "<form method='GET' action='/alarm/dismiss'>"
      "<button type='submit' style='width:100%;padding:16px;font-size:1.3em;font-weight:bold;"
      "background:#fff;color:#cc0000;border:none;border-radius:8px;margin-top:8px'>"
      "&#9989; SKAKEL WEKKER AF</button></form>"
      "<script>setTimeout(()=>location.reload(),1000)</script>"
      "</div>";
  }

  // ── Determine overall state string and colour ────────────────────────────
  String state, stcol;
  if (g_panic_active) {
    state  = "&#128680; PANIC — EMERGENCY!";
    stcol  = "#ff0000";
    uint32_t elapsed = (now_ms - g_panic_start_ms) / 1000UL;
    uint32_t rem     = (elapsed < (uint32_t)PANIC_SIREN_TIMEOUT_S)
                       ? ((uint32_t)PANIC_SIREN_TIMEOUT_S - elapsed) : 0;
    detail = "<p style='color:#ff4444'>Panic button activated — siren screaming!</p>"
             "<p style='color:#ff6666;font-size:0.9em'>Auto-stops in <b>"
             + String(rem / 60) + "m " + String(rem % 60) + "s</b> — or press DISARM to stop now</p>"
             "<script>setTimeout(()=>location.reload(),1000)</script>";
  } else if (g_sec_triggered) {
    state  = (g_trig_zone == 1)
             ? "&#128680; ALARM! INTRUDER IN COURTYARD!"
             : "&#128680; ALARM! INTRUDER INSIDE!";
    stcol  = "#ff0000";
    String sensor_info = (g_trig_sensor == 255) ? "Manual panic button" :
                         (String(PIR_NAMES[g_trig_sensor]) + " — " + String(PIR_ZONES[g_trig_sensor]));
    // Show how long siren has been running and when it auto-resets
    uint32_t siren_elapsed = (g_siren_start_ms > 0) ? (now_ms - g_siren_start_ms) / 1000UL : 0;
    uint32_t siren_rem     = (siren_elapsed < (uint32_t)ALARM_SIREN_TIMEOUT_S)
                             ? ((uint32_t)ALARM_SIREN_TIMEOUT_S - siren_elapsed) : 0;
    detail = "<p style='color:#ff4444'>Sensor: <b>" + sensor_info + "</b></p>"
             "<p style='color:#ff6666;font-size:0.9em'>Siren auto-resets in <b>"
             + String(siren_rem) + "s</b> if no disarm</p>"
             "<script>setTimeout(()=>location.reload(),1000)</script>";
  } else if (g_sec_entry_delay) {
    uint32_t elapsed = now_ms - g_sec_entry_ms;
    uint32_t rem = (elapsed < (uint32_t)ALARM_ENTRY_DELAY_S * 1000UL)
                   ? ((uint32_t)ALARM_ENTRY_DELAY_S * 1000UL - elapsed) / 1000UL : 0;
    state  = "&#9888; ENTRY DELAY";
    stcol  = "#ff6600";
    detail = "<p style='color:#ff6600;font-size:1.2em'>MOTION DETECTED inside! "
             "Disarm within <b>" + String(rem) + "s</b> or siren fires!</p>"
             "<script>setTimeout(()=>location.reload(),1000)</script>";
  } else if (g_sec_exit_delay) {
    uint32_t elapsed = now_ms - g_sec_arm_ms;
    uint32_t rem = (elapsed < (uint32_t)ALARM_EXIT_DELAY_S * 1000UL)
                   ? ((uint32_t)ALARM_EXIT_DELAY_S * 1000UL - elapsed) / 1000UL : 0;
    if (rem == 0) {
      // Countdown done — loop() will clear g_sec_exit_delay on next pass
      // Show ARMED immediately so page does not get stuck at 0s
      state  = "&#128274; AWAY — FULLY ARMED";
      stcol  = "#ff3300";
      detail = "<p style='color:#ff3300'>Zone 1 + Zone 2 active. All PIR sensors monitoring.</p>"
               "<script>setTimeout(()=>location.reload(),2000)</script>";
    } else {
      state  = "&#9201; EXIT DELAY — LEAVE NOW";
      stcol  = "#ffaa00";
      detail = "<p style='color:#ffaa00'>Leave within <b>" + String(rem)
               + "s</b>. Zone 2 arms automatically.</p>"
               "<script>setTimeout(()=>location.reload(),1000)</script>";
    }
  } else if (g_z2_armed) {
    state  = "&#128274; AWAY — FULLY ARMED";
    stcol  = "#ff3300";
    detail = "<p style='color:#ff3300'>Zone 1 + Zone 2 active. All PIR sensors monitoring.</p>"
             "<script>setTimeout(()=>location.reload(),5000)</script>";
  } else if (g_z1_active) {
    state  = "&#127968; HOME — Courtyard Armed";
    stcol  = "#00aaff";
    detail = "<p style='color:#00aaff'>Zone 1 (courtyard) active 24/7.<br>"
             "Zone 2 (inside) disarmed — you are home.</p>"
             "<script>setTimeout(()=>location.reload(),10000)</script>";
  } else {
    state  = "&#128275; ALL DISARMED";
    stcol  = "#888888";
    detail = "<p style='color:#888'>All sensors off. Not recommended!</p>";
  }

  // ── Build page ───────────────────────────────────────────────────────────
  String page =
    "<!DOCTYPE html><html><head><meta charset='utf-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>Alarm — Ouma Ria Smart Clock</title>"
    "<style>"
    "body{font-family:sans-serif;background:#111;color:#eee;text-align:center;"
    "padding:16px;max-width:440px;margin:0 auto}"
    "h1{color:#ffa500;margin:8px 0}"
    "@keyframes flashred{"
    "0%,100%{background:#330000;border-color:#ff0000;color:#ff0000}"
    "50%{background:#ff0000;border-color:#ff6666;color:#fff}}"
    ".status{font-size:1.5em;font-weight:bold;padding:14px;border-radius:10px;"
    "margin:12px 0;border:2px solid " + stcol + ";color:" + stcol + "}"
    ".alarm-flash{animation:flashred 0.6s infinite}"
    ".btn{display:block;width:100%;padding:18px;font-size:1.3em;font-weight:bold;"
    "border:none;border-radius:12px;cursor:pointer;margin:10px 0}"
    ".arm-btn{background:#cc2200;color:#fff}"
    ".dis-btn{background:#007722;color:#fff}"
    ".z1on{background:#005599;color:#fff}"
    ".z1off{background:#444;color:#aaa}"
    ".zone-box{border:1px solid #444;border-radius:8px;padding:10px;margin:10px 0;text-align:left}"
    ".zone-title{font-weight:bold;font-size:1.05em;margin-bottom:4px}"
    ".sensor{color:#aaa;font-size:0.9em;margin:2px 0}"
    "hr{border-color:#333;margin:14px 0}"
    "nav a{color:#0af;margin-right:1rem;font-size:0.9em}"
    "</style></head><body style='padding-bottom:84px'>"
    "<a href='/' style='position:fixed;bottom:18px;right:16px;width:60px;height:60px;"
    "background:#0a84ff;color:#fff;border-radius:50%;display:flex;align-items:center;"
    "justify-content:center;font-size:1.8rem;text-decoration:none;"
    "box-shadow:0 3px 12px rgba(0,0,0,0.6);z-index:999'>&#127968;</a>"
    "<h1>&#128274; Security Alarm</h1>"
    "<nav><a href='/'>Status</a><a href='/alarm'>Alarm</a></nav><hr>"
    + wake_banner +
    "<div class='" + String((g_sec_triggered || g_panic_active) ? "status alarm-flash" : "status") + "'>" + state + "</div>"
    + detail +

    // Zone 2 — main ARM/DISARM (away mode)
    "<hr><p style='color:#aaa;font-size:0.95em;margin:4px 0'><b>ZONE 2 — Indoor (Away mode)</b></p>"
    "<form method='POST' action='/alarm/arm'>"
    "<button class='btn arm-btn'>&#128663; ARM — I am leaving (Away)</button></form>"
    "<form method='POST' action='/alarm/disarm'>"
    "<button class='btn dis-btn'>&#128275; DISARM — I am home</button></form>"

    // PANIC button — always visible, big and red
    "<hr>"
    "<p style='color:#ff4444;font-size:0.95em;margin:4px 0'><b>&#9888; EMERGENCY</b></p>"
    "<form method='POST' action='/alarm/panic'>"
    "<button class='btn' style='background:#ff0000;color:#fff;font-size:1.6em;"
    "padding:24px;border:3px solid #ff6666;letter-spacing:2px'>"
    "&#128680; PANIC / EMERGENCY &#128680;</button></form>"
    "<p style='color:#666;font-size:0.8em;margin:2px 0'>"
    "Press if you feel unsafe — fires siren immediately</p>"

    // Zone 1 — courtyard toggle
    "<hr><p style='color:#aaa;font-size:0.95em;margin:4px 0'><b>ZONE 1 — Courtyard (24/7)</b></p>"
    + (g_z1_active
      ? "<form method='POST' action='/alarm/z1off'>"
        "<button class='btn z1off'>&#9208; Pause Courtyard (mowing lawn etc.)</button></form>"
        "<p style='color:#888;font-size:0.8em;margin:2px 0'>"
        "Pauses new detections only. Remember to ARM before leaving!</p>"
      : "<form method='POST' action='/alarm/z1on'>"
        "<button class='btn z1on'>&#128994; Resume Courtyard sensors</button></form>"
        "<p style='color:#ffaa00;font-size:0.85em;margin:2px 0'>"
        "&#9888; Courtyard sensors paused! ARM will re-enable them automatically.</p>") +

    // Zone status boxes
    "<hr>"
    "<div class='zone-box'>"
    "<div class='zone-title' style='color:#00aaff'>&#127968; ZONE 1 — Courtyard (always on)</div>"
    "<div class='sensor'>&#9898; PIR 1 — Courtyard sensor A (parked — moving to PCF8575)</div>"
    "<div class='sensor'>&#128308; PIR 2 — Courtyard sensor B (GPIO 15)</div>"
    "<div class='sensor'>&#128308; PIR 3 — Courtyard sensor C (GPIO 36)</div>"
    "<div class='sensor' style='color:#888'>No entry delay — immediate siren</div>"
    "</div>"
    "<div class='zone-box'>"
    "<div class='zone-title' style='color:#ff6633'>&#128274; ZONE 2 — Indoor (Away mode only)</div>"
    "<div class='sensor'>&#128308; PIR 4 — Front door / Entrance (GPIO 39)</div>"
    "<div class='sensor'>&#128308; PIR 5 — Passage / Lounge (GPIO 23)</div>"
    "<div class='sensor' style='color:#888'>Exit delay: " + String(ALARM_EXIT_DELAY_S)
    + "s &nbsp;|&nbsp; Entry delay: " + String(ALARM_ENTRY_DELAY_S) + "s</div>"
    "</div>"

    "<hr><div class='zone-box'>"
    "<div class='zone-title' style='color:#aaa'>&#128241; Pushover — laaste poging</div>"
    "<div class='sensor' style='color:#ccc'>" + g_po_last + "</div>"
    "<div class='sensor'><a href='/potest' style='color:#0af'>Stuur toets-boodskap nou</a></div>"
    "</div>"

    "<hr><p style='color:#666;font-size:0.8em'>"
    "Arm before you leave. Disarm before you enter.<br>"
    "Courtyard sensors protect you even when you are home.</p>"
    "</body></html>";

  webServer.send(200, "text/html", page);
}

/******************************************************************************
 * End Security Alarm Functions
 ******************************************************************************/

/******************************************************************************
 * PUSHOVER NOTIFICATIONS
 ******************************************************************************/
//  Sends push notifications to your Oppo phone via the Pushover app.
//  Works anywhere your phone has data — even 80km away in town.
//  priority 0 = normal,  priority 1 = high (plays sound even on silent)
//  priority 2 = emergency — Pushover keeps RESENDING with sound/vibration
//  every PO_RETRY_SECS seconds, for up to PO_EXPIRE_SECS, until the
//  notification is opened/acknowledged on the phone or it expires. Required
//  by Pushover's API whenever priority=2 is used (NEW 2026-07-01).
// PO_RETRY_SECS / PO_EXPIRE_SECS moved up next to the PoMsg queue —
// they are default arguments in the po_* declarations now.

void po_send(const String& title, const String& msg, uint8_t priority, const String& sound, uint16_t retry, uint16_t expire) {
  if (WiFi.status() != WL_CONNECTED) {
    g_po_last = po_time() + " — OORGESLAAN: WiFi af / WiFi not connected";
    Serial.println("Pushover: WiFi not connected — skipped");
    return;
  }

  WiFiClientSecure client;
  client.setInsecure();     // skip SSL cert check — fine for Pushover
  client.setTimeout(10000); // FIX  (2026-07-04): was setTimeout(5).
  // ↑ THE PUSHOVER KILLER. On older ESP32 cores setTimeout() took SECONDS
  // (5 = 5s, worked fine). Arduino-ESP32 core 3.x changed it to Arduino-
  // standard MILLISECONDS — the SAME line recompiled under a newer core gave
  // the TLS handshake a 5ms budget, so client.connect() failed on EVERY
  // send. That's why Pushover died at the v13 flash without po_send()
  // changing: the poison came in with the toolchain, not the source. 10000
  // is sane in ms-land; under an old seconds-core it only inflates the read
  // timeout, which the bounded loop below caps at 3s regardless.

  Serial.print("Pushover: sending... ");

  if (!client.connect(PO_HOST, 443, 10000)) {   // explicit 10s connect budget (ms in all cores)
    g_po_last = po_time() + " — KONNEKSIE MISLUK / connect to api.pushover.net failed";
    Serial.println("connection failed");
    return;
  }

  // Build POST body
  // Sanitize the two free-text fields first. The body is
  // form-urlencoded, so a literal '&' or '=' inside a title/message would
  // truncate or corrupt the fields Pushover parses. All current callers use
  // safe constants, but this makes po_send() safe for ANY future text.
  String t_enc = title;  t_enc.replace("&", "%26");  t_enc.replace("=", "%3D");
  String m_enc = msg;    m_enc.replace("&", "%26");  m_enc.replace("=", "%3D");
  String body = "token="   + String(PO_APP_TOKEN)
              + "&user="   + String(PO_USER_KEY)
              + "&title="  + t_enc
              + "&message=" + m_enc
              + "&priority=" + String(priority);
  if (priority == 2) {
    // Emergency priority requires retry + expire, or Pushover rejects the request
    body += "&retry=" + String(retry) + "&expire=" + String(expire);   // Per-message pacing
  }
  if (sound.length() > 0) {
    body += "&sound=" + sound;   // e.g. "bike" — see https://pushover.net/api#sounds
  }

  // URL-encode spaces and newlines in body
  body.replace(" ", "+");
  body.replace("\n", "%0A");

  client.print(String("POST /1/messages.json HTTP/1.1\r\n")
             + "Host: " + PO_HOST + "\r\n"
             + "Content-Type: application/x-www-form-urlencoded\r\n"
             + "Content-Length: " + String(body.length()) + "\r\n"
             + "Connection: close\r\n\r\n"
             + body);

  // BOUNDED response capture — hard 3s ceiling, typically <300ms.
  // The 2026-07-01 no-wait fix was right about the problem (the old code
  // could pin Core 0 for 5s+ during an alarm) but threw away the delivery
  // confirmation, which made this week's failure invisible. This loop keeps
  // the Core 0 protection (3s worst case, millis()-budgeted, never relies on
  // stream timeouts) and recovers the HTTP status line so /potest and /alarm
  // can show whether Pushover actually accepted the message.
  String resp; bool done = false;
  uint32_t t0 = millis();
  while (!done && (millis() - t0 < 3000)) {
    while (client.available()) {
      char c = (char)client.read();
      if (c == '\n') { done = true; break; }        // end of HTTP status line
      if (c != '\r' && resp.length() < 120) resp += c;
    }
    if (!done) {
      if (!client.connected() && !client.available()) break;
      delay(10);
    }
  }
  if (resp.length() == 0) {
    g_po_last = po_time() + " — GESTUUR, geen antwoord binne 3s / sent, no reply in 3s";
    Serial.println("sent (no response within 3s)");
  } else {
    g_po_last = po_time() + " — " + resp;           // e.g. "HTTP/1.1 200 OK"
    Serial.println(resp);
  }
  client.stop();
}

/* ORIGINAL response-wait code, kept for reference — do not re-enable without
   moving po_send() off Core 0 first, e.g. onto its own dedicated task, or
   this exact bug comes right back:

  // Wait for response
  uint32_t t = millis();
  while (client.available() == 0 && millis() - t < 5000) delay(10);

  if (client.available()) {
    String line = client.readStringUntil('\n');
    Serial.println(line.indexOf("200") >= 0 ? "OK" : "resp: " + line);
  }
  client.stop();
}
*/

// Helper — HH:MM timestamp for notification messages
String po_time(void) {
  struct tm ti;
  if (!getLocalTime(&ti)) return "??:??";
  char buf[6];
  sprintf(buf, "%02d:%02d", ti.tm_hour, ti.tm_min);
  return String(buf);
}


/******************************************************************************
 * PIEZO BEEP CONFIRMATIONS
 ******************************************************************************/
//  Single beep  = alarm button pressed / acknowledged
//  Double beep  = alarm ARMED (away mode activated)
//  Triple beep  = alarm DISARMED (welcome home)
//  Uses SIREN_PIN via tone() — short bursts, not the wailing sweep

void beep_single(void) {
  // One short beep — button pressed / acknowledged
  tone(SIREN_PIN, 1800, 80);    // 1800Hz for 80ms
  delay(80);
  noTone(SIREN_PIN);
  digitalWrite(SIREN_PIN, LOW);
}

void beep_double(void) {
  // Two beeps — armed confirmation
  tone(SIREN_PIN, 1800, 80);
  delay(120);
  noTone(SIREN_PIN);
  delay(80);
  tone(SIREN_PIN, 1800, 80);
  delay(80);
  noTone(SIREN_PIN);
  digitalWrite(SIREN_PIN, LOW);
}

void beep_triple(void) {
  // Three beeps — disarmed / all clear
  for (uint8_t i = 0; i < 3; i++) {
    tone(SIREN_PIN, 1200, 80);
    delay(160);
    noTone(SIREN_PIN);
  }
  digitalWrite(SIREN_PIN, LOW);
}


/******************************************************************************
 * QUICK REFERENCE
 ******************************************************************************/
//
//  BUTTON SUMMARY
//  ┌──────────────┬────────────────────────────────────────────────────────┐
//  │ Button       │ Action                                                 │
//  ├──────────────┼────────────────────────────────────────────────────────┤
//  │ BTN_MODE     │ Short: cycle modes 0→1→2→3→4→5→0                      │
//  │              │ Long (>1s): toggle °C / °F temperature display        │
//  │ BTN_SET      │ Mode 1: alarm HH up  Mode 3: egg MM up  Mode 2: start │
//  │ BTN_UP       │ Mode 1: alarm MM up  Mode 3: egg SS up                │
//  │              │ Short during alarm: dismiss  Long: SNOOZE 9 min       │
//  │ BTN_ALM      │ Toggle current alarm slot on/off                      │
//  │ BTN_ALMSEL   │ Reserved / spare (plays confirm ding, no other action)│
//  │ BTN_CHIME    │ Toggle quarter-hour chimes on/off                     │
//  │ BTN_CVUP/DN  │ Volume up/down (day vol or night vol depending on time)│
//  │ BTN_SOUND    │ Cycle sound set 0→4→0 (plays preview chime)           │
//  │ BTN_PILL     │ Hold 3s: reboot clock (retry WiFi after loadshedding) │
//  │ BTN_MED      │ Ouma confirms medicine taken                          │
//  │ BTN_BDAY     │ Accept/dismiss birthday or doctor appointment alert   │
//  ├──────────────┼────────────────────────────────────────────────────────┤
//  │ BOOT combos  │ Hold BTN_SET + BTN_UP at power-on → factory reset     │
//  └──────────────┴────────────────────────────────────────────────────────┘
//
//  MODE COLOURS
//  ┌──────┬────────────────────┬───────────────┐
//  │ Mode │ Display            │ Colour        │
//  ├──────┼────────────────────┼───────────────┤
//  │  0   │ Clock (HH:MM)      │ Amber         │
//  │  1   │ Alarm set          │ Cyan          │
//  │  2   │ Egg timer running  │ Green + bar   │
//  │  3   │ Egg timer set      │ Orange        │
//  │  4   │ Temperature (°C/F) │ Magenta       │
//  │  5   │ Uptime (days.hrs)  │ Blue          │
//  │  --  │ Rainbow (Sw9)      │ Cycling       │
//  └──────┴────────────────────┴───────────────┘
//
//  SD CARD TRACK LAYOUT (numbered files on the DFPlayer's microSD card)
//  ┌──────────┬──────────────────────────────────────────────────────────┐
//  │ Track    │ Content                                                  │
//  ├──────────┼──────────────────────────────────────────────────────────┤
//  │ 1        │ Wake alarm sound                                         │
//  │ 2        │ Egg-timer / general alert sound                          │
//  │ 10-53    │ Quarter-hour chime melodies — 5 sound sets, addressed as │
//  │          │ (sound_set * 10 + 10 + quarter), quarter = 0..3          │
//  │ 14       │ Short confirm "ding" — used by most front-panel buttons  │
//  │ 61       │ "Pille geneem!" — daily pill confirmation (PILLS_ENABLED)│
//  │ 62       │ "Medisyne tyd Ouma!" — medicine dose reminder            │
//  │ 63       │ Birthday song                                            │
//  │ 64       │ "Doktersafspraak vandag!" — doctor appointment reminder  │
//  └──────────┴──────────────────────────────────────────────────────────┘
//  Record your own MP3s and number them to match if you want different
//  wording, a different language, or different chime melodies.
//
