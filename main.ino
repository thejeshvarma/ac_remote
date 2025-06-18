#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <lwip/napt.h>
#include <lwip/dns.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>

#define STASSID "Airtel_Coral"
#define STAPSK  "Coral123"
#define IR_LED_PIN 13  // D7 = GPIO13
#define NAPT 1000
#define NAPT_PORT 10

// MQTT Configuration
const char* mqtt_server = "35.154.62.193";
const int mqtt_port = 1883;
const char* mqtt_topic = "ac/control";

String baseSSID = "CEO_Room_AC_";
String currentSSID;
ESP8266WebServer server(80);
WiFiClient espClient;
PubSubClient mqttClient(espClient);

unsigned long lastOffSendTime = 0;
unsigned long offInterval = 900000; // 10min

IRsend irsend(IR_LED_PIN);
bool naptInitialized = false;
bool clientConnected = false;
bool offSentAfterDisconnect = false;

uint16_t rawOn[] =  {3514, 1682,  482, 1266,  482, 402,  478, 402,  476, 408,  476, 1270,  478, 402,  476, 404,  476, 408,  476, 402,  478, 1270,  476, 408,  472, 1270,  480, 1270,  478, 402,  476, 1270,  478, 1270,  476, 1270,  478, 1270,  476, 1270,  478, 402,  478, 406,  478, 1270,  476, 402,  478, 408,  472, 406,  476, 404,  476, 408,  476, 402,  478, 1270,  476, 1270,  478, 1270,  476, 1270,  478, 402,  478, 402,  476, 408,  476, 404,  476, 404,  476, 406,  478, 404,  476, 404,  476, 408,  472, 406,  478, 402,  476, 404,  476, 406,  478, 402,  478, 402,  476, 408,  480, 400,  478, 402,  476, 408,  476, 404,  476, 402,  478, 406,  478, 402,  476, 404,  476, 408,  476, 1270,  476, 404,  478, 402,  476, 408,  472, 408,  476, 402,  476, 408,  474, 28246,  3510, 1688,  478, 1266,  482, 402,  478, 402,  476, 408,  476, 1266,  482, 402,  476, 402,  478, 408,  476, 402,  478, 1270,  476, 402,  478, 1270,  476, 1270,  478, 406,  474, 1270,  476, 1270,  478, 1270,  476, 1270,  478, 1270,  476, 408,  472, 408,  476, 1270,  478, 402,  476, 408,  472, 408,  476, 402,  478, 402,  478, 406,  478, 404,  476, 402,  478, 406,  476, 404,  476, 404,  476, 408,  476, 404,  476, 402,  478, 408,  476, 402,  476, 404,  476, 408,  476, 1266,  482, 404,  476, 404,  476, 408,  472, 1270,  452, 1296,  476, 408,  472, 408,  476, 402,  478, 1270,  476, 404,  476, 408,  476, 1270,  478, 1266,  480, 402,  478, 402,  476, 408,  472, 408,  476, 404,  476, 402,  478, 406,  478, 402,  478, 402,  476, 408,  476, 404,  476, 404,  476, 408,  472, 406,  478, 1272,  476, 1272,  476, 402,  478, 1270,  478, 402,  476, 402,  478, 408,  478, 402,  478, 402,  478, 408,  472, 408,  476, 404,  476, 408,  472, 408,  476, 404,  476, 402,  478, 406,  472, 408,  478, 404,  480, 404,  472, 408,  472, 408,  476, 404,  478, 406,  472, 408,  476, 402,  478, 406,  474, 406,  476, 404,  476, 408,  472, 406,  478, 402,  478, 406,  472, 408,  472, 408,  476, 408,  472, 408,  472, 408,  476, 406,  472, 408,  472, 408,  476, 408,  472, 408,  472, 408,  476, 408,  472, 406,  472, 408,  472, 408,  476, 408,  472, 408,  472, 406,  478, 408,  472, 406,  472, 1276,  476, 404,  476, 402,  478, 406,  478, 402,  476, 1272,  476, 1270,  478, 402,  476, 408,  478, 402,  476, 402,  478, 408,  476, 402,  478, 402,  476, 404,  476, 408,  476, 402,  478, 402,  478, 406,  478, 402,  476, 404,  476, 408,  476, 402,  478, 1270,  476, 1272,  476, 1270,  478, 402,  478, 404,  476, 1270,  476, 1272,  476, 1272,  476};
uint16_t rawOff[]=  {3514, 1684,  478, 1270,  476, 408,  474, 408,  476, 404,  476, 1272,  476, 402,  478, 408,  476, 402,  478, 402,  478, 1270,  478, 402,  476, 1272,  476, 1272,  476, 402,  478, 1272,  476, 1270,  476, 1272,  476, 1272,  476, 1270,  478, 402,  476, 404,  476, 1272,  476, 408,  476, 402,  478, 404,  476, 402,  478, 406,  478, 402,  478, 1270,  476, 1272,  476, 1270,  478, 1270,  478, 402,  476, 402,  478, 406,  478, 402,  478, 402,  478, 406,  478, 402,  476, 404,  476, 408,  476, 404,  476, 404,  476, 408,  472, 406,  478, 402,  478, 402,  476, 408,  478, 402,  476, 404,  476, 408,  476, 404,  476, 404,  476, 408,  472, 406,  478, 402,  478, 406,  474, 1270,  476, 408,  478, 402,  476, 404,  476, 408,  472, 408,  476, 402,  478, 28244,  3510, 1688,  476, 1272,  476, 402,  478, 406,  472, 408,  476, 1270,  478, 404,  476, 404,  476, 408,  476, 404,  476, 1270,  478, 402,  478, 1272,  476, 1272,  476, 402,  478, 1272,  476, 1270,  478, 1270,  476, 1272,  476, 1266,  476, 408,  476, 404,  476, 1270,  478, 402,  476, 408,  474, 406,  476, 402,  478, 408,  472, 408,  476, 402,  478, 404,  476, 408,  476, 404,  476, 404,  476, 408,  472, 406,  478, 402,  478, 406,  472, 408,  478, 402,  476, 404,  476, 408,  472, 408,  476, 402,  478, 408,  472, 1270,  478, 1270,  476, 408,  472, 408,  476, 402,  478, 1270,  478, 402,  478, 406,  478, 1266,  476, 1270,  476, 408,  476, 404,  476, 404,  476, 408,  476, 404,  476, 402,  476, 408,  472, 408,  476, 402,  478, 406,  474, 406,  478, 402,  478, 402,  476, 408,  472, 1270,  478, 1270,  476, 408,  472, 1270,  476, 408,  478, 402,  476, 402,  478, 406,  474, 406,  478, 402,  476, 408,  472, 408,  476, 404,  478, 402,  478, 408,  472, 406,  478, 402,  476, 408,  472, 408,  476, 404,  476, 408,  472, 408,  476, 402,  476, 408,  472, 408,  476, 402,  478, 402,  478, 406,  472, 408,  476, 404,  476, 408,  472, 408,  476, 402,  478, 406,  474, 406,  472, 408,  476, 408,  474, 406,  472, 406,  478, 402,  476, 408,  472, 408,  476, 402,  478, 406,  474, 408,  476, 404,  476, 408,  472, 406,  472, 408,  476, 408,  474, 406,  472, 406,  478, 408,  472, 1270,  476, 406,  478, 402,  478, 402,  476, 408,  478, 1266,  452, 1296,  476, 408,  476, 404,  476, 402,  478, 406,  448, 432,  476, 404,  476, 408,  446, 432,  478, 402,  478, 402,  476, 408,  478, 402,  476, 408,  472, 408,  476, 402,  476, 404,  476, 408,  476, 1266,  456, 1292,  478, 406,  478, 402,  476, 1270,  478, 1270,  476, 1272,  476};
uint16_t raw24[] =  {3560, 1634,  502, 1244,  532, 352,  502, 376,  502, 382,  528, 1218,  502, 376,  506, 378,  502, 376,  502, 376,  482, 1270,  502, 378,  502, 1244,  502, 1244,  508, 376,  502, 1244,  528, 1218,  528, 1218,  502, 1246,  476, 1270,  504, 380,  502, 378,  502, 1244,  532, 350,  528, 352,  502, 376,  506, 378,  528, 352,  506, 376,  502, 1244,  502, 1244,  502, 1246,  500, 1244,  508, 376,  502, 376,  502, 382,  502, 376,  502, 378,  506, 376,  502, 378,  502, 380,  528, 352,  502, 380,  502, 376,  504, 380,  502, 376,  502, 378,  506, 376,  502, 378,  502, 382,  502, 378,  502, 382,  528, 352,  502, 376,  532, 352,  502, 376,  508, 376,  502, 378,  502, 1244,  504, 380,  502, 376,  504, 376,  506, 376,  502, 378,  502, 380,  502, 28226,  3534, 1662,  502, 1244,  534, 350,  502, 378,  502, 382,  528, 1218,  528, 350,  502, 382,  504, 376,  504, 380,  502, 1244,  502, 376,  502, 1244,  506, 1244,  502, 376,  504, 1244,  502, 1244,  506, 1240,  506, 1240,  506, 1244,  528, 350,  502, 382,  502, 1246,  502, 378,  502, 376,  506, 378,  502, 376,  508, 376,  502, 378,  502, 380,  528, 352,  502, 378,  502, 382,  502, 376,  502, 382,  502, 378,  502, 382,  502, 376,  502, 378,  506, 378,  528, 352,  502, 1244,  506, 378,  502, 376,  504, 380,  528, 1220,  502, 1244,  502, 378,  506, 376,  502, 378,  506, 376,  502, 378,  502, 380,  502, 1244,  502, 1246,  502, 382,  476, 404,  502, 376,  506, 376,  504, 376,  502, 382,  502, 376,  502, 380,  502, 378,  502, 382,  502, 378,  502, 382,  502, 378,  528, 352,  506, 1244,  502, 376,  502, 1244,  502, 382,  502, 376,  502, 382,  502, 378,  528, 356,  502, 378,  502, 376,  502, 382,  502, 376,  502, 382,  502, 376,  502, 376,  508, 376,  528, 350,  502, 382,  502, 376,  502, 382,  528, 352,  502, 382,  502, 376,  502, 376,  508, 376,  502, 378,  502, 382,  528, 352,  502, 380,  502, 376,  502, 382,  502, 376,  504, 376,  502, 382,  502, 376,  502, 382,  502, 378,  502, 380,  502, 378,  502, 382,  502, 378,  502, 380,  498, 382,  528, 356,  498, 382,  502, 382,  498, 382,  502, 378,  502, 382,  502, 376,  502, 382,  476, 402,  502, 382,  498, 1248,  504, 376,  502, 382,  498, 382,  502, 376,  502, 1250,  502, 1244,  502, 376,  502, 382,  498, 380,  502, 378,  502, 382,  502, 378,  502, 382,  502, 378,  502, 380,  498, 382,  502, 376,  502, 382,  498, 1248,  502, 376,  502, 382,  502, 376,  502, 1246,  502, 382,  504, 1244,  502, 376,  502, 1248,  502, 378,  502, 376,  502, 1248,  502};
uint16_t raw27[]=   {3514, 1682,  480, 1270,  476, 404,  480, 404,  476, 402,  476, 1270,  476, 404,  482, 402,  478, 402,  482, 402,  478, 1270,  476, 402,  480, 1266,  480, 1266,  482, 402,  476, 1270,  476, 1270,  482, 1266,  482, 1266,  482, 1266,  482, 402,  478, 402,  480, 1266,  482, 402,  476, 402,  482, 402,  478, 402,  482, 402,  476, 402,  476, 1270,  478, 1270,  478, 1270,  482, 1266,  482, 402,  476, 404,  480, 402,  476, 404,  476, 402,  482, 402,  476, 402,  482, 402,  476, 404,  476, 404,  480, 402,  476, 402,  482, 402,  476, 402,  482, 402,  476, 404,  476, 402,  482, 402,  476, 404,  480, 402,  478, 402,  476, 402,  482, 402,  478, 402,  482, 402,  476, 1270,  478, 402,  480, 402,  478, 402,  476, 404,  480, 404,  476, 402,  482, 28250,  3514, 1682,  482, 1270,  476, 402,  482, 402,  476, 402,  482, 1266,  482, 402,  476, 404,  480, 402,  476, 402,  480, 1270,  476, 404,  480, 1266,  482, 1266,  482, 402,  476, 1270,  480, 1266,  480, 1266,  482, 1270,  478, 1270,  478, 402,  480, 404,  476, 1270,  476, 402,  482, 402,  476, 402,  482, 402,  476, 404,  476, 402,  482, 402,  476, 402,  482, 402,  476, 404,  480, 402,  478, 402,  482, 402,  478, 402,  482, 402,  476, 404,  476, 402,  482, 402,  476, 1272,  480, 404,  476, 402,  476, 404,  480, 1266,  480, 1272,  476, 402,  482, 402,  476, 404,  480, 1266,  480, 1270,  476, 402,  482, 1266,  482, 1266,  480, 404,  476, 402,  480, 404,  476, 402,  482, 402,  476, 402,  482, 402,  476, 404,  480, 402,  478, 402,  480, 402,  478, 402,  480, 402,  478, 402,  476, 1270,  480, 404,  478, 1270,  476, 404,  480, 402,  476, 404,  480, 402,  476, 402,  476, 408,  476, 402,  478, 402,  480, 402,  478, 402,  480, 402,  478, 402,  482, 402,  476, 402,  476, 408,  476, 402,  478, 402,  480, 402,  478, 402,  480, 404,  476, 402,  476, 408,  476, 402,  476, 402,  482, 402,  476, 404,  480, 402,  476, 402,  478, 406,  476, 404,  476, 402,  482, 402,  476, 402,  478, 406,  476, 404,  476, 404,  480, 404,  476, 402,  482, 402,  476, 402,  476, 408,  476, 402,  476, 408,  476, 402,  476, 404,  480, 402,  476, 404,  480, 402,  476, 402,  478, 406,  476, 1270,  476, 402,  476, 408,  476, 402,  476, 408,  478, 1270,  476, 1270,  478, 402,  482, 402,  476, 402,  482, 402,  476, 402,  478, 406,  476, 402,  476, 408,  476, 404,  476, 408,  476, 402,  476, 402,  482, 1266,  482, 402,  476, 402,  482, 402,  476, 1270,  478, 1270,  476, 402,  482, 1266,  480, 1266,  482, 402,  478, 402,  480, 1266,  482};
uint16_t raw29[]=   {3514, 1688,  476, 1270,  478, 402,  478, 406,  478, 402,  478, 1270,  478, 406,  478, 402,  476, 408,  472, 408,  476, 1270,  476, 402,  478, 1270,  478, 1270,  478, 406,  478, 1270,  476, 1270,  476, 1270,  478, 1270,  476, 1270,  476, 408,  476, 408,  472, 1274,  476, 402,  478, 406,  476, 404,  476, 406,  472, 408,  476, 402,  478, 1270,  476, 1274,  476, 1270,  478, 1270,  478, 406,  472, 408,  478, 402,  478, 406,  476, 404,  476, 402,  478, 406,  476, 404,  476, 408,  472, 406,  478, 402,  476, 408,  476, 402,  478, 406,  472, 408,  476, 402,  478, 406,  472, 408,  476, 402,  478, 406,  478, 402,  476, 408,  472, 406,  478, 404,  476, 408,  476, 1270,  478, 402,  476, 408,  472, 406,  478, 406,  472, 408,  476, 402,  478, 28252,  3514, 1686,  478, 1270,  476, 402,  476, 408,  472, 408,  476, 1270,  476, 408,  472, 406,  478, 402,  476, 408,  476, 1270,  476, 402,  478, 1270,  476, 1270,  476, 408,  476, 1270,  476, 1270,  476, 1270,  476, 1270,  476, 1270,  478, 406,  478, 402,  476, 1270,  478, 406,  476, 402,  478, 408,  476, 402,  476, 408,  472, 406,  478, 402,  476, 406,  478, 402,  476, 408,  478, 402,  476, 406,  472, 408,  476, 404,  476, 406,  478, 402,  476, 408,  476, 404,  476, 1272,  476, 408,  472, 408,  476, 402,  478, 1270,  476, 1270,  478, 406,  476, 404,  476, 408,  476, 1270,  476, 404,  478, 1270,  476, 1270,  478, 1270,  478, 406,  476, 402,  478, 406,  472, 408,  476, 404,  476, 408,  476, 402,  476, 408,  476, 402,  478, 402,  476, 408,  476, 402,  478, 406,  478, 402,  478, 1270,  478, 406,  472, 1270,  478, 406,  478, 402,  476, 408,  476, 404,  478, 402,  478, 406,  478, 402,  476, 408,  472, 406,  478, 402,  476, 408,  476, 404,  476, 408,  472, 406,  478, 402,  476, 408,  476, 404,  476, 406,  474, 406,  476, 404,  476, 408,  476, 402,  478, 406,  472, 408,  476, 408,  472, 408,  476, 402,  478, 406,  472, 408,  476, 408,  472, 406,  478, 402,  476, 408,  476, 402,  478, 406,  472, 408,  476, 402,  478, 406,  478, 402,  476, 406,  472, 408,  476, 406,  472, 408,  476, 408,  472, 408,  476, 402,  478, 406,  476, 402,  476, 408,  472, 408,  476, 1270,  476, 408,  476, 402,  478, 406,  476, 404,  476, 1270,  476, 1270,  478, 406,  478, 402,  478, 402,  476, 406,  478, 402,  476, 408,  476, 404,  476, 408,  476, 402,  478, 402,  476, 408,  476, 402,  478, 406,  478, 402,  476, 404,  476, 408,  476, 1270,  476, 1270,  478, 1270,  476, 1270,  476, 408,  476, 402,  478, 406,  478, 1270,  476};
uint16_t rawOnLen = sizeof(rawOn)/sizeof(rawOn[0]);
uint16_t rawOffLen = sizeof(rawOff)/sizeof(rawOff[0]);
uint16_t raw24Len = sizeof(raw24)/sizeof(raw24[0]);
uint16_t raw27Len = sizeof(raw27)/sizeof(raw27[0]);
uint16_t raw29Len = sizeof(raw29)/sizeof(raw29[0]);

void sendIR(uint16_t *data, uint16_t length, String label) {
  Serial.println("Sending IR: " + label);
  irsend.sendRaw(data, length, 38); // 38 kHz
}

void connectToWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(STASSID, STAPSK);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nConnected to WiFi!");
  Serial.print("STA IP address: ");
  Serial.println(WiFi.localIP());
}

void setupNAPT() {
  auto& dhcp = WiFi.softAPDhcpServer();
  dhcp.setDns(WiFi.dnsIP(0));

  if (!naptInitialized) {
    if (ip_napt_init(NAPT, NAPT_PORT) == ERR_OK) {
      naptInitialized = true;
      Serial.println("NAPT initialized.");
    } else {
      Serial.println("NAPT init failed!");
    }
  }

  if (ip_napt_enable_no(SOFTAP_IF, 1) == ERR_OK) {
    Serial.println("NAPT enabled, extender is ready!");
  } else {
    Serial.println("NAPT enable failed!");
  }
}

void generateNewSSID() {
  int randNum = random(0, 99999);
  currentSSID = baseSSID + String(randNum);
  Serial.println("Generated SSID: " + currentSSID);
}

void startAccessPoint() {
  WiFi.softAPdisconnect(true);
  generateNewSSID();
  IPAddress apIP(192, 168, 4, 1);
  IPAddress netMsk(255, 255, 255, 0);
  WiFi.softAPConfig(apIP, apIP, netMsk);
  WiFi.softAP(currentSSID.c_str(), NULL, 1, false, 1); // open AP
  Serial.println("AP Started with SSID: " + currentSSID);
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
  setupNAPT();
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String cmd = "";
  for (unsigned int i = 0; i < length; i++) {
    cmd += (char)payload[i];
  }
  cmd.trim();
  Serial.println("MQTT received: " + cmd);

  if (cmd.equalsIgnoreCase("ON")) {
    sendIR(rawOn, rawOnLen, "ON");
  }
  else if (cmd.equalsIgnoreCase("OFF")) {
    sendIR(rawOff, rawOffLen, "OFF");
  }
  else if (cmd.startsWith("TEMP:")) {
    int temp = cmd.substring(5).toInt();
    switch (temp) {
      case 24: sendIR(raw24, raw24Len, "24°C"); break;
      case 25: sendIR(rawOn, rawOnLen, "25°C"); break;
      case 27: sendIR(raw27, raw27Len, "27°C"); break;
      case 29: sendIR(raw29, raw29Len, "29°C"); break;
      default: Serial.println("Unknown temperature command");
    }
  }
}

void reconnectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    
    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("connected");
      mqttClient.subscribe(mqtt_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" retrying in 5 seconds");
      delay(5000);
    }
  }
}

void setupWebServer() {
  // Captive portal detection routes for various OS
  server.on("/generate_204", []() { server.send(204, "text/html", ""); });               // Android
  server.on("/hotspot-detect.html", []() { server.send(200, "text/html", "Success"); }); // iOS/macOS
  server.on("/ncsi.txt", []() { server.send(200, "text/plain", "Microsoft NCSI"); });    // Windows

  // Main UI route
server.on("/", []() {
  server.send(200, "text/html", R"rawliteral(
    <!DOCTYPE html>
    <html lang="en">
    <head>
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <title>AC Remote Control</title>
      <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.5.0/css/all.min.css">
      <style>
        body {
          margin: 0;
          font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
          background-color: #e0e0e0;
          display: flex;
          justify-content: center;
          align-items: center;
          height: 100vh;
        }

        .remote {
          background: #fff;
          border-radius: 20px;
          box-shadow: 0 8px 20px rgba(0,0,0,0.2);
          padding: 25px 20px;
          width: 280px;
          display: flex;
          flex-direction: column;
          align-items: center;
        }

        .remote img {
          width: 60px;
          margin-bottom: 8px;
        }

        .remote h1 {
          font-size: 20px;
          color: #444;
          margin-bottom: 15px;
        }

        .display {
          background-color: #1c1c1c;
          color: #0f0;
          font-family: monospace;
          font-size: 16px;
          padding: 10px 15px;
          border-radius: 10px;
          width: 100%;
          text-align: center;
          margin-bottom: 20px;
          box-shadow: inset 0 0 5px rgba(0, 255, 0, 0.4);
        }

        .button {
          background-color: #ff6f00;
          color: white;
          border: none;
          border-radius: 50%;
          width: 60px;
          height: 60px;
          font-size: 18px;
          margin: 10px;
          cursor: pointer;
          transition: 0.3s;
          display: flex;
          align-items: center;
          justify-content: center;
        }

        .button:hover {
          background-color: #e65c00;
        }

        .power {
          background-color: #d32f2f;
          width: 70px;
          height: 70px;
          font-size: 22px;
          border-radius: 50%;
          margin: 10px 0;
        }

        .temp-buttons {
          display: flex;
          flex-wrap: wrap;
          justify-content: center;
        }

        .label {
          margin-top: 10px;
          font-weight: bold;
          color: #555;
        }
      </style>
    </head>
    <body>
      <div class="remote">
        <img src="https://i.ibb.co/tPHMr0Tn/hq720-removebg-preview.png" alt="Logo">
        <h1>AC Remote</h1>

        <div class="display" id="statusDisplay">
          Ready
        </div>

        <button class="power" onclick="sendCommand('/on', 'Power ON')">
          <i class="fas fa-power-off"></i>
        </button>
        <button class="power" onclick="sendCommand('/off', 'Power OFF')">
          <i class="fas fa-power-off"></i>
        </button>

        <div class="label">Set Temperature</div>
        <div class="temp-buttons">
          <button class="button" onclick="sendCommand('/24', 'Temperature set to 24°C')">
            <i class="fas fa-temperature-low"></i><span style="margin-left:6px;">24</span>
          </button>
          <button class="button" onclick="sendCommand('/25', 'Temperature set to 25°C')">
            <i class="fas fa-temperature-low"></i><span style="margin-left:6px;">25</span>
          </button>
          <button class="button" onclick="sendCommand('/27', 'Temperature set to 27°C')">
            <i class="fas fa-temperature-high"></i><span style="margin-left:6px;">27</span>
          </button>
          <button class="button" onclick="sendCommand('/29', 'Temperature set to 29°C')">
            <i class="fas fa-temperature-high"></i><span style="margin-left:6px;">29</span>
          </button>
        </div>
      </div>

      <script>
        function sendCommand(endpoint, statusText) {
          fetch(endpoint)
            .then(() => {
              document.getElementById('statusDisplay').textContent = statusText;
            })
            .catch(() => {
              document.getElementById('statusDisplay').textContent = 'Error sending command';
            });
        }
      </script>
    </body>
    </html>
  )rawliteral");
});

server.on("/on", []() {
  sendIR(rawOn, rawOnLen, "ON");
  server.send(200, "text/plain", "Sent ON");
});

server.on("/off", []() {
  sendIR(rawOff, rawOffLen, "OFF");
  server.send(200, "text/plain", "Sent OFF");
});

server.on("/24", []() {
  sendIR(raw24, raw24Len, "24°C");
  server.send(200, "text/plain", "Sent 24°C");
});
server.on("/25", []() {
  sendIR(rawOn, rawOnLen, "25°C");
  server.send(200, "text/plain", "Sent 25°C");
});
server.on("/27", []() {
  sendIR(raw27, raw27Len, "27°C");
  server.send(200, "text/plain", "Sent 27°C");
});
server.on("/29", []() {
  sendIR(raw29, raw29Len, "29°C");
  server.send(200, "text/plain", "Sent 29°C");
});
server.begin();
Serial.println("Web server started");
}

void setupOTA() {
  ArduinoOTA.setHostname("AC_Remote_Ceo");
  ArduinoOTA.onStart([]() {
    Serial.println("Start updating...");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nUpdate Complete");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress * 100) / total);
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("OTA Ready");
}

void setup() {
  Serial.begin(115200);
  delay(100);
  randomSeed(micros());

  irsend.begin();
  WiFi.setOutputPower(1); // 0 dBm to reduce interference
  connectToWiFi();
  setupOTA();
  startAccessPoint();
  setupWebServer();

  // Setup MQTT
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCallback(mqttCallback);

  WiFi.onSoftAPModeStationConnected([](const WiFiEventSoftAPModeStationConnected& evt) {
    Serial.println("Client connected event.");
    clientConnected = true;  // Flag for loop logic
  });
}

void loop() {
  server.handleClient();
  ArduinoOTA.handle();

  // MQTT connection check and reconnect if needed
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  mqttClient.loop();

  int clientCount = WiFi.softAPgetStationNum();
  unsigned long now = millis();

  if (clientCount > 0) {
    if (!clientConnected) {
      Serial.println("Client connected!");
      clientConnected = true;
      offSentAfterDisconnect = false;

      // Boost signal strength when client connects
      WiFi.setOutputPower(20.5); // Max power

      delay(3000); // Optional delay before sending ON
      sendIR(rawOn, rawOnLen, "ON (client connect)");
    }
  } else {
    if (clientConnected && !offSentAfterDisconnect) {
      Serial.println("Client disconnected!");

      // Reduce signal strength to minimum
      WiFi.setOutputPower(0.0); // Min power
      
      sendIR(rawOff, rawOffLen, "OFF (disconnect)");
      clientConnected = false;
      offSentAfterDisconnect = true;
      startAccessPoint();  // Optional: restart AP
    }

    // Continue sending OFF every 5min if no client is connected
    if (now - lastOffSendTime >= offInterval) {
      lastOffSendTime = now;
      Serial.println("No client — sending OFF again");
      sendIR(rawOff, rawOffLen, "OFF (no client)");
    }
  }
}
