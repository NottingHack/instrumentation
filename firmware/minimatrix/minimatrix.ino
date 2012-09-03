// screen is 38x2 characters, or 192*16 pixels


#include "SystemFont5x8.h"
#include "TimerOne.h"
#include <EtherShield.h>
#include <NanodeUNIO.h>
#include <OneWire.h>
#include <avr/wdt.h>
 
#define ONEWIRE_SEARCH 0

#define OE      A0
#define STROBE  A1
#define CLK     A2
#define S_DATA  A3

#define AD0 3
#define AD1 4
#define AD2 5
#define ED2 6

#define REFRESH_INTERVAL 20000 // screen refresh in microseconds (20ms)

#define NET_BUF_SIZE (42 + 145) // 42 byte ethernet/udp header + 145 payload
#define ARP_RETRY 10000 // 10s

#define TEMP_READ_FREQ 20000         // Milliseconds
//#define TEMP_READ_FREQ 1500         // Milliseconds

// Port  49168
#define PORT_L  0x10
#define PORT_H  0xC0


#define MSG_SET_BUFFER 'B'
#define MSG_SHOW 'S'

#define OPTION_SCROLLIN 1
#define OPTION_SCROLL 2
#define OPTION_INSTANT_DISPLAY 4
#define OPTION_FLASH 8
#define OPTION_INVERT 16

EtherShield es=EtherShield();
static uint8_t mymac[6]; // Read from chip on nanode.
static uint8_t myip[4] = {192,168,0,13};
static uint8_t dstip[4] = {192,168,0,1};
static uint8_t dstmac[6]; 
 
const char iphdr[] PROGMEM ={0x45,0,0,0x82,0,0,0x40,0,0x20}; // 0x82 is the total
static uint8_t netbuf [NET_BUF_SIZE]; 
  
enum {LINE_UPPER, LINE_LOWER};

char msg_nhbanner[] PROGMEM = {"Nottinghack mini-matrix 0.01"}; 
char msg_initnet[] PROGMEM = {"Init net..."}; 
char msg_initonewire[] PROGMEM = {"Init one wire..."}; 
char msg_boot[] PROGMEM = {"Mini-matrix boot!"};

//byte sensor_addr[] PROGMEM = {0x28, 0xDA, 0xB8, 0xC2, 0x03, 0x00, 0x00, 0xBE};
byte sensor_addr[8];

//char msg_initnet[] PROGMEM = {91, 126, 127, 128,93, 0}; 
//char msg_nhbanner[] PROGMEM = {91, 126, 127, 128,93, 0}; 

// Frame buffers for top and bottom line of display (2 buffers per line - active and back buffer)
struct disp
{
  char fbuf[2][192];
  byte cur_buf;
  byte msg_details;
  byte nxtmsg;
} line[2]; // ~772


// Buffers to hold IRC/twitter/email messages/names
struct msg
{
  uint8_t msg[141]; //141
  int pos; // 0 to msg_len * 5
  int msg_len;
  byte options;
} msg_detail[4];  // ~ 882
 
  
unsigned long lastmove; 
unsigned long flash_timer;
unsigned long last_temp_read;
byte temp_read_in_progress;
OneWire  ds(A4);  // on pin 10



void setup () 
{
  wdt_disable();
  int count;
  
  digitalWrite(OE, LOW);
  Serial.begin(9600);
  Serial.println F("Setup() start");
  pinMode(OE, OUTPUT);
  pinMode(STROBE, OUTPUT);
  pinMode(CLK, OUTPUT);
  pinMode(S_DATA, OUTPUT);
  pinMode(AD0, OUTPUT);
  pinMode(AD1, OUTPUT);
  pinMode(AD2, OUTPUT);
  pinMode(ED2, OUTPUT);


  digitalWrite(AD0, LOW);
  digitalWrite(AD1, LOW);
  digitalWrite(AD2, LOW);
  digitalWrite(ED2, LOW);

  PORTB =0;
  PORTD &= B11000011;
  delay(100);
  

  // prep buffer
  memset(msg_detail, 0x00, sizeof(msg_detail)); 
  memset(line, 0x00, sizeof(line));   
  
  line[LINE_UPPER].cur_buf = 0;
  line[LINE_UPPER].msg_details = 0;
  line[LINE_UPPER].nxtmsg = 0xFF;
  
  line[LINE_LOWER].cur_buf = 0;
  line[LINE_LOWER].msg_details = 1;
  line[LINE_LOWER].nxtmsg = 0xFF;  
 
  disp_progmem_msg(msg_nhbanner, LINE_UPPER);
  disp_progmem_msg(msg_initnet, LINE_LOWER);

  digitalWrite(OE, HIGH);
 
  Timer1.initialize(); // Timer interupt  
  Timer1.attachInterrupt(int_refresh_screen, REFRESH_INTERVAL); 
  // Get MAC address
  NanodeUNIO unio(NANODE_MAC_DEVICE);
  unio.read(mymac,NANODE_MAC_ADDRESS,6);

  // Init network
  memset (netbuf, 0, sizeof(netbuf));
 
  // Initialise SPI interface & ENC28J60
  es.ES_enc28j60SpiInit();
  es.ES_enc28j60Init(mymac, 8);

  //init the ethernet/ip layer:
  es.ES_init_ip_arp_udp_tcp(mymac,myip, 80);  
  
  //Get MAC address of server
  Serial.println F("Init net complete");  
  get_mac();  
  Serial.println F("Got mac");  
  lastmove = millis();
  flash_timer = millis();
  last_temp_read = millis();
  
  disp_progmem_msg(msg_initonewire, LINE_LOWER);
  init_one_wire();  
 
  memset (msg_detail[1].msg, ' ', sizeof(msg_detail[1].msg)); 
  
  udp_send_prgmem_msg(msg_boot);
  wdt_enable(WDTO_8S);
  Serial.println F("Setup() complete");
}

void init_one_wire()
{
  temp_read_in_progress = false;
  ds.search(sensor_addr);
  ds.reset_search();
  delay(250);
}

void one_wire_loop()
{
  byte i;
  byte present = 0;
  byte type_s;
  byte *data;
  byte *addr;
  char *net_data;
    
  if (temp_read_in_progress)
  {
    
    if ((millis() - last_temp_read) > 1000)
    {
      netbuf[42] = 'T'; // 0-41 is the Ethernet/UDP header
      addr = netbuf + 43; 
      data = netbuf + 52;      
      
      for (i=0; i < 8; i++)
        //addr[i] = pgm_read_byte_near(sensor_addr + i);
        addr[i] = (byte)sensor_addr[i];      
      
      present = ds.reset();
      ds.select(sensor_addr);    
      ds.write(0xBE);         // Read Scratchpad
    
      for ( i = 0; i < 9; i++) 
        data[i] = ds.read();
    /* Now do this server side 
      // convert the data to actual temperature
      unsigned int raw = (data[1] << 8) | data[0];
      byte cfg = (data[4] & 0x60);
      if (cfg == 0x00) raw = raw << 3;  // 9 bit resolution, 93.75 ms
      else if (cfg == 0x20) raw = raw << 2; // 10 bit res, 187.5 ms
      else if (cfg == 0x40) raw = raw << 1; // 11 bit res, 375 ms
      // default is 12 bit resolution, 750 ms conversion time
    
      celsius = (float)raw / 16.0;
      Serial.print F("  Temperature = ");
      Serial.println(celsius);
      
      sprintf(net_data, "T%x:%x:%x:%x:%x:%x:%x:%x %f", addr, celsius);
      
      */
      net_send(19);
      memset (netbuf, 0, sizeof(netbuf));    
      temp_read_in_progress = false;
    }
  } else
  {
    if ((millis() - last_temp_read) > TEMP_READ_FREQ)
    {
      last_temp_read = millis();
      temp_read_in_progress = true;
      
      ds.reset();
      ds.select(sensor_addr);
      ds.write(0x44);         // start conversion       
    }
  }
}

void get_mac()
{
  byte gotmac = 0;  
  int plen, i;
  
  unsigned long time = millis();
  
  // Send arp request
  es.ES_client_arp_whohas(netbuf, dstip);
    
  while(!gotmac)
  {
    plen = es.ES_enc28j60PacketReceive(NET_BUF_SIZE, netbuf); 
    if (plen>0)
    {
      if (es.ES_eth_type_is_arp_and_my_ip(netbuf, plen))
      {          
        gotmac = 1;
        
        // Check the reply is actually for dstip
        for (i=0; i < 4; i++)
          if(netbuf[ETH_ARP_SRC_IP_P+i]!=dstip[i])
            gotmac  = 0;
         
        if (gotmac)
        {
          // Save mac & return
          for (i=0; i < 6; i++)
            dstmac[i] = netbuf[ETH_ARP_SRC_MAC_P + i];     
          
          return;      
        }
      }
    }    
    
    if ((millis() - time) > ARP_RETRY)
    {
      time = millis();
      // Re-send arp request
      es.ES_client_arp_whohas(netbuf, dstip);      
    } 
    

  } // while(!gotmac)
  
}


void disp_progmem_msg(char *msg, int lineno)
{
  int count = 0;
  byte b;
  msg_detail[lineno].msg[count] = 0;
  
  // Clear current message
  memset(msg_detail[lineno].msg, ' ', sizeof(msg_detail[lineno].msg)); 
  
  while(1)
  {
    b = pgm_read_byte_near(msg + count);
    if (!b)
      break;
    msg_detail[lineno].msg[count++] = b;
  }
  msg_detail[lineno].msg_len = count * 5;

  prep_line(lineno);
  
  // Switch to newly generated buffer
  line[lineno].cur_buf = !line[lineno].cur_buf;  
  
}


void int_refresh_screen()
{
  digitalWrite(OE, HIGH);
    
  for(int j=0; j<16; j++) // refresh each row in turn
  {
  
    // Set strobe/clk/data low  
    PORTC &= B11110001;
      
    // Select row
    PORTD &= B10000111;
    PORTD |= j << 3;  
     
    for (int i=0; i<192; i++)  
    { 
      if (j < 8) // Top line
      {
        if (msg_detail[line[LINE_UPPER].msg_details].options & OPTION_INVERT) // Display top line inverted?
        {
          if ( line[0].fbuf[line[LINE_UPPER].cur_buf][i] & (1 << j))
            PORTC &= B11110111;        
          else
            PORTC |= B00001000;        
        } else
        {
          if ( line[0].fbuf[line[LINE_UPPER].cur_buf][i] & (1 << j))
            PORTC |= B00001000;       
          else
            PORTC &= B11110111;                  
        }
      }  
      else // Bottom line
      {
        if (msg_detail[line[LINE_LOWER].msg_details].options & OPTION_INVERT)
        {        
          if ( line[1].fbuf[line[LINE_LOWER].cur_buf][i] & (1 << (j-8)))
            PORTC &= B11110111;        
          else
            PORTC |= B00001000;  
        } else
        {
          if ( line[1].fbuf[line[LINE_LOWER].cur_buf][i] & (1 << (j-8)))
            PORTC |= B00001000;        
          else
            PORTC &= B11110111;            
        }
      }      
      
      // CLK
      PORTC |= B00000100; 
      delayMicroseconds(1);
      PORTC &= B11111011;  

    }
    // strobe
    PORTC |= B00000010;  
  }
  digitalWrite(OE, LOW);

}




// Regenerate the framebuffer; use the position counter to put the text in the right place.
// Update the non-visible buffer for the line.
void prep_line(int lineno)
{
  int   count = 0;
  byte  bufno;
  unsigned int tmp;
  
  bufno = line[lineno].cur_buf; // Get the current active buffer
  
  // Get buffer to update
  bufno = !bufno;  
  
  for (int x = 0; x < 192; x++)
  {
    if ( (x < msg_detail[line[lineno].msg_details].pos) || (x > (msg_detail[line[lineno].msg_details].pos + msg_detail[line[lineno].msg_details].msg_len)) )
    {
      line[lineno].fbuf[bufno][x] = 0;
    } else
    {  
      //line[lineno].fbuf[bufno][x]  = pgm_read_byte_near(System5x8 +  ((msg_detail[line[lineno].msg_details].msg[((x - msg_detail[line[lineno].msg_details].pos)/5)]-32)*5) + ((x-msg_detail[line[lineno].msg_details].pos) % 5) ); 

      tmp = (msg_detail[line[lineno].msg_details].msg[((x - msg_detail[line[lineno].msg_details].pos)/5)]) - 32;
      line[lineno].fbuf[bufno][x]  = pgm_read_byte_near(System5x8 +  (tmp*5) + ((x-msg_detail[line[lineno].msg_details].pos) % 5) ); 

    }
  }
}



void loop() 
{

  wdt_reset();
  
  if ((millis() - lastmove) > 15)
  {
    
    // Scroll any messages with the scroll bit set
    for (int ln=0; ln < 2; ln++)
    {
      if (msg_detail[line[ln].msg_details].options & OPTION_SCROLL)
        if (msg_detail[line[ln].msg_details].pos-- < -1*msg_detail[line[ln].msg_details].msg_len)
        {
          // message has scrolled off screen. Either scroll it back on again, or switch to the next message
          if (line[ln].nxtmsg != 0xFF)
          {
            line[ln].msg_details = line[ln].nxtmsg;      
            line[ln].nxtmsg = 0xFF;
          } else
          {
            msg_detail[line[ln].msg_details].pos = 192;      
          }
        }
    }

    // Scroll any messages with the scroll in bit set, that aren't in possition yet
    for (int ln=0; ln < 2; ln++)
    {
      if ((msg_detail[line[ln].msg_details].options & OPTION_SCROLLIN) && (!(msg_detail[line[ln].msg_details].options & OPTION_SCROLL)))
      {
        if (msg_detail[line[ln].msg_details].pos > 0)
        {
          msg_detail[line[ln].msg_details].pos--;     
        }
        else
        {
          // If nxtmsg is set, and either:
          // * the 'other' line is set to scroll, has nxt msg set, and is about to go of screen, OR
          // * the 'other' line isn't set to scroll
          // update the options to scroll this one of screen
          if (line[ln].nxtmsg != 0xFF)
          {
            
            if (
                (
                  (msg_detail[line[(ln+1)%2].msg_details].options & OPTION_SCROLL) &&
                  (line[(ln+1)%2].nxtmsg != 0xFF) &&
                  ((msg_detail[line[(ln+1)%2].msg_details].msg_len + msg_detail[line[(ln+1)%2].msg_details].pos) < msg_detail[line[ln].msg_details].msg_len)
                ) ||
                (
                  (!(msg_detail[line[(ln+1)%2].msg_details].options & OPTION_SCROLL))
                )
               ) 
            {
              msg_detail[line[ln].msg_details].options |= OPTION_SCROLL;
            }
          }  
        }
      }
    }    
 
    // Update off-screen buffers
    prep_line(LINE_UPPER);
    prep_line(LINE_LOWER);  
    
//    cli();
    // Switch to newly generated buffer
    line[LINE_UPPER].cur_buf = !line[LINE_UPPER].cur_buf;
    line[LINE_LOWER].cur_buf = !line[LINE_LOWER].cur_buf;    
//    sei();  
    
    lastmove = millis();
  }
 
  if ((millis() - flash_timer) > 200)
  {
    if (msg_detail[line[LINE_UPPER].msg_details].options & OPTION_FLASH)
      msg_detail[line[LINE_UPPER].msg_details].options ^= OPTION_INVERT;

    if (msg_detail[line[LINE_LOWER].msg_details].options & OPTION_FLASH)
      msg_detail[line[LINE_LOWER].msg_details].options ^= OPTION_INVERT;

    flash_timer = millis();
  }

  net_loop();
  one_wire_loop();
}

void set_message(int msgno, uint8_t *msg, int msglen)
{
  
  // Clear current message
  memset(msg_detail[msgno].msg, ' ', sizeof(msg_detail[msgno].msg)); 
  
  // Swap any characters that can't be displayed for a space (e.g. CR/LF).
  for (int x=0; x < msglen; x++)
  {
    if ((msg[x] >= ' ') && (msg[x] <= 130)) 
      msg_detail[msgno].msg[x] = msg[x];
    else
      msg_detail[msgno].msg[x] = ' ';        
  }
  
  // Update msglen
  msg_detail[msgno].msg_len = strlen((char*)msg) * SYSTEM5x8_WIDTH;
  
  
  // If changing an active buffer, don't reset the position
  if (!((line[LINE_UPPER].msg_details==msgno) || (line[LINE_LOWER].msg_details==msgno)))
  {
    if (msg_detail[msgno].options & OPTION_SCROLLIN)
      msg_detail[msgno].pos = 192; // start off screen
    else
      msg_detail[msgno].pos = 0; 
  }
    
}

void net_loop()
{
  int i;
  uint16_t plen, dat_p;
  byte msg_type;
  byte bufno;
  byte bufno2;  
  char str_opt[3];
  

  plen = es.ES_enc28j60PacketReceive(NET_BUF_SIZE, netbuf);


  if (plen) // 0 if no packet received
  {
    dat_p=es.ES_packetloop_icmp_tcp(netbuf,plen);

    if (es.ES_eth_type_is_ip_and_my_ip(netbuf, plen)) // Is packet for us?
    {
      // Is it UDP & on the expected port?
      if ((netbuf[IP_PROTO_P] == IP_PROTO_UDP_V) && (netbuf[UDP_DST_PORT_H_P]==PORT_H) && (netbuf[UDP_DST_PORT_L_P] == PORT_L )) 
      {
        msg_type = netbuf[UDP_DATA_P];
        
        Serial.println F("got udp");  
        
        switch (msg_type)
        {
          case MSG_SET_BUFFER:
            bufno = netbuf[UDP_DATA_P+1] - '0';
            if (bufno > 3)
            {
              Serial.println F("Invalid [B]uf message received");  
              break;
            }
            str_opt[0] = netbuf[UDP_DATA_P+2];
            str_opt[1] = netbuf[UDP_DATA_P+3];
            str_opt[2] = 0;
            
            msg_detail[bufno].options = atoi(str_opt);
            set_message(bufno, netbuf+UDP_DATA_P+4, plen-UDP_DATA_P-4);  
            break;
            
          case MSG_SHOW:
            bufno  = netbuf[UDP_DATA_P+1] - '0';
            bufno2 = netbuf[UDP_DATA_P+2] - '0';
            if ((bufno > 4) || (bufno2 > 3))
            {
              Serial.println F("Invalid [S]how message received");  
              break;
            }
            net_show_msg(LINE_UPPER, bufno);
            net_show_msg(LINE_LOWER, bufno2);            
            break; 
            
          default:
            break;
        }

              
      }   
    }
  }
}

void udp_send_prgmem_msg(char *msg)
{
  
  uint8_t *buffer;
  int count = 0;
  uint8_t b;
  
  buffer = netbuf + 42; // first 42 bytes is Ethernet/UDP header

  while(1)
  {
    b = pgm_read_byte_near(msg + count);
    buffer[count++] = b;
    if (!b)
      break;    
  }

  net_send(count);
}

/*
 * Call after populating netbuf[42] with data. call this
 * function with the number of bytes to send as a parameter
 */
void net_send(byte len)
{
  uint8_t i=0;
  uint16_t ck;
  
  // Setup the MAC addresses for ethernet header
  while(i<6)
  {
    netbuf[ETH_DST_MAC +i]= dstmac[i];
    netbuf[ETH_SRC_MAC +i]= mymac[i];
    i++;
  }
  netbuf[ETH_TYPE_H_P] = ETHTYPE_IP_H_V;
  netbuf[ETH_TYPE_L_P] = ETHTYPE_IP_L_V;
  es.ES_fill_buf_p(&netbuf[IP_P],9,iphdr);

  // IP Header
  netbuf[IP_TOTLEN_L_P]=28+len;
  netbuf[IP_PROTO_P]=IP_PROTO_UDP_V;
  i=0;
  while(i<4)
  {
    netbuf[IP_DST_P+i] = dstip[i];
    netbuf[IP_SRC_P+i] = myip[i];
    i++;
  }
  es.ES_fill_ip_hdr_checksum(netbuf);
  netbuf[UDP_DST_PORT_H_P]=PORT_H;
  netbuf[UDP_DST_PORT_L_P]=PORT_L;
  netbuf[UDP_SRC_PORT_H_P]=PORT_H;
  netbuf[UDP_SRC_PORT_L_P]=PORT_L; 
  netbuf[UDP_LEN_H_P]=0;
  netbuf[UDP_LEN_L_P]=8+len;
  // zero the checksum
  netbuf[UDP_CHECKSUM_H_P]=0;
  netbuf[UDP_CHECKSUM_L_P]=0;

  // Create correct checksum
  ck=es.ES_checksum(&netbuf[IP_SRC_P], 16 + len,1);
  netbuf[UDP_CHECKSUM_H_P]=ck>>8;
  netbuf[UDP_CHECKSUM_L_P]=ck& 0xff;
  es.ES_enc28j60PacketSend(42 + len, netbuf);
}



void net_show_msg(int ln, int buf)
{
  // New or previous message is/was set to instant - then switch now, don't wait
  if ((msg_detail[buf].options & OPTION_INSTANT_DISPLAY) || (msg_detail[line[ln].msg_details].options & OPTION_INSTANT_DISPLAY))
  { 
    line[ln].msg_details = buf;      
  } else
  {
    line[ln].nxtmsg = buf;
  }   
}





