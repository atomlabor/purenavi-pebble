# 🧭 PureNavi for Pebble (v2)

![PureNavi Header](https://raw.githubusercontent.com/atomlabor/purenavi-pebble/main/header.jpeg)

[![Pebble SDK](https://img.shields.io/badge/Pebble_SDK-4.0-orange.svg)](https://developer.rebble.io/)
[![Version](https://img.shields.io/badge/Version-2.0.0-blue.svg)]()
[![License](https://img.shields.io/badge/License-MIT-green.svg)]()

**PureNavi v2.0.0 (Golden Build)** is a minimalist, high-efficiency directional compass for the entire Pebble family. Instead of distracting you with complex maps, PureNavi delivers the purest form of navigation: an arrow and the distance to your target.

*"As the crow flies"*

---

## 🇩🇪 Deutsch

###  Kern-Features (v2.0.0)
* Hybrid-Navigation: Nutzt GPS-Heading bei Bewegung (>0,8 m/s) für höchste Genauigkeit und schaltet im Stand automatisch auf den gedämpften magnetischen Kompass um.
* Universal Geocoding: Gib einfach eine Adresse oder einen Ort (z. B. "Alexanderplatz, Berlin") in den Clay-Einstellungen ein.
* Zielankunft-Feedback: Sobald du den 50m-Umkreis erreichst, vibriert die Uhr und zeigt groß **"GOAL!"** an.
  
* Adaptives UI-Design
    * **Sidebar (Rectangular):** Eine ultrakompakte 15px-Leiste mit Trennlinien für Emery/Basalt/Diorite.
    * **Full-Screen (Round):** Die Sidebar wird auf der Pebble Chalk/Round 2 automatisch ausgeblendet, um das runde Display voll zu nutzen.
    * **Standard Dark-Mode:** Die App startet direkt im batterieschonenden Invert-Modus.


### Bedienung (Buttons)
* **OBEN (S):** Tracking Start / Pause (Standby).
* **MITTE (I):** Display manuell invertieren (Hell/Dunkel).
* **UNTEN (P):** **Instant-Pin:** Setzt deine aktuelle Position sofort als neues Ziel (ideal, um das Auto oder den Startpunkt wiederzufinden).

---

## 🇺🇸 English

###  Key Features (v2.0.0)
* Hybrid Logic: Uses GPS heading during movement (>0.8 m/s) for rock-solid precision and automatically switches to the damped magnetic compass when stationary.
* Universal Geocoding: No coordinates needed. Simply enter an address or POI (e.g., "Empire State Building") in the smartphone settings.
* Arrival Alert: Vibrates and displays **"GOAL!"** as soon as you are within 50 meters of your destination.
  
* Adaptive UI Design:
    * **Narrow Sidebar:** Ultra-slim 15px sidebar with separators for rectangular Pebbles.
    * **Round Optimization:** The sidebar is automatically hidden on Pebble Chalk/Round 2 to maximize the circular screen area.
    * **Default Dark Mode:** Starts inverted (white on black) by default for better visibility and battery life.


### Button Controls
* **UP (S):** Start / Pause GPS tracking.
* **SELECT (I):** Invert display (Toggle Black/White).
* **DOWN (P):** **Instant-Pin:** Immediately sets your current location as the target. Perfect for finding your car or hotel.

### Setup & Installation
1. Install the `.pbw` file via the Pebble/Rebble app.
2. Open the app settings (Clay) on your smartphone.
3. Enter your destination address and save.
4. **Onboarding:** If the screen is empty, follow the watch hint: Set a target or press **P** to pin your current location.
5. **Tip:** If you don't get a GPS fix, open Google Maps on your phone for a moment to trigger location services.

---

## Technical Details
* **SDK:** Built with Pebble SDK 4.0.
* **Platforms:** Supported on Aplite, Basalt, Chalk, Diorite, Emery and Flint.
* **Geocoding:** Powered by Nominatim (OpenStreetMap).
* **Filter:** Low-pass filtering and hysteresis for smooth arrow movement and stable text instructions.

---
Produced by [Atomlabor.de](https://atomlabor.de)
