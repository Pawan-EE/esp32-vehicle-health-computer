/* ESP32-S3 Vehicle Health Computer. See docs/firmware.md before live use.
 * Arduino-ESP32 3.3.12; bundled legacy driver/twai.h API, no MCP2515.
 * Default demo does not install TWAI. Live reads require a deliberate rebuild.
 */
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <driver/twai.h>
#include <esp_system.h>
#include "config.h"
#include "obd.h"
#include "dashboard.h"

static_assert(CAN_KBIT==250 || CAN_KBIT==500, "Choose 250 or 500 kbit/s");
static_assert(ECU_RESPONSE_ID>=0x7E8 && ECU_RESPONSE_ID<=0x7EF, "11-bit OBD ECU response only");
static_assert(POLL_MS>=300, "Keep diagnostic request rate conservative");
struct Metric { uint8_t pid; const char *name,*unit; float value; uint32_t updated; bool seen,supported; };
Metric metrics[] = {
  {0x0C,"Engine speed","rpm"}, {0x0D,"Vehicle speed","km/h"},
  {0x05,"Coolant","C"}, {0x04,"Calculated load","%"},
  {0x11,"Throttle","%"}, {0x0F,"Intake air","C"},
  {0x10,"Mass air flow","g/s"}, {0x2F,"Fuel level","%"},
  {0x42,"ECU voltage","V"}
};
WebServer server(80); DNSServer dns; obd::IsoTp transport;
bool installed=false, halted=false, pending=false, dtcQueued=false;
bool monitorSeen=false,mil=false,monitorSupported=false;
uint8_t dtcCount=0, pendingService=0,pendingPid=0, discovery=0,metricIndex=0;
uint32_t sentAt=0,lastRequest=0,monitorAt=0,lastMonitorRequest=0,startedAt=0;
uint32_t txCount=0,rxCount=0,timeouts=0,consecutiveTimeouts=0;
uint8_t discoveryTries=0;
String state="Starting",dtcStatus="Not scanned this session",codes="",token;

bool liveAllowed() { return !DEMO_MODE && ENABLE_OBD_REQUESTS && installed && !halted; }
void halt(const char *reason) {
  halted=true; pending=false; dtcQueued=false; transport.reset(); state=reason;
  if(installed) twai_stop(); // No automatic restart: inspect wiring and reboot deliberately.
  if(dtcStatus=="Reading stored DTCs") dtcStatus="Scan aborted; reboot after inspection";
}
// The only general CAN transmit primitive is private to this sketch. No raw web endpoint.
bool sendFrame(const uint8_t *bytes) {
  if(!liveAllowed()) return false;
  twai_message_t m={}; m.identifier=ECU_RESPONSE_ID-8; m.data_length_code=8;
  m.ss=1; // Single shot: no indefinite automatic retransmission on wrong/missing bus.
  memcpy(m.data,bytes,8);
  if(twai_transmit(&m,0)!=ESP_OK) { halt("Transmit queue failure; inspect and reboot"); return false; }
  txCount++; return true;
}
bool allowedPid(uint8_t pid) {
  if(pid==0 || pid==0x20 || pid==0x40 || pid==1) return true;
  for(auto &m:metrics) if(m.pid==pid) return true;
  return false;
}
void request(uint8_t service,uint8_t pid=0) {
  if(pending || !(service==3 || (service==1 && allowedPid(pid)))) return;
  uint8_t b[8]={uint8_t(service==3?1:2),service,pid,0,0,0,0,0};
  if(sendFrame(b)) {
    pending=true; pendingService=service; pendingPid=pid;
    sentAt=lastRequest=millis(); transport.reset();
  }
}
void completePayload() {
  const auto *p=transport.data; size_t n=transport.size;
  if(n>=3 && p[0]==0x7F && p[1]==pendingService) {
    pending=false; transport.reset();
    if(pendingService==3) dtcStatus="ECU rejected or deferred scan; retry later";
    return; // No repeated requests or unbounded response-pending extension.
  }
  if(n<1 || p[0]!=(pendingService+0x40)) return;
  if(pendingService==1) {
    if(n<2 || p[1]!=pendingPid) return;
    uint8_t pid=p[1];
    if(pid==0 || pid==0x20 || pid==0x40) {
      if(n<6) return;
      uint32_t bits=(uint32_t(p[2])<<24)|(uint32_t(p[3])<<16)|(uint32_t(p[4])<<8)|p[5];
      for(auto &m:metrics) if(m.pid>pid && m.pid<=pid+32) m.supported=obd::supported(bits,pid,m.pid);
      if(pid==0) monitorSupported=obd::supported(bits,0,1);
      discovery=(pid<0x40 && (bits&1)) ? uint8_t(discovery+1) : 3;
      discoveryTries=0;
    } else if(pid==1) {
      if(n<6) return;
      mil=p[2]&0x80; dtcCount=p[2]&0x7F; monitorSeen=true; monitorAt=millis();
    } else {
      for(auto &m:metrics) if(m.pid==pid) {
        float value; if(!obd::decode(pid,p+2,n-2,value)) return;
        m.value=value; m.seen=true; m.updated=millis();
      }
    }
  } else if(pendingService==3) {
    // ISO 15765 OBD Mode 03: service, number of DTCs, then two-byte codes.
    if(n<2 || n<size_t(2+2*p[1])) { dtcStatus="Malformed or truncated DTC response"; pending=false; return; }
    codes="";
    for(size_t i=0;i<p[1];i++) {
      if(p[2+i*2]==0 && p[3+i*2]==0) continue;
      char code[6]; obd::dtc(p[2+i*2],p[3+i*2],code);
      if(codes.length()) codes+=",";
      codes+=String('"')+code+'"';
    }
    dtcStatus="Stored DTC scan complete (selected ECU only)";
  }
  pending=false; consecutiveTimeouts=0; state="Receiving selected ECU";
}
void receiveCan() {
  uint32_t alerts=0; twai_read_alerts(&alerts,0);
  if(alerts&(TWAI_ALERT_BUS_OFF|TWAI_ALERT_ERR_PASS|TWAI_ALERT_TX_FAILED|TWAI_ALERT_RX_QUEUE_FULL)) {
    halt("CAN fault; transmission stopped. Inspect bitrate/wiring and reboot"); return;
  }
  twai_message_t m;
  for(int budget=0;budget<32 && twai_receive(&m,0)==ESP_OK;budget++) {
    rxCount++;
    if(!pending || m.extd || m.rtr || m.identifier!=ECU_RESPONSE_ID || m.data_length_code!=8) continue;
    // Reject unrelated first/single-frame services before allocating a transaction / sending FC.
    uint8_t kind=m.data[0]>>4;
    if(kind<=1) {
      uint8_t offset=kind==1?2:1;
      if(m.data[offset]!=pendingService+0x40 && m.data[offset]!=0x7F) continue;
      if(m.data[offset]==0x7F && m.data[offset+1]!=pendingService) continue;
      if(pendingService==1 && m.data[offset]==0x41 && m.data[offset+1]!=pendingPid) continue;
    }
    int r=transport.feed(m.data,m.data_length_code);
    if(r<0) { pending=false; transport.reset(); if(pendingService==3) dtcStatus="Invalid ISO-TP response"; }
    if(r==2) { uint8_t fc[8]={0x30,0,10,0,0,0,0,0}; sendFrame(fc); }
    if(r==1) completePayload();
  }
}
void scheduler() {
  uint32_t now=millis();
  if(now-startedAt>=SESSION_MS) { halt("Parked test session expired; disconnect or reboot"); return; }
  if(pending && now-sentAt>1000) { // Bounded whole ISO-TP transaction
    pending=false; transport.reset(); timeouts++; consecutiveTimeouts++;
    if(pendingService==3) dtcStatus="DTC scan timed out; result unavailable";
  } else if(pending && !transport.active && now-sentAt>RESPONSE_MS) {
    pending=false; timeouts++; consecutiveTimeouts++;
    if(pendingService==3) dtcStatus="DTC scan timed out; result unavailable";
  }
  if(consecutiveTimeouts>=10) { halt("No valid replies after 10 requests; inspect and reboot"); return; }
  if(pending || now-lastRequest<POLL_MS) return;
  if(discovery<3) {
    if(discoveryTries>=3) { halt("PID discovery failed; check selected ECU and bitrate"); return; }
    discoveryTries++; request(1,discovery*0x20); return;
  }
  if(dtcQueued) { dtcQueued=false; dtcStatus="Reading stored DTCs"; codes=""; request(3); return; }
  if(monitorSupported && (!monitorSeen || now-lastMonitorRequest>=5000)) {
    lastMonitorRequest=now; request(1,1); return;
  }
  for(size_t i=0;i<sizeof(metrics)/sizeof(metrics[0]);i++) {
    auto &m=metrics[metricIndex++ % (sizeof(metrics)/sizeof(metrics[0]))];
    if(m.supported) { request(1,m.pid); return; }
  }
}
void demo() {
  static uint32_t last=0; uint32_t now=millis(); if(now-last<500)return; last=now;
  float wave=sinf(now/6000.0f);
  float values[]={850+100*wave,0,88+2*wave,22+3*wave,14,26,4.5f+wave,65,14.1f};
  for(size_t i=0;i<sizeof(metrics)/sizeof(metrics[0]);i++) {
    metrics[i].value=values[i]; metrics[i].seen=metrics[i].supported=true; metrics[i].updated=now;
  }
  monitorSeen=true; monitorAt=now; mil=codes.length()>0; dtcCount=mil?1:0;
}
void jsonStatus() {
  uint32_t now=millis(); String out; out.reserve(2300);
  out="{\"mode\":\""+String(DEMO_MODE?"DEMO — SIMULATED DATA":ENABLE_OBD_REQUESTS?"LIVE DIAGNOSTIC READS":"PASSIVE LISTEN ONLY")+"\",\"status\":\""+state+"\",\"token\":\""+token+"\",\"ecu\":\"0x"+String(ECU_RESPONSE_ID,HEX)+"\",\"tx\":"+txCount+",\"rx\":"+rxCount+",\"timeouts\":"+timeouts;
  out+=",\"monitorValid\":"+String(monitorSeen && now-monitorAt<STALE_MS && !halted?"true":"false")+",\"mil\":"+(mil?"true":"false")+",\"dtcCount\":"+dtcCount;
  out+=",\"dtcStatus\":\""+dtcStatus+"\",\"dtcs\":["+codes+"],\"canScan\":"+String(DEMO_MODE || (liveAllowed() && discovery==3 && !pending && !dtcQueued)?"true":"false")+",\"metrics\":[";
  for(size_t i=0;i<sizeof(metrics)/sizeof(metrics[0]);i++) {
    auto &m=metrics[i]; if(i) out+=',';
    out+="{\"name\":\""+String(m.name)+"\",\"unit\":\""+m.unit+"\",\"supported\":"+(m.supported?"true":"false")+",\"valid\":"+(m.seen && now-m.updated<STALE_MS && !halted?"true":"false")+",\"value\":"+String(m.value,2)+",\"age\":"+String(now-m.updated)+"}";
  }
  out+="]}"; server.sendHeader("Cache-Control","no-store"); server.send(200,"application/json",out);
}
void setup() {
  Serial.begin(115200); startedAt=millis();
  token=String(esp_random(),HEX)+String(esp_random(),HEX);
  WiFi.mode(WIFI_AP); WiFi.softAPConfig(IPAddress(192,168,4,1),IPAddress(192,168,4,1),IPAddress(255,255,255,0));
  if(!WiFi.softAP(AP_SSID,AP_PASSWORD,1,false,2)) { state="Wi-Fi startup failed"; halted=true; return; }
  dns.start(53,"*",WiFi.softAPIP());
  const char *headers[]={"X-Session-Token"}; server.collectHeaders(headers,1);
  server.on("/",HTTP_GET,[]{ server.send_P(200,"text/html",DASHBOARD); });
  server.on("/api/status",HTTP_GET,jsonStatus);
  server.on("/api/dtc",HTTP_POST,[]{
    if(server.header("X-Session-Token")!=token) { server.send(403,"text/plain","Invalid session token"); return; }
    if(DEMO_MODE) { codes="\"P0300\""; dtcStatus="SIMULATED example only: P0300"; server.send(200,"text/plain",dtcStatus); return; }
    if(!liveAllowed() || discovery!=3 || pending || dtcQueued) { server.send(409,"text/plain","Unavailable or busy"); return; }
    dtcQueued=true; server.send(202,"text/plain","Scan queued");
  });
  server.onNotFound([]{server.sendHeader("Location","http://192.168.4.1/",true);server.send(302,"text/plain","");});
  server.begin();
  if(DEMO_MODE) {state="Demo: CAN driver disabled";return;}
  twai_general_config_t g=TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)CAN_TX_PIN,(gpio_num_t)CAN_RX_PIN,ENABLE_OBD_REQUESTS?TWAI_MODE_NORMAL:TWAI_MODE_LISTEN_ONLY);
  g.tx_queue_len=2; g.rx_queue_len=32;
  g.alerts_enabled=TWAI_ALERT_BUS_OFF|TWAI_ALERT_ERR_PASS|TWAI_ALERT_TX_FAILED|TWAI_ALERT_RX_QUEUE_FULL;
  twai_timing_config_t t=TWAI_TIMING_CONFIG_500KBITS(); if(CAN_KBIT==250) t=TWAI_TIMING_CONFIG_250KBITS();
  twai_filter_config_t f=TWAI_FILTER_CONFIG_ACCEPT_ALL();
  // Single standard ID filter (ID bits 31:21); flags/DLC still checked in software.
  f.acceptance_code=ECU_RESPONSE_ID<<21; f.acceptance_mask=~(0x7FFUL<<21);
  if(twai_driver_install(&g,&t,&f)!=ESP_OK) {halt("TWAI installation failed");return;}
  installed=true;
  if(twai_start()!=ESP_OK) {halt("TWAI start failed");return;}
  state=ENABLE_OBD_REQUESTS?"Discovering supported PIDs":"Passive mode; no OBD requests or decoding";
}
void loop() {
  dns.processNextRequest(); server.handleClient();
  if(DEMO_MODE) demo();
  else if(installed && !halted) {receiveCan(); if(liveAllowed())scheduler();}
  delay(1); // Yield to Wi-Fi/RTOS; no blocking CAN receive loops.
}
