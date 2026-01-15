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
        "defaultValue": "Configuration"
      },
      {
        "type": "select",
        "messageKey": "CONFIG_UNIT",
        "label": "Distance Units",
        "defaultValue": "0",
        "options": [
          { "label": "Metric (Kilometers)", "value": "0" },
          { "label": "Imperial (Miles)", "value": "1" }
        ]
      },
      {
        "type": "heading",
        "defaultValue": "Destination"
      },
      {
        "type": "input",
        "messageKey": "CONFIG_TARGET_NAME",
        "label": "Custom Label",
        "defaultValue": "Destination",
        "description": "Short name shown on top of the distance"
      },
      {
        "type": "input",
        "messageKey": "CONFIG_ADDRESS",
        "label": "Address or POI",
        "defaultValue": "",
        "description": "Enter an address (e.g. 'Oxford Street, London') and tap Save. The watch will confirm the exact location."
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "How to use"
      },
      {
        "type": "text",
        "defaultValue": "1. Enter a target and tap <b>Save</b>.<br>2. Check your watch: It will display <b>TARGET SET:</b> once resolved.<br>3. Press the top button <b>(S)</b> to start tracking.<br>4. The red arrow shows the <b>direct line</b> to your target using Compass & GPS.<br><br><i>Tip:</i> Press the bottom button <b>(P)</b> to pin your current location (perfect for finding your car)."
      },
      {
        "type": "text",
        "defaultValue": "v2.0.0 - produced by <a href='https://atomlabor.de'>Atomlabor.de</a>"
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save Settings"
  }
];