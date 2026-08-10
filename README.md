# Ouma Ria Smart Clock
### A Smart Companion for Independent Living

An ESP32-based bedside clock built for one purpose: help an aging parent live independently,
a little longer, a little safer — without a single feature added just because it was possible.

Every subsystem in this project exists because a real problem showed up in a real home first.
There is no blinking LED here that isn't also telling someone something they needed to know.

---

> **A note on timing:** the author is currently travelling for work and may be slower than
> usual to respond to issues or questions over the next couple of weeks. The project itself is
> stable — this is a timing note, not a sign of anything wrong. Bug reports and questions are
> still very welcome; just please be patient waiting for a reply.

---

## Table of Contents

1. [Why this exists](#why-this-exists)
2. [What it actually does](#what-it-actually-does)
3. [Documentation — start here](#documentation--start-here)
4. [Hardware overview](#hardware-overview)
5. [Getting started](#getting-started)
6. [First boot & Wi-Fi setup](#first-boot--wi-fi-setup)
7. [Pushover notifications (optional but recommended)](#pushover-notifications-optional-but-recommended)
8. [Making it yours](#making-it-yours)
9. [Repository contents](#repository-contents)
10. [Known limitations & honest caveats](#known-limitations--honest-caveats)
11. [Feedback & contact](#feedback--contact)
12. [Dedication](#dedication)
13. [License](#license)
14. [Final remarks](#final-remarks)

---

## Why this exists

This project began with a simple question:

> How can technology help us remain independent as we grow older?

The answer was not a single device, but a collection of practical features brought together
into one reliable system.

Ouma Ria Smart Clock combines accurate timekeeping, medication reminders, appointments,
birthdays, household reminders, kitchen timers, security monitoring, internet connectivity,
and spoken announcements into a single embedded system designed for everyday living.

It was developed over many months through continuous improvement, practical testing, and
real-life use in the author's own home — for his wife, affectionately known throughout the
family (and the build process) as "Ouma" (Afrikaans for Grandma, the name grandchildren gave
her). The interface itself stayed bilingual Afrikaans/English for exactly that reason: it was
built for a real person, in the language she actually speaks.

---

## What it actually does

- **Accurate timekeeping** — DS3231 RTC, automatically corrected over NTP when Wi-Fi is
  available, keeps correct time even through a full power cut thanks to its own coin-cell
  backup.
- **A wake alarm**, plus **three independent "Wekker" alarms** for anything else that needs
  a daily reminder — each with its own time, its own day-of-week schedule, and a gentle
  repeat-until-acknowledged behaviour so a single missed beep doesn't mean a missed reminder.
- **Daily medicine reminders**, up to four doses a day, with a visible LED and a spoken
  announcement — plus a **short-course scheduler** (two independent slots) for things like
  antibiotics that only run for a few days and then stop reminding on their own, and a
  one-tap log for anything taken outside the normal schedule.
- **Birthdays and doctor's appointments**, with an approaching-countdown light and a
  spoken reminder — doctor's appointments get an evening-before warning that also reaches a
  phone, since that one is worth getting right.
- **A general calendar and a small shopping-list notebook**, both managed from the web
  interface.
- **Kitchen helpers** — an egg timer, a smoker uptimer that survives a power cut and pushes
  a phone notification when it's done, and a braai timer with a built-in cooking-times and
  recipe reference.
- **A two-zone security system** — an always-on courtyard zone and a delayed-entry indoor
  zone, with a panic button, Pushover notifications, and a status LED that is deliberately
  never shared with any other alert, so there is never a moment's doubt about whether a
  flashing light means an emergency.
- **A full web interface** for configuration, plus an on-device LED and button reference so
  nobody ever has to guess what a colour or a button does.

---

## Documentation — start here

This repository includes four documents, each aimed at a different reader. All four are
provided as both PDF (read this) and DOCX (edit this, adapt this, translate this):

| Document | For | Files |
|---|---|---|
| **The Book** | The full story — why this exists, how it was built, every design decision explained | `OUMA_RIA_SMART_CLOCK.pdf` / `.docx` |
| **User Manual** | Anyone using the clock day to day — plain language, no jargon, colour-coded buttons | `Ouma_Ria_Smart_Clock_User_Manual.pdf` / `.docx` |
| **Technical Reference** | Anyone building one — pin tables, critical wiring warnings, NVS storage keys, web routes, default values | `Ouma_Ria_Smart_Clock_Technical_Reference.pdf` / `.docx` |
| **Screen & Web Guide** | A visual, page-by-page walkthrough of every screen and every web page, using real screenshots | `Ouma_Ria_Smart_Clock_Screen_and_Web_Guide.pdf` / `.docx` |

If you only read one thing before building: **the Technical Reference's "Critical Warnings"
section**. It covers the mistakes most likely to cause a confusing first boot, or worse,
damaged hardware.

---

## Hardware overview

- ESP32-WROOM-32 on an expansion board
- Four WS2812B 8×8 LED panels (256 pixels) for the main display, driven from one GPIO
- A separate 30-LED WS2812B status strip
- DS3231 RTC + BH1750 ambient light sensor, shared I²C bus
- DFPlayer Mini + speaker for spoken announcements and chimes
- Up to five AM312 PIR sensors across two security zones
- Twelve front-panel push-buttons
- An IRLZ44N MOSFET driving the siren/alarm output
- 12V supply with battery backup, stepped down to a regulated 5V rail

Full wiring detail, every pin assignment, and the reasoning behind each hardware choice is in
the **Technical Reference**. `OUMA CLOCK_ Wiring.pdf` and `My LED Matrix Case.jpg` are the
author's own wiring diagram and enclosure concept, included as a visual reference —
worth reading alongside the Technical Reference, not instead of it.

---

## Getting started

1. Read the **Technical Reference**'s Critical Warnings section before wiring anything.
2. Wire the hardware following the pin table in the Technical Reference.
3. Open `Ouma Ria Smart Clock.ino` in the Arduino IDE.
4. Set **Tools → Partition Scheme** to a scheme with two OTA app slots (e.g. "Minimal
   SPIFFS"). This is required for over-the-air updates to work at all — a "Huge APP" scheme
   will silently disable OTA with no obvious symptom other than uploads failing.
5. Set your own `OTA_PASSWORD` (search for `CHANGE_ME` in the Constants section).
6. Flash, then continue to first boot below.

---

## First boot & Wi-Fi setup

On first power-up, the clock opens its own Wi-Fi access point for initial setup. Connect to
it from a phone or computer and follow the on-screen instructions to join it to your home
network.

From then on, if Wi-Fi ever drops — a power cut, a router reboot — the clock retries on its
own in the background, with no button press needed. A manual restart (hold the WiFi Reset
button for 3 seconds) is available as a backup, but shouldn't normally be necessary.

---

## Pushover notifications (optional but recommended)

Several reminders — medicine, doctor's appointments, security alerts, the smoker and egg
timers — can also send a notification to a phone via [Pushover](https://pushover.net), a free
service.

1. Create a Pushover account and install the app.
2. Create an "Application" to get an API token.
3. Paste your own `PO_APP_TOKEN` and `PO_USER_KEY` into the firmware (search for
   `YOUR_PUSHOVER_APP_TOKEN_HERE` and `YOUR_PUSHOVER_USER_KEY_HERE`).

Without this, every reminder still works locally — the clock will still light up, speak, and
sound an alert. Pushover is simply the backup for when nobody is standing next to it.

---

## Making it yours

This project was built for one specific household, and it shows — the recipes, the braai
guide, the exact PIR sensor count. None of that is meant to be prescriptive. Fork it, rename
it, replace the recipes with your own family's, adjust the security zones to match your own
home. The Technical Reference's enclosure section is explicitly documented as *a* reference
design, not *the* official one — adapt it to whatever fabrication method you actually have
available.

---

## Repository contents

```
Ouma Ria Smart Clock.ino                              — the complete firmware, single file by design
OUMA_RIA_SMART_CLOCK.pdf / .docx                       — the Book: full story and design history
Ouma_Ria_Smart_Clock_User_Manual.pdf / .docx           — plain-language guide for everyday use
Ouma_Ria_Smart_Clock_Technical_Reference.pdf / .docx   — pin tables, warnings, reference data
Ouma_Ria_Smart_Clock_Screen_and_Web_Guide.pdf / .docx  — every screen and web page, illustrated
OUMA CLOCK_ Wiring.pdf                                 — author's wiring diagram
My LED Matrix Case.jpg                                 — author's enclosure concept drawing
LICENSE                                                — MIT (software) — see below
README.md                                              — this file
```

---

## Known limitations & honest caveats

- The security system is a genuine deterrent and notification layer, not a certified alarm
  system. Do not rely on it as a substitute for one where that distinction matters.
- The enclosure documented in the Technical Reference is the author's own design concept; it
  has not been fabricated as a complete case at the time of writing. The working prototype
  runs in a simpler enclosure while that design is refined.
- Some hardware quirks are specific to the author's own build — see the Technical Reference's
  "A Fault That Wasn't Where I Expected" note for a real example of a board-level defect that
  looked, for a long time, like a firmware bug.
- The recipes, braai times, and specific reminder wording reflect one household's real use.
  They are meant to be replaced, not preserved.

---

## Feedback & contact

Found a bug, have a question, or built your own version and want to share how it went? GitHub
Issues is the best place for anything code- or hardware-related, since it keeps the discussion
visible for the next person who runs into the same thing.

For anything else, or if GitHub isn't convenient: **hvermaak.projects@proton.me**

---

## Dedication

Built for Ouma Ria.

---

## License

The firmware is released under the [MIT License](LICENSE) — use it, modify it, build on it,
share it. See the LICENSE file for the full text, including a note on the hardware this
firmware controls (a siren, mains-adjacent wiring, and a security system) — the software
license covers the code; building and operating the physical device is at your own risk.

---

## Final remarks

This project took shape one real need at a time, over many months, with a great deal of
patient testing by the person it was actually built for. If it helps one other family the way
it has helped this one, that is the whole point of publishing it.
