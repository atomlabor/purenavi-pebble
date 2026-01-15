var Clay = require('pebble-clay');
var clayConfig = require('./ClaySettings');
var clay = new Clay(clayConfig);

var target = { lat: 0, lon: 0, name: "Target" };
var settings = { unit: 0 };
var lastPos = null;
var isPaused = true;
var arrivedVibrated = false;

function calculateDistance(lat1, lon1, lat2, lon2) {
  var R = 6371;
  var dLat = (lat2-lat1)*Math.PI/180, dLon = (lon2-lon1)*Math.PI/180;
  var a = Math.sin(dLat/2)*Math.sin(dLat/2) + Math.cos(lat1*Math.PI/180) * Math.cos(lat2*Math.PI/180) * Math.sin(dLon/2)*Math.sin(dLon/2);
  return R * (2 * Math.atan2(Math.sqrt(a), Math.sqrt(1-a)));
}

function calculateBearing(lat1, lon1, lat2, lon2) {
  var dLon = (lon2-lon1)*Math.PI/180;
  var y = Math.sin(dLon) * Math.cos(lat2*Math.PI/180);
  var x = Math.cos(lat1*Math.PI/180)*Math.sin(lat2*Math.PI/180) - Math.sin(lat1*Math.PI/180)*Math.cos(lat2*Math.PI/180)*Math.cos(dLon);
  return (Math.atan2(y, x) * 180 / Math.PI + 360) % 360;
}

function updateWatch(forceText) {
  if (forceText) { Pebble.sendAppMessage({ 'ID_DIST': String(forceText) }); return; }
  if (!lastPos || target.lat === 0 || isPaused) return;

  var dKM = calculateDistance(lastPos.lat, lastPos.lon, target.lat, target.lon);
  var tBearing = calculateBearing(lastPos.lat, lastPos.lon, target.lat, target.lon);
  var distValue = (settings.unit === 1) ? (dKM * 0.621371) : dKM;

  var msg = {
    'ID_BEARING': Math.round(tBearing),
    'ID_GPS_HEADING': Math.round(lastPos.speed > 0.8 ? lastPos.heading : -1),
    'ID_ACCURACY': Math.round(lastPos.accuracy || 0)
  };

  if (dKM < 0.05) { 
    msg.ID_DIST = "GOAL!";
    if (!arrivedVibrated) { msg.ID_VIBE = 1; arrivedVibrated = true; }
  } else {
    msg.ID_DIST = target.name + "\n" + distValue.toFixed(2) + (settings.unit === 1 ? " mi" : " km");
    arrivedVibrated = false; 
  }
  Pebble.sendAppMessage(msg);
}

Pebble.addEventListener('ready', function() {
  var cached = localStorage.getItem('cached_target'); if (cached) target = JSON.parse(cached);
  var saved = localStorage.getItem('clay_settings');
  if (saved) {
    var c = JSON.parse(saved);
    settings.unit = parseInt(c.CONFIG_UNIT.value || c.CONFIG_UNIT || 0);
    target.name = String(c.CONFIG_TARGET_NAME.value || c.CONFIG_TARGET_NAME || "Target");
  }
  navigator.geolocation.watchPosition(function(pos) {
    if (pos.coords.latitude !== 0) {
      lastPos = { lat: pos.coords.latitude, lon: pos.coords.longitude, heading: pos.coords.heading, speed: pos.coords.speed, accuracy: pos.coords.accuracy };
      updateWatch();
    }
  }, null, { enableHighAccuracy: true });
});

Pebble.addEventListener('appmessage', function(e) {
  if (e.payload.ID_BUTTON === 1) { isPaused = !isPaused; updateWatch(isPaused ? "PAUSED" : "GPS RESUME"); }
  if (e.payload.ID_BUTTON === 2 && lastPos) {
    target = { lat: lastPos.lat, lon: lastPos.lon, name: "Pinned Pos" };
    localStorage.setItem('cached_target', JSON.stringify(target));
    isPaused = false; arrivedVibrated = false; updateWatch("PIN SET!");
  }
});

Pebble.addEventListener('webviewclosed', function(e) {
  if (e && e.response && e.response !== 'CANCELLED') {
    var config = JSON.parse(decodeURIComponent(e.response));
    localStorage.setItem('clay_settings', JSON.stringify(config));
    settings.unit = parseInt(config.CONFIG_UNIT.value !== undefined ? config.CONFIG_UNIT.value : config.CONFIG_UNIT);
    target.name = String(config.CONFIG_TARGET_NAME.value !== undefined ? config.CONFIG_TARGET_NAME.value : config.CONFIG_TARGET_NAME);
    var addr = config.CONFIG_ADDRESS.value !== undefined ? config.CONFIG_ADDRESS.value : config.CONFIG_ADDRESS;
    if (addr) { 
      var url = "https://nominatim.openstreetmap.org/search?q=" + encodeURIComponent(addr) + "&format=json&limit=1";
      var req = new XMLHttpRequest(); req.open('GET', url, true);
      req.setRequestHeader('User-Agent', 'PureNavi-v2');
      req.onload = function() {
        if (req.status === 200) {
          var json = JSON.parse(req.responseText);
          if (json && json.length > 0) {
            target.lat = parseFloat(json[0].lat); target.lon = parseFloat(json[0].lon);
            localStorage.setItem('cached_target', JSON.stringify(target));
            isPaused = false; arrivedVibrated = false; updateWatch("TARGET SET:\n" + target.name);
          } else { updateWatch("NOT FOUND"); }
        }
      };
      req.send(null);
    }
  }
});