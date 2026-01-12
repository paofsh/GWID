# <ins>G</ins>eneral-purpose <ins>W</ins>iFi-connected <ins>I</ins>ndicator <ins>D</ins>evice (GWID) Overview


The General-purpose WiFi-connected Indicator Device (GWID) is an ESP8266/NodeMCU–based WiFi-connected NeoPixel indicator with open-source firmware, hardware design, documentation, and a Hubitat Elevation driver. GWID can flash, pulse, strobe, signal S.O.S., display a solid color or pixel patterns, and sound an audio alert. 

When connected to WiFi, the GWID can be controlled directly from a browser window or any device connected to the same network using HTTP protocols. A Hubitat Elevation home automation hub can be configured to use the GWID to provide remote visual and acoustic alerts based on apps and rules. Potential use cases include any situation where having a small, simple alert device alert is helpful (such as when a water leak is detected, garage door is open, etc.). 

GWID hardware/firmware and Hubitat driver were designed and tested using a Hubitat Elevation C-8 hub. 

---

## Repository Structure

```text
hardware/      → Schematics
firmware/      → Arduino ESP8266 NodeMCU Firmware source code  
hubitat/       → device driver (and related docs) desinged for use with Hubitat Elevation hub
docs/          → General build guides, usage docs, diagrams
examples/      → Example application (e.g., HTML page for a GWID control panel)
LICENSE-*      → Licenses for software, hardware, and documentation  
CHANGELOG.md   → Version history  
 
Features
•	General-purpose network-connected status/indicator device (GWID)
•	Control a NeoPixel ring using an ESP8266/NodeMCU over WiFi
•	Simple HTTP API for color/effects
•	Optional Hubitat Elevation integration via custom device driver
•	Open hardware design 
•	Designed for hobbyists to fork, extend, and customize
```


# Getting Started
1. Hardware
    - GWID schematics for the hardware build are in `hardware/`.
    <br>(Adapt these to your own form factor or PCB layout.)
2. Firmware (ESP8266 / NodeMCU)
     - Install the Arduino IDE
     - Install ESP8266 board support and the Adafruit NeoPixel (or equivalent) library.
     - Arduino code for the GWID firmware is located at `firmware/src/`.
     - Copy and paste latest firmware version to Arduino IDE.
     - Flash the firmware to your NodeMCU board.
3. Driver for use with Hubitat Elevation home automation hub
   - Custom driver source is in `hubitat/drivers/`.
   - See `hubitat/docs/hubitat-setup.md` for installing the driver, then adding and configuring the device on the hub.

## Licensing Overview
This project contains a hardware design, firmware, a device driver for use with Hubitat Elevation hub, and documentation. Each category is licensed under an open-source license appropriate for that type.


### 1. Hardware Design

License: CERN-OHL-P v2 (Permissive)

Applies to:

- `hardware/`
  
You may make, modify, manufacture, and distribute hardware based on these designs.
You are not required to open-source derivative designs, but you must preserve the
license notice when redistributing the design files.
See: [`LICENSE-HARDWARE`](/LICENSE-HARDWARE)
 
### 2. Firmware / Software

  License: MIT (permissive)
 
  Applies to:
  - `firmware/`
  - `hubitat/drivers/`

  You may use, modify, and redistribute the software (including commercial use) as long as you retain the copyright and license notice.
  See: [`LICENSE-SOFTWARE`](/LICENSE-SOFTWARE)
 
### 3. Documentation (including Hubitat docs)

License: Creative Commons Attribution-ShareAlike 4.0 International (CC BY-SA 4.0)

Applies to:

- `docs/`
- `hubitat/docs/`
  
You may copy, modify, and redistribute the documentation, including commercially,
as long as you provide attribution and distribute derivatives under the same license.
See: [`LICENSE-DOCS`](/LICENSE-DOCS)
 
### Licensing Summary Table

|Category	| Location	| License |
|:---|:---|:---|
|Hardware Designs|`hardware/`|CERN-OHL-P v2|
|Firmware (ESP8266)|`firmware/`|MIT
|Device Driver for Hubitat (Groovy)|`hubitat/drivers/`|	MIT|
|Documentation|	`docs/`, `hubitat/docs/`|	CC BY-SA 4.0|
 
## Contributing

Pull requests and forks are welcome!
- Keep firmware in `firmware/` and drivers in `hubitat/drivers/`.
- For hardware changes, update the relevant README(s) in `hardware/`.
- For docs, extend files under `docs/` or `hubitat/docs/`.
- For example applications (e.g., HTML-based control panel) add to `examples/`
Please follow the existing structure and include SPDX headers in new source files.

>[!IMPORTANT]
>Despite giving it a formal name the GWID project has been and remains a hobbiest (for fun) IOT activity, not a serious product development effort. In particular the GWID is NOT intended to be used in any safety-critical or sensitive applications where high reliabilty and strong cyber security are crucial. For example, GWID uses basic HTTP communication protocols, which means WiFi message traffic between it, the router, and other devices on that network such as a browser window or home automation hub is NOT encrypted. 

If you are interested in further background about the project, please take a look at [gwid-project-backstory.md](docs/gwid-project-backstory.md) in the `docs/` folder.
