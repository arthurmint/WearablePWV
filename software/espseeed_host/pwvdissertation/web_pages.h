// web_pages.h
#pragma once

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <title>ESP32 Gateway</title>
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <style>
    *, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      font-family: system-ui, -apple-system, sans-serif;
      background: #0f172a; color: #e5e7eb;
      display: flex; justify-content: center; align-items: flex-start;
      min-height: 100vh; padding: 2rem 1rem;
    }
    .card {
      background: #111827; border-radius: 1rem;
      box-shadow: 0 10px 25px rgba(0,0,0,.4);
      padding: 1.5rem; width: 100%; max-width: 520px;
    }
    h1 { font-size: 1.4rem; font-weight: 500; margin-bottom: 1.25rem; }

    /* ---- Latency widget ---- */
    .lat-box {
      background: #020617; border: 1px solid #1f2933;
      border-radius: .75rem; padding: .875rem 1rem;
      display: flex; align-items: center; gap: 1rem;
      margin-bottom: .75rem;
    }
    .lat-val {
      font-size: 2rem; font-weight: 500;
      font-family: ui-monospace, monospace;
      min-width: 88px; line-height: 1;
      transition: color .4s;
    }
    .lat-val.good { color: #10b981; }
    .lat-val.ok   { color: #f59e0b; }
    .lat-val.bad  { color: #ef4444; }
    .lat-side { flex: 1; display: flex; flex-direction: column; gap: 6px; }
    .lat-label { font-size: .8rem; color: #6b7280; }
    .lat-dot {
      display: inline-block; width: 7px; height: 7px;
      border-radius: 50%; background: #6b7280;
      margin-right: 5px; vertical-align: middle;
      transition: background .4s;
    }
    .lat-dot.good { background: #10b981; }
    .lat-dot.bad  { background: #ef4444; }
    .bar-track {
      background: #1f2933; border-radius: 4px;
      height: 5px; overflow: hidden;
    }
    .bar-fill {
      height: 100%; border-radius: 4px;
      transition: width .5s ease, background .4s;
    }

    .stats-row {
      display: flex; gap: .5rem; margin-bottom: .75rem;
    }
    .stat {
      flex: 1; background: #020617;
      border: 1px solid #1f2933; border-radius: .5rem;
      padding: .4rem .5rem; text-align: center;
    }
    .stat-val {
      font-size: .85rem; font-weight: 500;
      font-family: ui-monospace, monospace;
      color: #e5e7eb;
    }
    .stat-label { font-size: .7rem; color: #6b7280; margin-top: 1px; }

    /* ---- Message log ---- */
    #status { font-size: .85rem; color: #6b7280; margin-bottom: .75rem; }
    #log {
      background: #020617; border: 1px solid #1f2933;
      border-radius: .5rem; padding: .75rem;
      height: 200px; overflow-y: auto;
      font-family: ui-monospace, monospace;
      font-size: .8rem; margin-bottom: .75rem;
    }
    .row { display: flex; gap: .5rem; }
    input[type="text"] {
      flex: 1; padding: .5rem .75rem;
      border-radius: .5rem; border: 1px solid #374151;
      background: #020617; color: #e5e7eb;
    }
    button {
      border: none; border-radius: .5rem;
      padding: .5rem .9rem; background: #10b981;
      color: white; cursor: pointer; font-weight: 500;
    }
    button:disabled { background: #374151; cursor: not-allowed; }
  </style>
</head>
<body>
<div class="card">
  <h1>ESP32 Gateway</h1>

  <!-- Latency display -->
  <div class="lat-box">
    <div class="lat-val" id="latVal">— ms</div>
    <div class="lat-side">
      <div class="lat-label">
        <span class="lat-dot" id="latDot"></span>
        <span id="latStatus">Waiting for ping...</span>
      </div>
      <div class="bar-track">
        <div class="bar-fill" id="latBar" style="width:0%;background:#6b7280"></div>
      </div>
    </div>
  </div>

  <div class="stats-row">
    <div class="stat"><div class="stat-val" id="sMin">—</div><div class="stat-label">min ms</div></div>
    <div class="stat"><div class="stat-val" id="sAvg">—</div><div class="stat-label">avg ms</div></div>
    <div class="stat"><div class="stat-val" id="sMax">—</div><div class="stat-label">max ms</div></div>
    <div class="stat"><div class="stat-val" id="sLoss">0%</div><div class="stat-label">loss</div></div>
  </div>

  <!-- Message log -->
  <div id="status">Connecting...</div>
  <div id="log"></div>
  <div class="row">
    <input id="msgInput" type="text" placeholder="Test message..." />
    <button id="sendBtn" onclick="sendMessage()" disabled>Send</button>
  </div>
</div>

<script>
  let socket;
  const statusEl  = document.getElementById('status');
  const logEl     = document.getElementById('log');
  const inputEl   = document.getElementById('msgInput');
  const sendBtn   = document.getElementById('sendBtn');
  const latVal    = document.getElementById('latVal');
  const latBar    = document.getElementById('latBar');
  const latDot    = document.getElementById('latDot');
  const latStatus = document.getElementById('latStatus');

  // Rolling latency stats
  const latHistory = [];
  const MAX_HISTORY = 50;
  let lossCount = 0, totalPings = 0;

  function updateLatency(rtt) {
    totalPings++;
    latHistory.push(rtt);
    if (latHistory.length > MAX_HISTORY) latHistory.shift();

    const min = Math.min(...latHistory);
    const max = Math.max(...latHistory);
    const avg = latHistory.reduce((a, b) => a + b, 0) / latHistory.length;

    document.getElementById('sMin').textContent  = min.toFixed(1);
    document.getElementById('sAvg').textContent  = avg.toFixed(1);
    document.getElementById('sMax').textContent  = max.toFixed(1);
    document.getElementById('sLoss').textContent =
      totalPings > 0 ? ((lossCount / totalPings) * 100).toFixed(0) + '%' : '0%';

    // Color thresholds (RTT): <20 ms good, <60 ok, else bad
    const cls = rtt < 20 ? 'good' : rtt < 60 ? 'ok' : 'bad';
    const color = rtt < 20 ? '#10b981' : rtt < 60 ? '#f59e0b' : '#ef4444';

    latVal.textContent = rtt.toFixed(1) + ' ms';
    latVal.className   = 'lat-val ' + cls;
    latDot.className   = 'lat-dot ' + (cls === 'bad' ? 'bad' : 'good');
    latStatus.textContent = 'RTT · one-way ≈ ' + (rtt / 2).toFixed(1) + ' ms';

    // Bar scales 0–100 ms = 0–100%
    const pct = Math.min(rtt / 100 * 100, 100);
    latBar.style.width      = pct + '%';
    latBar.style.background = color;
  }

  function logLine(message) {
    const time = new Date().toLocaleTimeString();
    logEl.innerHTML += '[' + time + '] ' + message + '<br>';
    logEl.scrollTop = logEl.scrollHeight;
  }

  function connectWebSocket() {
    socket = new WebSocket('ws://' + window.location.hostname + '/ws');

    socket.onopen = () => {
      statusEl.textContent = 'Connected';
      sendBtn.disabled = false;
      logLine('WebSocket connected');
    };
    socket.onclose = () => {
      statusEl.textContent = 'Disconnected — retrying...';
      sendBtn.disabled = true;
      setTimeout(connectWebSocket, 1000);
    };
    socket.onerror = () => logLine('WebSocket error');

    socket.onmessage = (event) => {
      const data = event.data;

      // Latency message — update widget, don't print to log
      if (data.startsWith('LAT:')) {
        const rtt = parseFloat(data.substring(4));
        if (!isNaN(rtt)) updateLatency(rtt);
        return;
      }

      // Everything else → log
      logLine(data);
    };
  }

  function sendMessage() {
    const text = inputEl.value.trim();
    if (!text || socket.readyState !== WebSocket.OPEN) return;
    socket.send(text);
    logLine('Browser → ' + text);
    inputEl.value = '';
    inputEl.focus();
  }

  window.addEventListener('load', () => {
    connectWebSocket();
    inputEl.addEventListener('keyup', e => { if (e.key === 'Enter') sendMessage(); });
  });
</script>
</body>
</html>
)rawliteral";