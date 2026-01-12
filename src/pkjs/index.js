var Clay = require('pebble-clay');
var clayConfig = require('./ClaySettings');
var clay = new Clay(clayConfig);

var target = { name: "Start...", lat: 0, lon: 0 }; 
var settings = { unit: 0, lang: 0 }; // 0=KM/DE, 1=MI/EN
var lastPos = null, manualPause = true;

// Hilfstexte für JS-seitige Nachrichten
var TXT = {
  de: { search: "SUCHE...", gps: "GPS SUCHE...", pause: "PAUSE", arrived: "ZIEL\nERREICHT!", set: "ZIEL GESETZT:", err: "ADRESSE NICHT\nGEFUNDEN!" },
  en: { search: "SEARCH...", gps: "GPS SEARCH...", pause: "PAUSED", arrived: "DESTINATION\nREACHED!", set: "TARGET SET:", err: "ADDRESS NOT\nFOUND!" }
};

function getTxt(key) {
  return (settings.lang == 1) ? TXT.en[key] : TXT.de[key];
}

function resolveAddress(address, label) {
  var url = "https://nominatim.openstreetmap.org/search?q=" + encodeURIComponent(address) + "&format=json&limit=1";
  
  var req = new XMLHttpRequest();
  req.open('GET', url, true);
  req.onload = function() {
    if (req.readyState === 4 && req.status === 200) {
      try {
        var json = JSON.parse(req.responseText);
        if (json && json.length > 0) {
          target.lat = parseFloat(json[0].lat);
          target.lon = parseFloat(json[0].lon);
          target.name = label; 
          localStorage.setItem('cached_target', JSON.stringify(target));
          Pebble.sendAppMessage({ 'id_dist': getTxt('set') + "\n" + target.name });
        } else {
          Pebble.sendAppMessage({ 'id_dist': getTxt('err') });
        }
      } catch(e) {}
    }
  };
  req.send(null);
}

function calculateBearing(lat1, lon1, lat2, lon2) {
  var dLon = (lon2 - lon1) * Math.PI / 180;
  var y = Math.sin(dLon) * Math.cos(lat2 * Math.PI / 180);
  var x = Math.cos(lat1 * Math.PI / 180) * Math.sin(lat2 * Math.PI / 180) - Math.sin(lat1 * Math.PI / 180) * Math.cos(lat2 * Math.PI / 180) * Math.cos(dLon);
  return (Math.atan2(y, x) * 180 / Math.PI + 360) % 360;
}

function calculateDistance(lat1, lon1, lat2, lon2) { 
  if (!lat1 || !lon1 || !lat2 || !lon2) return 0;
  var R = 6371; // KM
  var dLat = (lat2 - lat1) * Math.PI / 180;
  var dLon = (lon2 - lon1) * Math.PI / 180;
  var a = Math.sin(dLat / 2) * Math.sin(dLat / 2) + Math.cos(lat1 * Math.PI / 180) * Math.cos(lat2 * Math.PI / 180) * Math.sin(dLon / 2) * Math.sin(dLon / 2);
  var d = R * (2 * Math.atan2(Math.sqrt(a), Math.sqrt(1 - a)));
  return d;
}

function updateSettings(newSettings) {
  if (!newSettings) return;
  // Sprache und Einheit laden (Standard 0)
  var l = newSettings.CONFIG_LANG || newSettings['13'];
  var u = newSettings.CONFIG_UNIT || newSettings['12'];
  settings.lang = (l && l.value) ? parseInt(l.value) : 0;
  settings.unit = (u && u.value) ? parseInt(u.value) : 0;
  
  // Ziel laden
  var name = newSettings.CONFIG_TARGET_NAME || newSettings['10'];
  if (name) target.name = (typeof name === 'object') ? name.value : name;
  
  // An C-Code senden (für Richtungstexte)
  Pebble.sendAppMessage({ 'CONFIG_LANG': settings.lang }); 
}

Pebble.addEventListener('ready', function(e) {
  var cached = localStorage.getItem('clay_settings');
  if (cached) updateSettings(JSON.parse(cached));
  
  var cachedTarget = localStorage.getItem('cached_target');
  if (cachedTarget) target = Object.assign(target, JSON.parse(cachedTarget));

  Pebble.sendAppMessage({ 'id_dist': getTxt('pause') + "\n" + target.name });
});

Pebble.addEventListener('webviewclosed', function(e) {
  if (e && e.response && e.response !== 'CANCELLED') {
    try {
      var s = JSON.parse(decodeURIComponent(e.response));
      updateSettings(s);
      
      var addr = s.CONFIG_ADDRESS || s['11'];
      var addrString = (typeof addr === 'object') ? addr.value : addr;

      if (addrString && addrString.length > 2) {
        Pebble.sendAppMessage({ 'id_dist': getTxt('search') });
        resolveAddress(addrString, target.name);
      } else {
        // Nur Settings update, keine neue Adresse
        Pebble.sendAppMessage({ 'id_dist': getTxt('pause') + "\n" + target.name });
      }
    } catch (err) {}
  }
});

Pebble.addEventListener('appmessage', function(e) {
  if (e.payload.id_button === 1) { 
    manualPause = !manualPause; 
    Pebble.sendAppMessage({ 'id_dist': (manualPause ? getTxt('pause') : getTxt('gps')) });
  }
});

navigator.geolocation.watchPosition(function(pos) {
  if (manualPause) return;
  var cur = { lat: pos.coords.latitude, lon: pos.coords.longitude };
  var speed = pos.coords.speed || 0;
  
  var distKM = calculateDistance(cur.lat, cur.lon, target.lat, target.lon);
  var tBearing = calculateBearing(cur.lat, cur.lon, target.lat, target.lon);
  var myHeading = (pos.coords.heading !== null && speed > 0.5) ? pos.coords.heading : (lastPos ? lastPos.heading : 0);
  var relBearing = (tBearing - myHeading + 360) % 360;
  cur.heading = myHeading; lastPos = cur;

  // Einheiten Umrechnung
  var distStr = "";
  if (settings.unit === 1) { // Miles
    var miles = distKM * 0.621371;
    distStr = miles.toFixed(2) + " mi";
  } else { // KM
    distStr = distKM.toFixed(2) + " km";
  }

  var distText = distStr + "\n" + target.name;
  if (distKM < 0.05) distText = getTxt('arrived');

  var msg = { 'id_dist': distText, 'id_bearing': Math.round(relBearing) };
  if (distKM < 0.05 && distKM > 0.00) msg.id_vibe = 1;
  Pebble.sendAppMessage(msg);
}, null, { enableHighAccuracy: true, maximumAge: 5000, timeout: 10000 });