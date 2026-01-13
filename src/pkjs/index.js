var Clay = require('pebble-clay');
var clayConfig = require('./ClaySettings');
var clay = new Clay(clayConfig);

var target = { name: "PureNavi", lat: 0, lon: 0 }; 
var settings = { unit: 0, lang: 0 }; 
var lastPos = null, manualPause = true;
var watchId = null; 

var TXT = {
  de: { search: "SUCHE...", gps: "GPS SUCHE...", pause: "PAUSE", arrived: "ZIEL\nERREICHT!", set: "ZIEL GESETZT:", err: "ADRESSE NICHT\nGEFUNDEN!", pin: "PIN GESETZT!" },
  en: { search: "SEARCH...", gps: "GPS SEARCH...", pause: "PAUSED", arrived: "DESTINATION\nREACHED!", set: "TARGET SET:", err: "ADDRESS NOT\nFOUND!", pin: "PIN SET!" }
};

function getTxt(key) {
  return (settings.lang === 1) ? TXT.en[key] : TXT.de[key];
}

// Aggressiver GPS-Watcher
function startGpsWatcher() {
  if (watchId) navigator.geolocation.clearWatch(watchId);
  
  watchId = navigator.geolocation.watchPosition(function(pos) {
    handleLocationUpdate(pos);
  }, function(err) {
    console.log("GPS Error: " + err.code);
  }, {
    enableHighAccuracy: true,
    maximumAge: 1000,   // Erzwingt frische Daten vom Sensor
    timeout: 15000      // Gibt dem Handy Zeit für den ersten Fix
  });
}

function handleLocationUpdate(pos) {
  var cur = { lat: pos.coords.latitude, lon: pos.coords.longitude };
  var speed = pos.coords.speed || 0;
  var myHeading = (pos.coords.heading !== null && speed > 0.5) ? pos.coords.heading : (lastPos ? lastPos.heading : 0);
  cur.heading = myHeading; lastPos = cur;

  if (manualPause) return;

  var dKM = calculateDistance(cur.lat, cur.lon, target.lat, target.lon);
  var tBearing = calculateBearing(cur.lat, cur.lon, target.lat, target.lon);
  var relBearing = (tBearing - myHeading + 360) % 360;

  var dText = (settings.unit === 1) ? (dKM * 0.621371).toFixed(2) + " mi" : dKM.toFixed(2) + " km";
  var distText = dText + "\n" + target.name;
  if (dKM < 0.05) distText = getTxt('arrived');

  var msg = { 'id_dist': distText, 'id_bearing': Math.round(relBearing) };
  if (dKM < 0.05 && dKM > 0.00) msg.id_vibe = 1;
  Pebble.sendAppMessage(msg);
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
        } else { Pebble.sendAppMessage({ 'id_dist': getTxt('err') }); }
      } catch(e) { console.log("JSON Error"); }
    }
  };
  req.send(null);
}

function calculateBearing(lat1, lon1, lat2, lon2) {
  var dLon = (lon2 - lon1) * Math.PI / 180;
  var y = Math.sin(dLon) * Math.cos(lat2 * Math.PI / 180);
  var x = Math.cos(lat1 * Math.PI / 180) * Math.sin(lat2 * Math.PI / 180) - 
          Math.sin(lat1 * Math.PI / 180) * Math.cos(lat2 * Math.PI / 180) * Math.cos(dLon);
  return (Math.atan2(y, x) * 180 / Math.PI + 360) % 360;
}

function calculateDistance(lat1, lon1, lat2, lon2) { 
  var R = 6371;
  var dLat = (lat2 - lat1) * Math.PI / 180;
  var dLon = (lon2 - lon1) * Math.PI / 180;
  var a = Math.sin(dLat / 2) * Math.sin(dLat / 2) + Math.cos(lat1 * Math.PI / 180) * Math.cos(lat2 * Math.PI / 180) * Math.sin(dLon / 2) * Math.sin(dLon / 2);
  return R * (2 * Math.atan2(Math.sqrt(a), Math.sqrt(1 - a)));
}

function updateSettings(config) {
  if (!config) return;
  var l = config.CONFIG_LANG || config['13'];
  var u = config.CONFIG_UNIT || config['12'];
  settings.lang = (l && typeof l === 'object') ? parseInt(l.value) : parseInt(l || 0);
  settings.unit = (u && typeof u === 'object') ? parseInt(u.value) : parseInt(u || 0);
  var name = config.CONFIG_TARGET_NAME || config['10'];
  if (name) target.name = (typeof name === 'object') ? name.value : name;
  localStorage.setItem('clay_settings', JSON.stringify(config));
  Pebble.sendAppMessage({ 'CONFIG_LANG': settings.lang }); 
}

Pebble.addEventListener('ready', function(e) {
  var savedSettings = localStorage.getItem('clay_settings');
  if (savedSettings) updateSettings(JSON.parse(savedSettings));
  var cachedTarget = localStorage.getItem('cached_target');
  if (cachedTarget) {
    var parsed = JSON.parse(cachedTarget);
    target.lat = parsed.lat; target.lon = parsed.lon; target.name = parsed.name;
  }
  
  // Sofort GPS triggern beim App-Start
  startGpsWatcher(); 
  Pebble.sendAppMessage({ 'id_dist': getTxt('pause') + "\n" + target.name });
});

Pebble.addEventListener('webviewclosed', function(e) {
  if (e && e.response && e.response !== 'CANCELLED') {
    var config = JSON.parse(decodeURIComponent(e.response));
    updateSettings(config);
    var addr = config.CONFIG_ADDRESS || config['11'];
    var addrString = (typeof addr === 'object') ? addr.value : addr;
    if (addrString && addrString.length > 2) {
      Pebble.sendAppMessage({ 'id_dist': getTxt('search') });
      resolveAddress(addrString, target.name);
    }
  }
});

Pebble.addEventListener('appmessage', function(e) {
  if (e.payload.id_button === 1) { 
    manualPause = !manualPause; 
    if (!manualPause) startGpsWatcher();
    Pebble.sendAppMessage({ 'id_dist': (manualPause ? getTxt('pause') : getTxt('gps')) });
  } 
  else if (e.payload.id_button === 2) { 
    if (lastPos) {
      target.lat = lastPos.lat; target.lon = lastPos.lon;
      target.name = (settings.lang === 1) ? "Saved Pin" : "Gesetzter Pin";
      localStorage.setItem('cached_target', JSON.stringify(target));
      Pebble.sendAppMessage({ 'id_dist': getTxt('pin') });
      manualPause = false; startGpsWatcher();
    }
  }
});