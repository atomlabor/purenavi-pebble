module.exports = [
  {
    "type": "heading",
    "defaultValue": "PureNavi Settings"
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "General / Allgemein"
      },
      {
        "type": "select",
        "messageKey": "CONFIG_LANG",
        "label": "Language / Sprache",
        "defaultValue": "0",
        "options": [
          { "label": "Deutsch", "value": "0" },
          { "label": "English", "value": "1" }
        ]
      },
      {
        "type": "select",
        "messageKey": "CONFIG_UNIT",
        "label": "Units / Einheiten",
        "defaultValue": "0",
        "options": [
          { "label": "Kilometer (km)", "value": "0" },
          { "label": "Miles (mi)", "value": "1" }
        ]
      },
      {
        "type": "heading",
        "defaultValue": "Destination / Ziel"
      },
      {
        "type": "input",
        "messageKey": "CONFIG_TARGET_NAME",
        "label": "Target Name",
        "defaultValue": "Home",
        "description": "Name shown on watch"
      },
      {
        "type": "input",
        "messageKey": "CONFIG_ADDRESS",
        "label": "Address / Adresse",
        "defaultValue": "",
        "description": "e.g. 'Berlin, Alexanderplatz'"
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "How to use / Anleitung"
      },
      {
        "type": "text",
        "defaultValue": "<b>EN:</b> 1. Enter an address and save. 2. Press the top button (S) to start. 3. The arrow shows the <b>direct line (as the crow flies)</b> to your target. <br><i>Tip:</i> If you don't get a GPS fix, open Google Maps on your phone for a moment. <br><br><b>DE:</b> 1. Adresse eingeben und speichern. 2. Oberen Button (S) drücken. 3. Der Pfeil zeigt die direkte <b>Luftlinie</b> zum Ziel. <br><i>Tipp:</i> Falls kein GPS-Fix kommt, öffne kurz Google Maps am Handy."
      },
      {
        "type": "text",
        "defaultValue": "produced by <a href='https://atomlabor.de'>Atomlabor.de</a>"
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save / Speichern"
  }
];