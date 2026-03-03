/*
 * ESP32 RS232 Web Serial Monitor — miniPET Edition
 * 
 * Hardware:
 *   ESP32 3V3    →  VCC  modulo HW-044
 *   ESP32 GND    →  GND  modulo HW-044
 *   ESP32 GPIO16 →  RXD  modulo HW-044
 *   ESP32 GPIO17 →  TXD  modulo HW-044
 *
 * Requested Libraries:
 *   - ESPAsyncWebServer  by Mathieu Carbou
 *   - AsyncTCP           by Mathieu Carbou
 */

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// ─── CONFIGURAZIONE ───────────────────────────────────────────────
const char* WIFI_SSID     = "3WIFI_SSID";      
const char* WIFI_PASSWORD = "WIFI_SSID";

IPAddress local_IP(192, 168, 1, 150);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8); 

#define RS232_BAUD   19200
#define RS232_RX_PIN 16
#define RS232_TX_PIN 17
#define MAX_LOG_LINES 50

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

String logBuffer[MAX_LOG_LINES];
int logHead  = 0;
int logCount = 0;
String currentLine = "";
unsigned long lastRxTime = 0;

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="it">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>miniPET 65C02 Terminal</title>
<style>
@import url('https://fonts.googleapis.com/css2?family=VT323&display=swap');
:root{
  --lb:#00008a;--lt:#a8d8ff;--ltb:#e0f4ff;--lg:rgba(100,180,255,0.5);
  --kt:#3a3028;
}
*{box-sizing:border-box;margin:0;padding:0;}
body{min-height:100vh;display:flex;align-items:center;justify-content:center;
  background:radial-gradient(ellipse at center,#2a2520 0%,#0e0c08 100%);
  font-family:'VT323',monospace;padding:20px;}
.pet{display:flex;flex-direction:column;align-items:center;
  filter:drop-shadow(0 20px 40px rgba(0,0,0,0.8));}
.pet-top{
  background:linear-gradient(170deg,#ede5d5 0%,#d8cfbc 50%,#c8bfa8 100%);
  border-radius:12px 12px 0 0;padding:18px 22px 14px;width:520px;
  box-shadow:inset 0 2px 4px rgba(255,255,255,0.5),2px 0 4px rgba(0,0,0,0.2);}
.cbm-badge{display:flex;justify-content:space-between;align-items:center;margin-bottom:12px;}
.cbm-logo{font-size:22px;color:#5a4a30;letter-spacing:2px;text-shadow:0 1px 0 rgba(255,255,255,0.4);}
.cbm-model{font-size:14px;color:#8a7a58;letter-spacing:1px;}
.lcd-frame{
  background:#111;border-radius:6px;padding:12px;
  box-shadow:inset 0 0 8px rgba(0,0,0,0.9),0 0 0 1px #333,0 3px 8px rgba(0,0,0,0.5);}
.lcd-frame::before{content:'';position:absolute;top:6px;left:12px;right:60%;
  height:2px;background:linear-gradient(90deg,transparent,rgba(255,255,255,0.08),transparent);border-radius:2px;}
.lcd-screen{
  background:var(--lb);width:420px;height:400px;padding:6px 8px;
  position:relative;overflow:hidden;border-radius:3px;
  box-shadow:inset 0 0 20px rgba(0,0,30,0.8),0 0 12px rgba(0,80,200,0.3);}
.lcd-screen::before{content:'';position:absolute;inset:0;
  background-image:repeating-linear-gradient(0deg,transparent 0px,transparent 3px,rgba(0,0,30,0.18) 3px,rgba(0,0,30,0.18) 4px),
  repeating-linear-gradient(90deg,transparent 0px,transparent 5px,rgba(0,0,30,0.08) 5px,rgba(0,0,30,0.08) 6px);
  pointer-events:none;z-index:4;}
.lcd-screen::after{content:'';position:absolute;top:0;left:0;right:0;height:40%;
  background:linear-gradient(180deg,rgba(150,200,255,0.05) 0%,transparent 100%);
  pointer-events:none;z-index:5;}
.lcd-content{position:relative;z-index:3;display:flex;flex-direction:column;}
.lcd-row{display:flex;height:24px;align-items:center;}
.lcd-char{
  width:21px;height:24px;display:flex;align-items:center;justify-content:center;
  font-size:19px;line-height:1;color:var(--lt);flex-shrink:0;
  text-shadow:0 0 3px var(--lg),0 0 8px rgba(100,180,255,0.3);}
.lcd-char.bright{color:var(--ltb);text-shadow:0 0 4px rgba(200,240,255,0.8),0 0 10px rgba(100,180,255,0.5);}
.lcd-cursor{position:relative;}
.lcd-cursor::after{content:'';position:absolute;bottom:1px;left:2px;right:2px;height:2px;background:var(--lt);box-shadow:0 0 4px var(--lg);animation:lcdblink 1s step-end infinite;}
.lcd-cursor-char{color:var(--lt)!important;}
@keyframes lcdblink{0%,49%{opacity:1;}50%,100%{opacity:0;}}
.pet-info{display:flex;justify-content:space-between;align-items:center;margin-top:10px;padding:0 2px;}
.status-led{display:flex;align-items:center;gap:6px;font-size:13px;color:#8a7a58;letter-spacing:1px;}
.led{width:8px;height:8px;border-radius:50%;background:#333;box-shadow:inset 0 1px 0 rgba(255,255,255,0.2);}
.led.green{background:#44dd44;box-shadow:0 0 6px #44dd44;}
.led.red{background:#ff3333;box-shadow:0 0 6px #ff3333;}
.pet-specs{font-size:13px;color:#8a7a58;letter-spacing:1px;}
/* TASTIERA */
.pet-kbd{
  background:linear-gradient(170deg,#ccc4b0 0%,#b8b0a0 50%,#a8a090 100%);
  border-radius:0 0 10px 10px;padding:14px 22px 18px;width:520px;
  box-shadow:inset 0 2px 0 rgba(255,255,255,0.2);}
.kbd-row{display:flex;gap:5px;margin-bottom:5px;justify-content:center;}
.key{
  background:linear-gradient(180deg,#d8d0c0 0%,#c0b8a8 40%,#b0a898 100%);
  border-radius:4px 4px 3px 3px;
  box-shadow:0 3px 0 #8a8070,0 4px 6px rgba(0,0,0,0.4),inset 0 1px 0 rgba(255,255,255,0.4);
  cursor:pointer;display:flex;flex-direction:column;align-items:center;justify-content:center;
  transition:transform 0.05s,box-shadow 0.05s;user-select:none;}
.key:active{transform:translateY(2px);box-shadow:0 1px 0 #8a8070,0 2px 3px rgba(0,0,0,0.4);}
.key-label{font-size:14px;color:var(--kt);letter-spacing:0.5px;line-height:1;}
.key-sub{font-size:9px;color:#b06020;line-height:1;font-weight:bold;}
.key-fn{width:37px;height:36px;}
.key-med{width:52px;height:36px;}
.key-wide{width:80px;height:36px;}
.key-space{flex:1;height:36px;max-width:180px;}
.kbd-input-row{display:flex;gap:6px;align-items:center;margin-top:8px;padding:0 2px;}
.pet-real-input{
  flex:1;background:#181410;border:1px solid #4a4030;border-radius:3px;
  color:var(--ltb);font-family:'VT323',monospace;font-size:18px;
  padding:4px 8px;outline:none;letter-spacing:1px;caret-color:var(--lt);}
.pet-real-input:focus{border-color:#8080c0;}
.pet-real-input::placeholder{color:#4a4860;}
.nl-sel{background:#181410;border:1px solid #4a4030;border-radius:3px;
  color:#a090c0;font-family:'VT323',monospace;font-size:15px;padding:4px 5px;}
.btn-sm{background:linear-gradient(180deg,#6060a0 0%,#404080 100%);border:none;border-radius:3px;
  color:#d0d8ff;font-family:'VT323',monospace;font-size:15px;padding:5px 10px;
  cursor:pointer;letter-spacing:1px;box-shadow:0 2px 0 #202060;}
.btn-sm:hover{background:linear-gradient(180deg,#7070c0 0%,#5050a0 100%);}
.btn-sm:active{transform:translateY(1px);box-shadow:0 1px 0 #202060;}
.btn-clr{background:linear-gradient(180deg,#804040 0%,#602020 100%);box-shadow:0 2px 0 #301010;}
.btn-clr:hover{background:linear-gradient(180deg,#a05050 0%,#803030 100%);}
</style>
</head>
<body>
<div class="pet">
  <div class="pet-top">
    <div class="cbm-badge">
      <span class="cbm-logo">&#9679; miniPET <small id="ip-addr" style="font-size:12px;opacity:0.8;margin-left:10px;">IP: ---</small></span>
      <span class="cbm-model">W65C02S &#8226; 1MHz &#8226; 32K</span>
    </div>
    <div class="lcd-frame">
      <div class="lcd-screen">
        <div class="lcd-content" id="lcd"></div>
      </div>
    </div>
    <div class="pet-info">
      <div class="status-led">
        <div class="led" id="led"></div>
        <span id="stxt">OFFLINE</span>
      </div>
      <div class="pet-specs" id="sbaud">19200 BAUD &#8226; 8N1</div>
    </div>
  </div>

  <div class="pet-kbd">
    <div class="kbd-input-row">
      <input class="pet-real-input" id="inp" type="text" placeholder="ENTER COMMAND..." autocomplete="off" maxlength="80"/>
      <button class="btn-sm" id="bsnd">SEND</button>
      <button class="btn-sm btn-clr" id="bcl">CLR</button>
    </div>
    <div class="kbd-row" style="margin-top:10px;">
      <div class="key key-fn"><span class="key-sub">!</span><span class="key-label">1</span></div>
      <div class="key key-fn"><span class="key-sub">"</span><span class="key-label">2</span></div>
      <div class="key key-fn"><span class="key-sub">#</span><span class="key-label">3</span></div>
      <div class="key key-fn"><span class="key-sub">$</span><span class="key-label">4</span></div>
      <div class="key key-fn"><span class="key-sub">%</span><span class="key-label">5</span></div>
      <div class="key key-fn"><span class="key-sub">&amp;</span><span class="key-label">6</span></div>
      <div class="key key-fn"><span class="key-sub">'</span><span class="key-label">7</span></div>
      <div class="key key-fn"><span class="key-sub">(</span><span class="key-label">8</span></div>
      <div class="key key-fn"><span class="key-sub">)</span><span class="key-label">9</span></div>
      <div class="key key-fn"><span class="key-sub">_</span><span class="key-label">0</span></div>
      <div class="key key-fn"><span class="key-sub">+</span><span class="key-label">-</span></div>
      <div class="key key-fn"><span class="key-sub">*</span><span class="key-label">=</span></div>
      <div class="key key-wide" id="retkey"><span class="key-label">RETURN</span></div>
    </div>
    <div class="kbd-row">
      <div class="key key-med"><span class="key-label">CTRL</span></div>
      <div class="key key-fn"><span class="key-sub">!</span><span class="key-label">Q</span></div>
      <div class="key key-fn"><span class="key-sub">"</span><span class="key-label">W</span></div>
      <div class="key key-fn"><span class="key-sub">#</span><span class="key-label">E</span></div>
      <div class="key key-fn"><span class="key-sub">$</span><span class="key-label">R</span></div>
      <div class="key key-fn"><span class="key-sub">%</span><span class="key-label">T</span></div>
      <div class="key key-fn"><span class="key-sub">&amp;</span><span class="key-label">Y</span></div>
      <div class="key key-fn"><span class="key-sub">'</span><span class="key-label">U</span></div>
      <div class="key key-fn"><span class="key-sub">(</span><span class="key-label">I</span></div>
      <div class="key key-fn"><span class="key-sub">)</span><span class="key-label">O</span></div>
      <div class="key key-fn"><span class="key-sub">_</span><span class="key-label">P</span></div>
      <div class="key key-fn"><span class="key-sub">+</span><span class="key-label">@</span></div>
    </div>
    <div class="kbd-row">
      <div class="key key-fn"><span class="key-sub">1</span><span class="key-label">A</span></div>
      <div class="key key-fn"><span class="key-sub">2</span><span class="key-label">S</span></div>
      <div class="key key-fn"><span class="key-sub">3</span><span class="key-label">D</span></div>
      <div class="key key-fn"><span class="key-sub">4</span><span class="key-label">F</span></div>
      <div class="key key-fn"><span class="key-sub">5</span><span class="key-label">G</span></div>
      <div class="key key-fn"><span class="key-sub">6</span><span class="key-label">H</span></div>
      <div class="key key-fn"><span class="key-sub">7</span><span class="key-label">J</span></div>
      <div class="key key-fn"><span class="key-sub">8</span><span class="key-label">K</span></div>
      <div class="key key-fn"><span class="key-sub">9</span><span class="key-label">L</span></div>
      <div class="key key-fn"><span class="key-sub">0</span><span class="key-label">:</span></div>
      <div class="key key-fn"><span class="key-sub">.</span><span class="key-label">;</span></div>
      <div class="key key-fn"><span class="key-sub">/</span><span class="key-label">,</span></div>
    </div>
    <div class="kbd-row">
      <div class="key key-med"><span class="key-label">SHIFT</span></div>
      <div class="key key-fn"><span class="key-label">Z</span></div>
      <div class="key key-fn"><span class="key-label">X</span></div>
      <div class="key key-fn"><span class="key-label">C</span></div>
      <div class="key key-fn"><span class="key-label">V</span></div>
      <div class="key key-fn"><span class="key-label">B</span></div>
      <div class="key key-fn"><span class="key-label">N</span></div>
      <div class="key key-fn"><span class="key-label">M</span></div>
      <div class="key key-fn"><span class="key-label">.</span></div>
      <div class="key key-fn"><span class="key-label">/</span></div>
      <div class="key key-fn"><span class="key-label">*</span></div>
      <div class="key key-med"><span class="key-label">SHIFT</span></div>
    </div>
    <div class="kbd-row">
      <div class="key key-fn"><span class="key-label">ESC</span></div>
      <div class="key key-space"><span class="key-label">SPACE</span></div>
      <div class="key key-fn"><span class="key-label">DEL</span></div>
    </div>
  </div>
</div>

<script>
var COLS=20,ROWS=16,display=[],curRow=0,curCol=0,lineCount=0;
function emptyRow(){var r=[];for(var i=0;i<COLS;i++)r.push(' ');return r;}
function initDisplay(){display=[];for(var i=0;i<ROWS;i++)display.push(emptyRow());curRow=0;curCol=0;}
function scrollUp(){display.shift();display.push(emptyRow());if(curRow>0)curRow--;}
function newLine(){curCol=0;curRow++;if(curRow>=ROWS)scrollUp();}
function putInline(str){
  str=(str||'');
  for(var i=0;i<str.length;i++){
    if(curCol>=COLS)newLine();
    display[curRow][curCol]=str[i];curCol++;
  }
  renderLcd();
}
function putString(str){
  //str=(str||'').toUpperCase();
  str = (str || '');
  if(!str.length){newLine();renderLcd();return;}
  while(str.length>0){
    var chunk=str.slice(0,COLS-curCol);
    str=str.slice(chunk.length);
    for(var i=0;i<chunk.length;i++){display[curRow][curCol]=chunk[i];curCol++;}
    if(str.length>0)newLine();
  }
  newLine();lineCount++;renderLcd();
}
function renderLcd(){
  var lcd=document.getElementById('lcd');lcd.innerHTML='';
  for(var r=0;r<ROWS;r++){
    var row=document.createElement('div');row.className='lcd-row';
    for(var c=0;c<COLS;c++){
      var cell=document.createElement('div');
      if(r===curRow&&c===curCol){
        cell.className='lcd-char lcd-cursor';
        var s=document.createElement('span');s.className='lcd-cursor-char';
        s.textContent=display[r][c];cell.appendChild(s);
      }else{
        cell.className='lcd-char';
        if(r===0&&display[r][c]!==' ')cell.classList.add('bright');
        cell.textContent=display[r][c];
      }
      row.appendChild(cell);
    }
    lcd.appendChild(row);
  }
}
var ws,dot=document.getElementById('led'),stxt=document.getElementById('stxt');
function conn(){
  ws=new WebSocket('ws://'+location.host+'/ws');
  ws.onopen=function(){
    dot.className='led green';
    stxt.textContent='CONNECTED';
    document.getElementById('ip-addr').textContent = 'IP: ' + location.hostname;
    //putString('miniPET 65C02 READY.');
    //putString('HOST: '+location.hostname);
    };
  ws.onclose=function(){dot.className='led red';stxt.textContent='OFFLINE';
    putString('CONN LOST...');setTimeout(conn,2000);};
  ws.onerror=function(){dot.className='led red';stxt.textContent='ERROR';};
  ws.onmessage=function(e){
    var d=JSON.parse(e.data);
    if(d.type==='rx')putString(d.msg);
    else if(d.type==='history')d.lines.forEach(function(l){putString(l);});
    else if(d.type==='baud')document.getElementById('sbaud').textContent=d.value+' BAUD \u2022 8N1';
    else if(d.type==='reset'){initDisplay();lineCount=0;renderLcd();}
    else if(d.type==='cursor'){putInline(d.msg);}
  };
}

function txCR(){
  if(ws&&ws.readyState===1) ws.send(JSON.stringify({type:'cr'}));
}
function txChar(ch){
  if(ws&&ws.readyState===1) ws.send(JSON.stringify({type:"tx",msg:ch}));
}
document.getElementById('inp').addEventListener('keydown',function(e){
  if(e.ctrlKey||e.altKey||e.metaKey) return;
  e.preventDefault();
  if(e.key==='Enter'){  txCR(); return; }
  if(e.key==='Backspace'){ txChar('\x08'); return; }
  if(e.key==='Escape'){    txChar('\x1B'); return; }
  if(e.key.length===1)     txChar(e.key.toUpperCase());
});
document.getElementById('bsnd').onclick=function(){
  var t=document.getElementById('inp').value;
  document.getElementById('inp').value='';
  if(!t||!ws||ws.readyState!==1)return;
  ws.send(JSON.stringify({type:"tx",msg:t}));
};
document.getElementById('retkey').onclick=function(){ txCR(); };
document.getElementById('bcl').onclick=function(){initDisplay();lineCount=0;renderLcd();};
// Tasti fisici cliccabili
document.querySelectorAll('.key-fn,.key-med,.key-wide,.key-space').forEach(function(k){
  if(k.id==='retkey'||k.id==='bcl'||k.id==='bsnd') return;
  var lbl=k.querySelector('.key-label');
  if(!lbl)return;
  var ch=lbl.textContent.trim();
  if(ch==='SPACE'){ k.onclick=function(){ txChar(' '); }; return; }
  if(ch==='ESC'){   k.onclick=function(){ txChar('\x1B'); }; return; }
  if(ch==='DEL'){   k.onclick=function(){ txChar('\x08'); }; return; }
  if(ch.length===1){ k.onclick=function(){ txChar(ch); }; }
});
initDisplay();
//putString('**miniPET TERMINAL**');
//putString('65C02 @ 1MHZ / 32K');
renderLcd();
conn();
setInterval(renderLcd,600);
</script>
</body>
</html>
)rawliteral";

// ─── WEBSOCKET ────────────────────────────────────────────────────
void addLog(const String& line) {
  logBuffer[logHead] = line;
  logHead = (logHead + 1) % MAX_LOG_LINES;
  if (logCount < MAX_LOG_LINES) logCount++;
  String e = line;
  e.replace("\\","\\\\"); e.replace("\"","\\\""); e.replace("\r",""); e.replace("\n","");
  ws.textAll("{\"type\":\"rx\",\"msg\":\"" + e + "\"}");
}

void sendHistory(AsyncWebSocketClient* client) {
  int start = (logCount < MAX_LOG_LINES) ? 0 : logHead;
  for (int i = 0; i < logCount; i++) {
    int idx = (start + i) % MAX_LOG_LINES;
    String e = logBuffer[idx];
    e.replace("\\","\\\\"); e.replace("\"","\\\""); e.replace("\r",""); e.replace("\n","");
    client->text("{\"type\":\"rx\",\"msg\":\"" + e + "\"}");
    delay(5);
  }
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
               void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_DATA) {
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->opcode == WS_TEXT) {
      data[len] = 0;
      String input = (char*)data;
      

      if (input.indexOf("\"type\":\"cr\"") >= 0) {
        Serial2.write(0x0D);
        return;
      }
      int start = input.indexOf("\"msg\":\"") + 7;
      int end = input.lastIndexOf("\"");
      
      if (start > 6 && end > start) {
        String cleanCmd = input.substring(start, end);
        // De-escape JSON
        cleanCmd.replace("\\\"", "\"");   // \" -> "
        cleanCmd.replace("\\n",  "\n");    // \n -> LF
        cleanCmd.replace("\\r",  "\r");    // \r -> CR
        cleanCmd.replace("\\x08","\x08");  // \x08 -> BS
        cleanCmd.replace("\\x1B","\x1B");  // \x1B -> ESC

        for (int i = 0; i < cleanCmd.length(); i++) {
          Serial2.write((uint8_t)cleanCmd[i]);
          delay(40);
        }
      }
    }
  }
}
void setup() {
  Serial.begin(115200); delay(200);
  Serial.println("\n=== miniPET RS232 Terminal ===");
  Serial.printf("[HEAP] %d\n", ESP.getFreeHeap());
  Serial2.begin(RS232_BAUD, SERIAL_8N1, RS232_RX_PIN, RS232_TX_PIN);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.printf("[WiFi] Connessione a %s", WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.printf("\n[WiFi] IP: %s\n", WiFi.localIP().toString().c_str());
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* req){
    req->send_P(200, "text/html", INDEX_HTML);
  });
  server.begin();
  Serial.printf("[HTTP] http://%s/\n", WiFi.localIP().toString().c_str());
}

void loop() {
  static String rxBuffer = "";
  static unsigned long lastByteTime = 0;

  static uint8_t escBuf[7];
  static int escLen = 0;
  const uint8_t RESET_SEQ[] = {0x1B, '[', '2', 'J', 0x1B, '[', 'H'};
  const int RESET_SEQ_LEN = 7;


  if (rxBuffer.length() > 0 && (millis() - lastByteTime) > 150) {
    String e = rxBuffer;
    // Escape JSON: backslash poi virgolette
    e.replace("\\", "\\\\");
    e.replace("\"", "\\\"");
    ws.textAll("{\"type\":\"cursor\",\"msg\":\"" + e + "\"}"); 
    rxBuffer = "";
  }

  while (Serial2.available()) {
    uint8_t b = Serial2.read();

    // ── RESET ANSI (ESC[2JESC[H) ──────────────
    if (b == 0x1B || escLen > 0) {
      escBuf[escLen++] = b;
    
      bool match = true;
      for (int i = 0; i < escLen; i++) {
        if (escBuf[i] != RESET_SEQ[i]) { match = false; break; }
      }
      if (!match) {
 
        escLen = 0;
        continue;
      }
      if (escLen == RESET_SEQ_LEN) {

        ws.textAll("{\"type\":\"reset\"}");
        Serial.println("[RESET_TERM]");
        escLen = 0;
        rxBuffer = "";
      }

      continue;
    }


    if (b == 0x0D) { /* CR: niente, aspettiamo LF */ }
    else if (b == 0x0A) Serial.print("\n");
    else Serial.write(b);

    if (b == 0x0D) {

    }
    else if (b == 0x0A) {

      if (rxBuffer.length() > 0) {
        String e = rxBuffer;
        e.replace("\\", "\\\\");   
        e.replace("\"", "\\\"");    
        ws.textAll("{\"type\":\"rx\",\"msg\":\"" + e + "\"}");
        rxBuffer = "";
      } else {
        ws.textAll("{\"type\":\"rx\",\"msg\":\"\"}");
      }
    }
    else if (b == '\\' && rxBuffer.length() == 0) {
      
      ws.textAll("{\"type\":\"rx\",\"msg\":\"\\\\\"}");
      rxBuffer = "";
    }
    else if (b >= 32 && b <= 126) {
      
      rxBuffer += (char)b;
      lastByteTime = millis();
    }
  }
  ws.cleanupClients();
  delay(1);
}
