const express = require('express');
const fs = require('fs');
const path = require('path');
const app = express();
const PORT = 3000;
const DATA_FILE = path.join(__dirname, '..', 'data', 'config.json');

app.use(express.json());
app.use(express.static(path.join(__dirname, '..', 'web')));

function loadConfig() {
  try { return JSON.parse(fs.readFileSync(DATA_FILE, 'utf8')); }
  catch (e) { return { pins: {} }; }
}
function saveConfig(c) {
  fs.writeFileSync(DATA_FILE, JSON.stringify(c, null, 2));
}

app.get('/api/config', (req, res) => {
  res.json({
    buttons: [
      {id:1, name:"LED 1", pin:"led1", icon:"1"},
      {id:2, name:"LED 2", pin:"led2", icon:"2"},
      {id:3, name:"Relay 1", pin:"relay1", icon:"3"},
      {id:4, name:"Relay 2", pin:"relay2", icon:"4"},
      {id:5, name:"Motor", pin:"motor", icon:"5"}
    ]
  });
});

app.get('/api/state', (req, res) => {
  const c = loadConfig();
  res.json(c.pins || {});
});

app.get('/api/toggle', (req, res) => {
  const pin = req.query.pin;
  if (!pin) return res.status(400).json({error:"Missing"});
  const c = loadConfig();
  if (!c.pins) c.pins = {};
  c.pins[pin] = c.pins[pin] ? 0 : 1;
  saveConfig(c);
  res.json({pin, state: c.pins[pin]});
});

app.get('/api/sensors', (req, res) => {
  res.json({
    temperature: (25 + Math.random() * 10).toFixed(1),
    humidity: (40 + Math.random() * 30).toFixed(1),
    unit_temp: "C",
    unit_hum: "%"
  });
});

app.listen(PORT, '0.0.0.0', () => {
  const os = require('os');
  let ip = 'localhost';
  const ifaces = os.networkInterfaces();
  for (const name of Object.keys(ifaces)) {
    for (const iface of ifaces[name]) {
      if (iface.family === 'IPv4' && !iface.internal) { ip = iface.address; break; }
    }
  }
  console.log('');
  console.log('============================================');
  console.log('  ESP32 SIMULATOR - READY');
  console.log('============================================');
  console.log('  Local:   http://localhost:' + PORT);
  console.log('  Network: http://' + ip + ':' + PORT);
  console.log('============================================');
  console.log('');
});
