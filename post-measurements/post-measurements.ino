#include <EtherCard.h>

static byte mymac[] = { 0x1A,0x2B,0x3C,0x4D,0x5E,0x6F };
byte Ethernet::buffer[700];

const char website[] PROGMEM = "192.168.0.2";
const char apiPath[] PROGMEM = "/api/endpoints/10/data/";
const char websiteFull[] PROGMEM = "192.168.0.2:8000";
const int dstPort PROGMEM = 8000;
const char token[] PROGMEM = "some-token";

// timer vars
static uint32_t timer;
const int timeInterval = 10000;

static byte session;
Stash stash;

void setup () {
  Serial.begin(9600);

  // Change 'SS' to your Slave Select pin, here 8 is selected
  if (ether.begin(sizeof Ethernet::buffer, mymac, 8) == 0) {
    Serial.println( "Failed to access Ethernet controller");
  }
  if (!ether.dhcpSetup()) {
    Serial.println("DHCP failed");
  }

  ether.printIp("IP:  ", ether.myip);
  ether.printIp("GW:  ", ether.gwip);
  ether.printIp("DNS: ", ether.dnsip);

  if (!ether.dnsLookup(website))
    Serial.println("DNS failed");

  ether.hisport = dstPort;
  ether.printIp("SRV: ", ether.hisip);
  Serial.print("SRV-port:");
  Serial.print(ether.hisport);
  Serial.println();
}

const char* reply;

void loop () {
  if (millis() > timer) {
    // send updates every timeInterval miliseconds
    timer = millis() + timeInterval;
    sendUpdate();
  }

  ether.packetLoop(ether.packetReceive());
  reply = ether.tcpReply(session);
  if (reply != 0) {
    Serial.println("Got a response!");
    Serial.println(reply);
  }
}

static void sendUpdate () {
  Serial.println("Sending message...");
  byte sd = stash.create();

  const char message[] = "{\"data\": \"cookies\", \"label\": \"arduino\", \"timestamp\": \"2019-10-20T03:14\"}";
  stash.println(message);
  stash.save();
  int stash_size = stash.size();

  // Compose the http POST request
  Stash::prepare(PSTR("POST $F HTTP/1.0" "\r\n"
    "Host: $F" "\r\n"
    "Content-Length: $D" "\r\n"
    "Content-Type: application/json" "\r\n"
    "Token: $F" "\r\n"
    "\r\n"
    "$H"),
  apiPath, websiteFull, stash_size, token, sd);

  // send the packet - this also releases all stash buffers once done
  // Save the session ID so we can watch for it in the main loop.
  session = ether.tcpSend();
}
