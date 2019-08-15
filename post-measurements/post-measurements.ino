#include <EtherCard.h>

static byte mymac[] = { 0x1A,0x2B,0x3C,0x4D,0x5E,0x6F };
byte Ethernet::buffer[700];
static uint16_t hisport = 4242;

static uint32_t timer;

const char website[] PROGMEM = "192.168.0.3";
const char apiPath[] PROGMEM = "/api/endpoints/10/data/";
const char websiteFull[] PROGMEM = "192.168.0.1:8000";
const int dstPort PROGMEM = 8000;
const char token[] PROGMEM = "some-token";

static byte session;
Stash stash;

void setup () {
  Serial.begin(9600);

  // Change 'SS' to your Slave Select pin, if you arn't using the default pin
  if (ether.begin(sizeof Ethernet::buffer, mymac, 8) == 0)
    Serial.println( "Failed to access Ethernet controller");
  if (!ether.dhcpSetup())
    Serial.println("DHCP failed");

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

  sendUpdate();
}

void loop () {
//  if (millis() > timer) {    
//    timer = millis() + 5000;

    ether.packetLoop(ether.packetReceive());

    const char* reply = ether.tcpReply(session);
    if (reply != 0) {
      Serial.println("Got a response!");
      Serial.println(reply);
    }
}

static void sendUpdate () {
  Serial.println("Sending message...");
  byte sd = stash.create();

  const char message[] = "{\"data\": \"cookies\", \"label\": \"arduino\", \"timestamp\": \"2019-10-20T03:14\"}";
//  stash.print("token=");
//  stash.print(TOKEN);
//  stash.print("&status=");
  stash.println(message);
  stash.save();
  int stash_size = stash.size();

  // Compose the http POST request, taking the headers below and appending
  // previously created stash in the sd holder.
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
