#include <Arduino.h>
#include <EtherCard.h>

#define PINRED 2
#define PINGREEN 3
#define PINBLUE 4

static byte mymac[] = {0x84,0x18,0x26,0x7c,0x2d,0x05 };
byte Ethernet::buffer[500];
BufferFiller bfill;
char state[5][7] = {"Off","Red","Green","Blue","All On" };
int lampstate = 0;

const char HOSTNAME[] PROGMEM = "SI-Z069N";

const char http_OK[] PROGMEM =
"HTTP/1.0 200 OK\r\n"
"Content-Type: text/html\r\n"
"Pragma: no-cache\r\n\r\n";

const char http_Found[] PROGMEM =
"HTTP/1.0 302 Found\r\n"
"Location: /\r\n\r\n";

const char http_Unauthorized[] PROGMEM =
"HTTP/1.0 401 Unauthorized\r\n"
"Content-Type: text/html\r\n\r\n"
"<h1>401 Unauthorized</h1>";

void setup(){
  pinMode(PINRED,OUTPUT); //pins are outputs
  digitalWrite(PINRED,LOW); //Off to begin with
  pinMode(PINGREEN,OUTPUT); //pins are outputs
  digitalWrite(PINGREEN,LOW); //Off to begin with
  pinMode(PINBLUE,OUTPUT); //pins are outputs
  digitalWrite(PINBLUE,LOW); //Off to begin with

  Serial.begin(115200);
  Serial.println("\n[start]");

  if (ether.begin(sizeof Ethernet::buffer, mymac) == 0) 
    Serial.println( "Failed to access Ethernet controller");
  if (!ether.dhcpSetup(HOSTNAME))
    Serial.println("DHCP failed");
  ether.printIp("IP:  ", ether.myip);
  ether.printIp("GW:  ", ether.gwip);  
  ether.printIp("DNS: ", ether.dnsip);  
}

void homePage(){
  //$F represents p string, $D represents word or byte, $S represents string. 
  bfill.emit_p(PSTR("$F"
    "<meta http-equiv='refresh' content='30'/>"
    "<title>VRTE Build Lamp</title>" 
    "Turn Lamp <a href=\"?lamp=A\">All On</a><br>" 
    "Turn Lamp <a href=\"?lamp=R\">RED</a><br>" 
    "Turn Lamp <a href=\"?lamp=G\">GREEN</a><br>" 
    "Turn Lamp <a href=\"?lamp=B\">BLUE</a><br>" 
    "Turn Lamp <a href=\"?lamp=O\">OFF</a><br>"
    "Lamp is $S<br>"),
  http_OK,
  state[lampstate]); 
}

void loop(){
  // wait for an incoming TCP packet, but ignore its contents
  word len = ether.packetReceive();
  word pos = ether.packetLoop(len); 
  if (pos) {
    delay(1); 
    bfill = ether.tcpOffset();
    char *data = (char *) Ethernet::buffer + pos;
    if (strncmp("GET /", data, 5) != 0) { 
      // Unsupported HTTP request
      bfill.emit_p(http_Unauthorized);
    } else {
        data += 5;
        if (data[0] == ' ') { 
          homePage();
        } else if (!strncmp("?lamp=",data,6)){ 
          data += 6;
          if (data[0] == 'R') { 
            lampstate = 1;
            digitalWrite(PINRED,1);
            digitalWrite(PINGREEN,0);
            digitalWrite(PINBLUE,0);
          } else if (data[0] == 'G') { 
            lampstate = 2;
            digitalWrite(PINRED,0);
            digitalWrite(PINGREEN,1);
            digitalWrite(PINBLUE,0);
          } else if (data[0] == 'B') { 
            lampstate = 3;
            digitalWrite(PINRED,0);
            digitalWrite(PINGREEN,0);
            digitalWrite(PINBLUE,1);
          } else if (data[0] == 'A') { 
            lampstate = 4;
            digitalWrite(PINRED,1);
            digitalWrite(PINGREEN,1);
            digitalWrite(PINBLUE,1);
          } else if (data[0] == 'O') { 
            lampstate = 0;
            digitalWrite(PINRED,0);
            digitalWrite(PINGREEN,0);
            digitalWrite(PINBLUE,0);
          }
          Serial.print("state = ");
          Serial.println(lampstate);
          bfill.emit_p(http_Found);
        } else { 
          //Otherwise, page isn't found
          // Page not found
          bfill.emit_p(http_Unauthorized);
        }
      }

      ether.httpServerReply(bfill.position());    // send http response
    
  }
}