#include <EtherCard.h>
// ethernet interface mac address, must be unique on the LAN
byte cdp_mac[] = {
  0x01, 0x00, 0x0c, 0xcc, 0xcc, 0xcc
};
byte mymac[] = {
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05
};
//-------------------NEW------------
byte lldp_mac[] = {0x01, 0x80, 0xc2, 0x00, 0x00, 0x0e};
int rbuflen;
char TLVTYPE[3];
char TLVLENGTH[3];
//----------------------------------
#define MAC_LENGTH 6
byte Ethernet::buffer[501];

int Device = 0;
int Port = 0;
int Model = 0;
String Protocal;
String LCD_data[8];
String Names[8] = {
  "Name:", "MAC:" , "Port:", "Model:", "VLAN:", "IP:", "Voice VLAN:", "Cap:"
};

#include <Adafruit_GFX.h>    // Core graphics librar
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <SPI.h>

//Use these pins for the shield!
#define sclk 13
#define mosi 11
#define cs   4
#define dc   8
#define rst  9  // you can also connect this to the Arduino reset

// Option 1: use any pins but a little slower
Adafruit_ST7735 tft = Adafruit_ST7735(cs, dc, rst);

//Print &out = tft;

void setup()
{
  //  tft.fillScreen(ST7735_BLACK);
  pinMode(13, OUTPUT);
  Serial.begin(19200);
  Serial.println("Serial Initialised\nScreen Initialising");
  tft.initR(INITR_BLACKTAB);   // initialize a ST7735S chip, black tab
  Serial.println("Screen Done");

  tft.fillScreen(ST7735_BLACK);

  tft.setTextWrap(false);
  tft.setRotation(1);
  tft.setCursor(0, 0);
  drawscreen();
  // testdrawtext("Default Interval is 60secs!\n", ST7735_RED);
  // testdrawtext("DHCP:", ST7735_BLUE);

  ether.begin(sizeof Ethernet::buffer, mymac, 10);
  Serial.println("Ethernet Done");
  if (!ether.dhcpSetup())
  {
    // testdrawtext("DHCP failed.\n", ST7735_RED);
    Serial.println("DHCP failed.");
  }
  else
  {
    drawscreen();
    // testdrawtext("DHCP IP:", ST7735_GREEN);
    Serial.print("DHCP IP:");
    for (unsigned int j = 0; j < 4; ++j) {
      Serial.print(String(ether.myip[j]));
      // testdrawtext(String(ether.myip[j]), ST7735_WHITE);
      if (j < 3) {
        Serial.print(".");
        //  testdrawtext(".", ST7735_WHITE);
      }
    }
    //testdrawtext("\n", ST7735_WHITE);
    Serial.print("\n");
  }
  delay(200);
  ENC28J60::enablePromiscuous();
  Serial.println("Ethernet Promiscuous");
  // ether.printIp("GW: ", ether.gwip);

}
void loop()
{
  int plen = ether.packetReceive();
  if ( plen > 0 ) {

    if (byte_array_contains(Ethernet::buffer, 0, cdp_mac, sizeof(cdp_mac))) {

      //CDP Packet found and is now getting processed

      Protocal = "CDP";
      Serial.println("CDP Packet Received");

      //Get source MAC Address
      byte* macFrom = Ethernet::buffer + sizeof(cdp_mac);
      print_mac(macFrom, 0, 6);




      //Set start hex address
      int DataIndex = 26;

      //Cycle through the frame reading the TLV fields
      while (DataIndex < plen) {
        unsigned int cdpFieldType = (Ethernet::buffer[DataIndex] << 8) | Ethernet::buffer[DataIndex + 1];
        DataIndex += 2;
        unsigned int TLVFieldLength = (Ethernet::buffer[DataIndex] << 8) | Ethernet::buffer[DataIndex + 1];
        DataIndex += 2;
        TLVFieldLength -= 4;

        switch (cdpFieldType) {

          case 0x0001: //CDP Device name
            Device = 1;
            handleCdpAsciiField(Ethernet::buffer, DataIndex, TLVFieldLength);
            Device = 0;
            break;

          case 0x0002: //CDP IP address
            handleCdpAddresses(Ethernet::buffer, DataIndex, TLVFieldLength);
            break;

          case 0x0003: //CDP Port Name
            Port = 1;
            handleCdpAsciiField( Ethernet::buffer, DataIndex, TLVFieldLength);
            Port = 0;
            break;

          /* case 0x0004: //Capabilities
            handleCdpCapabilities(Ethernet::buffer, DataIndex + 2, TLVFieldLength - 2);
            break; */


          case 0x0006: //CDP Model Name
            Model = 1;
            handleCdpAsciiField( Ethernet::buffer, DataIndex, TLVFieldLength);
            Model = 0;
            break;

          case 0x000a: //CDP VLAN #
            handleCdpNumField( Ethernet::buffer, DataIndex, TLVFieldLength);
            break;

          /* case 0x000b:
            SWver = 1;
            if (TLVFieldLength > 50) {    TLVFieldLength = 50;}
            handleCdpAsciiField( Ethernet::buffer, DataIndex, TLVFieldLength);
            SWver = 0;
            break; */


          case 0x000e: //CDP VLAN voice#
            handleCdpVoiceVLAN( Ethernet::buffer, DataIndex + 2, TLVFieldLength - 2);
            break;
        }
        DataIndex += TLVFieldLength;
      }
      drawscreen();
    }

    if (byte_array_contains(Ethernet::buffer, 0, lldp_mac, sizeof(lldp_mac))) {

      //CDP Packet found and is now getting processed
      Protocal = "LLDP";
      Serial.println("LLPD Packet Received");

      byte* macFrom1 = Ethernet::buffer + sizeof(lldp_mac);
      print_mac(macFrom1, 0, 6);

      int DataIndex = 14;

      while (DataIndex < plen) { // read all remaining TLV fields
        unsigned int cdpFieldType = (Ethernet::buffer[DataIndex]);
        DataIndex += 1;
        unsigned int TLVFieldLength = (Ethernet::buffer[DataIndex]);
        /*Serial.print(" type:");
        Serial.print(cdpFieldType, HEX);
        Serial.print(" Length:");
        Serial.print(TLVFieldLength);*/
        DataIndex += 1;

        switch (cdpFieldType) {


          case 0x0004: //Port Name
            Port = 1;
            handleCdpAsciiField( Ethernet::buffer, DataIndex + 1, TLVFieldLength - 1);
            Port = 0;
            break;

          case 0x0006: //TTL

            break;

          case 0x0008: //Port Description

            break;

          case 0x000a: //Device Name
            Device = 1;
            handleCdpAsciiField( Ethernet::buffer, DataIndex , (TLVFieldLength));
            Device = 0;
            break;

          case 0x000e://Capabilities
            // handleCdpCapabilities( Ethernet::buffer, DataIndex + 2, TLVFieldLength - 2);
            break;

          case 0x0010: //Management IP Address
            handleLLDPIPField(Ethernet::buffer, DataIndex + 2, 4);
            break;

          case 0x00fe: //LLDP Organisational TLV #
            handleLLDPOrgTLV(Ethernet::buffer, DataIndex, TLVFieldLength);
            break;
        }
        DataIndex += TLVFieldLength;
      }
      drawscreen();
    }
  }
}

void handleLLDPOrgTLV( const byte a[], unsigned int offset, unsigned int lengtha) {
  unsigned int Orgtemp;

  // for (unsigned int i = offset; i < ( offset + 3 ); ++i ) {
  //   Orgtemp += a[i];
  // }
  Orgtemp = (a[offset] << 16) | (a[offset + 1] << 8) | a[offset + 2];
  //  Serial.println( Orgtemp, HEX);
  /*Serial.print("\n Org Type:");
  Serial.print(Orgtemp, HEX);
  Serial.print(" Length:");
  Serial.print(lengtha);*/
  switch (Orgtemp) {

    case 0x0012BB:
      //TIA TR-41
    /*  Serial.print("\n Subtype:");
      Serial.print(a[offset + 3], HEX);
      Serial.print(" Length:");
      Serial.print(lengtha);*/
      switch (a[offset + 3]) {

        /*case 0x0001:
          //TIA TR-41 - Media Capabilities - returned binary for LLDP-MED TLV's enabled - Ignored for now
          break; */

        case 0x0002:
          //TIA TR-41 -network policy - deals with Application types and additional settings - good for voice vlan, etc..
/*          Serial.print("\n Subtype:");
          Serial.print(a[offset + 3], HEX);
          Serial.print(" Length:");
          Serial.print(lengtha); */
          switch (a[offset + 4]) {
            case 0x0001: //TIA TR-41 - network policy - Voice
              unsigned int VoiceVlanID;
              VoiceVlanID = (a[offset+ 5] << 16) | (a[offset + 6] << 8) | a[offset + 7];
              VoiceVlanID = VoiceVlanID >>9; //shift the bits to the left to remove the first bits
              LCD_data[6] = String(VoiceVlanID, DEC);
              break;

            case 0x0002:  //TIA TR-41 - Network policy - Voice signaling

              break;
          }

          break;

        /*case 0x0003:
          //TIA TR-41 - Location Indentification
          break; */

        /*case 0x0004:
          //TIA TR-41 - Extended power-via-MDI - POE information - returns a byte array
          break; */

        /*case 0x0005:
          //TIA TR-41 - Inventory - hardware revision
          break; */

        /*case 0x0006:
          //TIA TR-41 - Inventory - firmware revision
          break; */


        /*case 0x0007:
          //TIA TR-41 - Inventory - software revision
          break; */

        /*case 0x0009:
          //TIA TR-41 - Inventory - Manufacturer
          break;*/

        case 0x000a:
          //TIA TR-41 - Inventory - Model Name
          Model = 1;
          handleCdpAsciiField( a, offset + 4, lengtha - 4);
          Model = 0;
          break;

          /* case 0x000b:
            //TIA TR-41 - Inventory - Asset ID
            Model = 1;
            handleCdpAsciiField( a, offset + 4, lengtha - 4);
            Model = 0;
            break; */

      }
    case 0x00120F:
      //IEEE 802.3
     /* Serial.print("\n Subtype:");
      Serial.print(a[offset + 3], HEX);
      Serial.print(" Length:");
      Serial.print(lengtha);*/
      switch (a[offset + 3]) {
          //case 0x0001:
          //IEEE 802.3 - IEEE MAC/PHY Configuration/Status\n");
          //returns HEX array for different port settings like autonegotiate and speed.
          //break;
      }
      break;
    case 0x0080C2:
      //IEEE

      switch (a[offset + 3]) {
        case 0x0001:
          //IEEE - Port VLAN ID
          //the next octets are the vlan #
          handleCdpNumField( a, offset + 4, 2);
          break;
      }
      break;
  }

}
/*
  String CdpCapabilities(String temp) {

  String output;
  //Serial.print(temp.substring(26,27));
  if (temp.substring(15, 16) == "1") {
    output = output + "Router ";
  }
  if (temp.substring(14, 15) == "1") {
    output = output + "Trans_Bridge ";
  }
  if (temp.substring(13, 14) == "1") {
    output = output + "Route_Bridge,";
  }
  if (temp.substring(12, 13) == "1") {
    output = output + "Switch ";
  }
  if (temp.substring(11, 12) == "1") {
    output = output + "Host ";
  }
  if (temp.substring(10, 11) == "1") {
    output = output + "IGMP ";
  }
  if (temp.substring(9, 10) == "1") {
    output = output + "Repeater ";
  }

  return output;
  }


  String LldpCapabilities(String temp) {
  //Serial.print (temp);
  String output;
  //Serial.print(temp.substring(26,27));
  if (temp.substring(15, 16) == "1") {
    output = output + "Other ";
  }
  if (temp.substring(14, 15) == "1") {
    output = output + "Repeater ";
  }
  if (temp.substring(13, 14) == "1") {
    output = output + "Bridge ";
  }
  if (temp.substring(12, 13) == "1") {
    output = output + "WLAN ";
  }
  if (temp.substring(11, 12) == "1") {
    output = output + "Router ";
  }
  if (temp.substring(10, 11) == "1") {
    output = output + "Telephone ";
  }
  if (temp.substring(9, 10) == "1") {
    output = output + "DOCSIS ";
  }

  if (temp.substring(8, 9) == "1") {
    output = output + "Station ";
  }
  return output;
  }

  void handleCdpCapabilities( const byte a[], unsigned int offset, unsigned int lengtha) {
  int j = 0;
  String temp;
  if (Protocal == "CDP") {
    for (unsigned int i = offset; i < ( offset + lengtha ); ++i , ++j) {
      temp  =  temp + print_binary(a[i], 8);
    }
    LCD_data[7] = (CdpCapabilities(temp));
  }

  if (Protocal == "LLDP") {
    for (unsigned int i = offset; i < ( offset + lengtha ); ++i , ++j) {
      temp  =  temp + print_binary(a[i], 8)  ;
    }
    LCD_data[7] =  (LldpCapabilities(temp));
  }
  //  Serial.print(temp);
  }
*/
String print_binary(int v, int num_places)
{
  String output;
  int mask = 0, n;
  for (n = 1; n <= num_places; n++)
  {
    mask = (mask << 1) | 0x0001;
  }
  v = v & mask;  // truncate v to specified number of places

  while (num_places)
  {
    if (v & (0x0001 << num_places - 1))
    {
      output = output + "1" ;
    }
    else
    {
      output = output + "0" ;
    }
    --num_places;
  }
  return output;
}
void handleCdpNumField( const byte a[], unsigned int offset, unsigned int length) {
  unsigned long num = 0;
  for (unsigned int i = 0; i < length; ++i) {
    num <<= 8;
    num += a[offset + i];
  }
  LCD_data[4] = "" + String(num, DEC);
}

void handleCdpVoiceVLAN( const byte a[], unsigned int offset, unsigned int length) {
  unsigned long num = 0;
  for (unsigned int i = offset; i < ( offset + length ); ++i) {
    num <<= 8;
    // Serial.print(a[i]);
    num += a[i];
  }
  LCD_data[6] = String(num, DEC);
}

void handleLLDPIPField(const byte a[], unsigned int offset, unsigned int lengtha) {
  int j = 0;
  for (unsigned int i = offset; i < ( offset + lengtha ); ++i , ++j) {
    //LCD_data[5] = "";
    LCD_data[5] += a[i], DEC;
    if (j < 3) {
      LCD_data[5] += ".";
    }
  }

  // int lengthostring = sizeof(LCD_data[5]);
  // LCD_data[5][lengtha] = '\0';
  // LCD_data[5] += '\0';
}

void handleCdpAddresses(const byte a[], unsigned int offset, unsigned int length) {
  unsigned long numOfAddrs = (a[offset] << 24) | (a[offset + 1] << 16) | (a[offset + 2] << 8) | a[offset + 3];
  offset += 4;

  for (unsigned long i = 0; i < numOfAddrs; ++i) {
    unsigned int protoType = a[offset++];
    unsigned int protoLength = a[offset++];
    byte proto[8];
    for (unsigned int j = 0; j < protoLength; ++j) {
      proto[j] = a[offset++];
    }
    unsigned int addressLength = (a[offset] << 8) | a[offset + 1];

    offset += 2;
    byte address[4];
    if (addressLength != 4) Serial.println(F("Expecting address length: 4"));

    LCD_data[5] = "";
    for (unsigned int j = 0; j < addressLength; ++j) {
      address[j] = a[offset++];

      LCD_data[5] = LCD_data[5] + address[j] ;
      if (j < 3) {
        LCD_data[5] = LCD_data[5] + ".";
      }

    }
  }

}

void handleCdpAsciiField(byte a[], unsigned int offset, unsigned int lengtha) {
  int j = 0;

  char temp [lengtha + 1] ;
  for (unsigned int i = offset; i < ( offset + lengtha ); ++i , ++j) {
    temp[j] = a[i];
  }
  temp[lengtha  ] = '\0';

  if (Device != 0) {
    LCD_data[0] = temp;
  }

  if (Port != 0) {
    LCD_data[2] = temp;
  }

  if (Model != 0) {
    LCD_data[3] = temp;
  }

}

String print_ip(const byte a[], unsigned int offset, unsigned int length) {
  String ip;
  for (unsigned int i = offset; i < offset + length; ++i) {
    //    if(i>offset) Serial.print('.');
    //   Serial.print(a[i], DEC);
    if (i > offset) ip = ip + '.';
    ip = ip + String (a[i]);
  }
  int iplentgh;
  return ip;
}

String print_mac(const byte a[], unsigned int offset, unsigned int length) {
  String Mac;
  char temp [40];
  LCD_data[1] = "";
  for (unsigned int i = offset; i < offset + length; ++i) {

    if (i > offset) {
      //  LCD_data[1] = LCD_data[1] + Mac + ':';
      Mac = Mac + ':';
    }
    if (a[i] < 0x10) {
      Mac = Mac + '0';
      //    LCD_data[1] = LCD_data[1] + Mac + '0';
    }
    Mac = Mac + String (a[i], HEX);
  }
  LCD_data[1] = LCD_data[1]  + Mac;
  return Mac;
}

bool byte_array_contains(const byte a[], unsigned int offset, const byte b[], unsigned int length) {
  for (unsigned int i = offset, j = 0; j < length; ++i, ++j) {
    if (a[i] != b[j]) {
      return false;
    }
  }
  return true;
}


void testdrawtext(String text, uint16_t color) {
  tft.setTextColor(color);
  tft.print(text);
}

void drawscreen () {
  tft.fillScreen(ST7735_BLACK);
  tft.setTextWrap(false);
  tft.setRotation(1);
  tft.setCursor(0, 0);

  testdrawtext("MODLOG CDP/LLDP", ST7735_RED);
  printVolts();
  tft.setTextColor(ST7735_RED, ST7735_BLUE);
  if (String(ether.myip[1]) == "") {
    testdrawtext("DHCP: None Assigned.\n", ST7735_RED);
    Serial.println("DHCP failed.");
  }
  else
  {
    String DHCPtxt = "";
    testdrawtext("DHCP IP:", ST7735_GREEN);
    for (unsigned int j = 0; j < 4; ++j) {
      DHCPtxt = DHCPtxt + (String(ether.myip[j]));
      testdrawtext(String(ether.myip[j]), ST7735_WHITE);
      if (j < 3) {
        DHCPtxt = DHCPtxt + ".";
        testdrawtext(".", ST7735_WHITE);
      }
    }
    testdrawtext("\n", ST7735_WHITE);

    Serial.println("Device IP:" + DHCPtxt);
    tft.setCursor(0, 20);
    testdrawtext("Protocal:", ST7735_GREEN);
    testdrawtext(Protocal + "\n", ST7735_WHITE);
    testdrawtext("\n", ST7735_WHITE);
  }

  for (unsigned int i = 0; i < 8; ++i) {
    tft.setCursor(0, i * 10 + 30);
    testdrawtext(Names[i], ST7735_GREEN);
    testdrawtext(LCD_data[i] + "\n", ST7735_WHITE);
    Serial.println("" + Names[i] + "" + LCD_data[i]);
    LCD_data[i] = "";
  }

  Serial.println("--------END-------");
  delay(500);
}

void printVolts(){
  float sensorValue = analogRead(A3); //read the A0 pin value
  float voltage = sensorValue / 1024 * 5.0;
  //convert the value to a true voltage.
  int percent = 100 - ((5 - voltage ) / 2.4 * 100);
  tft.setCursor(110, 0);
  //Serial.println(voltage );
  //Serial.println(percent);
  testdrawtext("BATT:" + String(percent) + "%\n", ST7735_BLUE);
  //Serial.print(voltage); //print the voltage to LCD
  //Serial.print(" V");
  if (voltage < 6.50) //set the voltage considered low battery here
  {
    //Serial.print("LOW");
  }
}
