// web_pages.h
#pragma once

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <title>ESP32 WebSocket + ESP-NOW Viewer</title>
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <style>
    body {
      font-family: system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
      background-color: #0f172a;
      color: #e5e7eb;
      display: flex;
      justify-content: center;
      align-items: center;
      min-height: 100vh;
      margin: 0;
    }
    .card {
      background-color: #111827;
      padding: 1.5rem;
      border-radius: 1rem;
      box-shadow: 0 10px 25px rgba(0, 0, 0, 0.4);
      max-width: 520px;
      width: 100%;
    }
    h1 {
      margin-top: 0;
      font-size: 1.6rem;
      margin-bottom: 0.25rem;
    }
    #status {
      font-size: 0.9rem;
      margin-bottom: 1rem;
      color: #9ca3af;
    }
    #log {
      background-color: #020617;
      border-radius: 0.5rem;
      padding: 0.75rem;
      height: 220px;
      overflow-y: auto;
      font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, "Liberation Mono", "Courier New", monospace;
      font-size: 0.85rem;
      border: 1px solid #1f2933;
      margin-bottom: 0.75rem;
    }
    .row {
      display: flex;
      gap: 0.5rem;
    }
    input[type="text"] {
      flex: 1;
      padding: 0.5rem 0.75rem;
      border-radius: 0.5rem;
      border: 1px solid #374151;
      background-color: #020617;
      color: #e5e7eb;
    }
    button {
      border: none;
      border-radius: 0.5rem;
      padding: 0.5rem 0.9rem;
      background-color: #10b981;
      color: white;
      cursor: pointer;
      font-weight: 500;
    }
    button:disabled {
      background-color: #4b5563;
      cursor: not-allowed;
    }
  </style>
</head>
<body>
  <div class="card">
    <h1>ESP32 Message Viewer</h1>
    <div id="status">Connecting to WebSocket...</div>
    <div id="log"></div>
    <div class="row">
      <input id="msgInput" type="text" placeholder="Test message to ESP32 (optional)..." />
      <button id="sendBtn" onclick="sendMessage()" disabled>Send</button>
    </div>
  </div>

  <script>
    let socket;
    const statusEl = document.getElementById('status');
    const logEl = document.getElementById('log');
    const inputEl = document.getElementById('msgInput');
    const sendBtn = document.getElementById('sendBtn');

    function log(message) {
      const time = new Date().toLocaleTimeString();
      logEl.innerHTML += "[" + time + "] " + message + "<br>";
      logEl.scrollTop = logEl.scrollHeight;
    }

    function connectWebSocket() {
      const gateway = `ws://${window.location.hostname}/ws`;
      socket = new WebSocket(gateway);

      socket.onopen = () => {
        statusEl.textContent = "WebSocket connected";
        sendBtn.disabled = false;
        log("WebSocket connected");
      };

      socket.onclose = () => {
        statusEl.textContent = "WebSocket disconnected - retrying...";
        sendBtn.disabled = true;
        log("WebSocket disconnected, retrying in 1s...");
        setTimeout(connectWebSocket, 1000);
      };

      socket.onerror = (error) => {
        log("WebSocket error");
        console.error(error);
      };

      socket.onmessage = (event) => {
        // ESP32 already prefixes messages (e.g. "ESP-NOW → ...")
        log(event.data);
      };
    }

    function sendMessage() {
      const text = inputEl.value.trim();
      if (!text || socket.readyState !== WebSocket.OPEN) return;
      socket.send(text);
      log("Browser → " + text);
      inputEl.value = "";
      inputEl.focus();
    }

    window.addEventListener('load', () => {
      connectWebSocket();
      inputEl.addEventListener('keyup', (event) => {
        if (event.key === 'Enter') {
          sendMessage();
        }
      });
    });
  </script>
</body>
</html>
)rawliteral";
