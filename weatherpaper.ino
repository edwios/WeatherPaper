// This #include statement was automatically added by the Particle IDE.
#include "application.h"
#include <MQTT.h>
#include "epd.h"

SYSTEM_MODE(MANUAL);
SYSTEM_THREAD(ENABLED);

extern char* itoa(int a, char* buffer, unsigned char radix);

#define version 100
/*
 * This is a minimal example, see extra-examples.cpp for a version
 * with more explantory documentation, example routines, how to 
 * hook up your pixels and all of the pixel types that are supported.
 *
 */

/*
 * JSON parser example
   StaticJsonBuffer<200> jsonBuffer;

  char json[] =
      "{\"sensor\":\"gps\",\"time\":1351824120,\"data\":[48.756080,2.302038]}";

  JsonObject& root = jsonBuffer.parseObject(json);

  if (!root.success()) {
    Serial.println("parseObject() failed");
    return;
  }

  const char* sensor = root["sensor"];
  long time = root["time"];
  double latitude = root["data"][0];
  double longitude = root["data"][1];
  
 *
 */
  

#define CLIENT_NAME "envconsole08"

#define TP_HOME "sensornet/env/home/#"
#define TP_HOME_ALARM "sensornet/status/#"
#define TP_COMMAND "sensornet/command/envconsole08/#"
#define TP_LIVTEMP  "/living/temperature"
#define TP_LIVHUMI  "/living/humidity"
#define TP_LIVAQI  "/living/aqi"
#define TP_LIVMOTION  "/living/motion"
#define TP_BALTEMP  "/balcony/temperature"
#define TP_BALHUMI  "/balcony/humidity"
#define TP_BALAQI  "/balcony/aqi"
#define TP_STATUS   "sensornet/status/envconsole08"
#define TP_TIMESTR "sensornet/time/shortdatetime"
#define ARMED    "ARMED"
#define DISARMED   "DISARMED"
#define CMD_DISP_OFF "display/off"
#define CMD_DISP_ON "display/on"
 
#define HBLED D7                // Blinking HeartBeat LED
#define BEEPER D6               // Beeper
#define ERRFLASHTIME 100        // How fast do I blink when error
#define HBFLASHTIME 250         // How fast do I blink normally
#define RGBLEDRELASETIME 3500   // 3.5s to release control of RBG LED
#define LCDRELASETIME 15000     // 15s to release LCD
#define LCDREFRESHTIME 60000    // 60s refresh
#define RECONNECT 10000         // Try reconnect every 10s

#define LCD_TEXT 1
#define LCD_VALUE 2
#define LCD_PICT 3
#define LCD_PAGESELECT 4
#define LCD_FONTCOLOR 5
#define LCD_DIRECT 99

#define BASEPICT "p0"
#define OUTDOORTEMPFIELD "t0"
#define OUTDOORHUMIFIELD "t1"
#define INDOORTEMPFIELD "t2"
#define INDOORHUMIFIELD "t3"
#define STATUSFIELD "t4"
#define CLOCKFIELD "t5"
#define DATEFIELD "t6"
#define ALARMFIELD "t7"
#define OUTDOORAQIFIELD "t8"
#define INDOORAQIFIELD "t9"

#define OUTDOORTEMP_X 50
#define OUTDOORHUMI_X 50
#define OUTDOORAQI_X 50
#define INDOORTEMP_X 450
#define INDOORHUMI_X 450
#define INDOORAQI_X 450
#define STATUS_X 10
#define CLOCK_X 410
#define DATE_X 250
#define ALARM_X 600
#define OUTDOORTEMP_Y 200
#define OUTDOORHUMI_Y 300
#define OUTDOORAQI_Y 400
#define INDOORTEMP_Y 200
#define INDOORHUMI_Y 300
#define INDOORAQI_Y 400
#define STATUS_Y 560
#define CLOCK_Y 10
#define DATE_Y 10
#define ALARM_Y 20

#define FAST 125
#define SLOW 500

String lcdElems[] = {OUTDOORTEMPFIELD, OUTDOORHUMIFIELD, OUTDOORAQIFIELD, INDOORTEMPFIELD, INDOORHUMIFIELD, INDOORAQIFIELD,
                    STATUSFIELD, CLOCKFIELD, DATEFIELD, ALARMFIELD, BASEPICT};
int forecast[] = {0,1,2,3,4,5};
int timpicts[] = {3,5,2,4};
int lasttimepict = 99;

void callback(char* topic, byte* payload, unsigned int length);

// JSON Parser

byte server[] = { 10,0,1,250 };
MQTT client(server, 1883, callback);

// Various MQTT topics of interest
String tp_livtemp = TP_LIVTEMP;
String tp_livhumi = TP_LIVHUMI;
String tp_livaqi = TP_LIVAQI;
String tp_baltemp = TP_BALTEMP;
String tp_balhumi = TP_BALHUMI;
String tp_balaqi = TP_BALAQI;
String tp_status = TP_STATUS;

String livtemp="-",livhumi="-",livaqi="-",baltemp="-",balhumi="-",balaqi="-",sstatus="-",timeStr="Mon 00 00:00";

// timers
unsigned int d7Timer, ledTimer, lcdTimer, hbTimer;
unsigned long lastConnect;
int d7Rate = 250;

// flags
bool d7On = false;
bool init = true;
bool virgin = true;
bool blink;
bool connected=false;
bool armed=false;

// received command
char cmd[8];

// recieve message
void callback(char* topic, byte* payload, unsigned int length) {
    char p[length + 1];
    char mesg[128];
    String m;
    String tp = String(topic);

    m.reserve(10);
    memcpy(p, payload, length);
    p[length] = NULL;
    m = String(p);
    if (tp.endsWith(TP_LIVTEMP)) {
        // Living room temperature
        m = String(abs(m.toFloat()+0.5));
        m.concat("C");
        livtemp = m;
    } else if (tp.endsWith(TP_LIVHUMI)) {
        // Living room temperature
        m = String(abs(m.toFloat()+0.5));
        m.concat("%");
        livhumi = m;
    } else if (tp.endsWith(TP_LIVAQI)) {
        livaqi = m;
    } else if (tp.endsWith(TP_BALHUMI)) {
        // Balcony temperature
        m = String(abs(m.toFloat()+0.5));
        m.concat("%rH");
        balhumi = m;
    } else if (tp.endsWith(TP_BALTEMP)) {
        // Baconly temperature
        m = String(abs(m.toFloat()+0.5));
        m.concat("C");
        baltemp = m;
    } else if (tp.endsWith(TP_BALAQI)) {
        balaqi = m;
    } else if (tp.startsWith("sensornet/status/")) {
        if (m.equals(ARMED)) {
            armed = true;
            digitalWrite(BEEPER, LOW);
            delay (10);
            digitalWrite(BEEPER, HIGH);
        } else if (m.equals(DISARMED)) {
            armed = false;
            digitalWrite(BEEPER, LOW);
            delay (10);
            digitalWrite(BEEPER, HIGH);
            delay (300);
            digitalWrite(BEEPER, LOW);
            delay (10);
            digitalWrite(BEEPER, HIGH);
        }
    } else if (tp.startsWith("sensornet/command/envconsole08/")) {
        if (tp.endsWith(CMD_DISP_OFF)) {
            lcdOff();
        } else if (tp.endsWith(CMD_DISP_ON)) {
            lcdOn();
        }
    } else if (tp.equals(TP_TIMESTR)) {
        timeStr = m;
    }
//    updateAll();
}

void drawBackground() 
{
    epd_fill_rect(0, CLOCK_Y + 68, 800, CLOCK_Y +70);
    draw_text("Indoor", INDOORTEMP_X, INDOORTEMP_Y - 80, ASCII32);
    draw_text("Outdoor", OUTDOORTEMP_X, OUTDOORTEMP_Y - 80, ASCII32);
//    epd_draw_line(OUTDOORTEMP_X - 40, INDOORTEMP_Y - 40, INDOORTEMP_X + 300, INDOORTEMP_Y - 40);
//    epd_draw_line(400, INDOORTEMP_Y - 40, 400, OUTDOORHUMI_Y + 84);
    epd_fill_rect(0, STATUS_Y - 4, 800, STATUS_Y - 6);
}

void updateAll()
{
    // updateBkgPict();
    drawBackground();
    sendToLCD(LCD_TEXT,INDOORTEMPFIELD,livtemp);
    sendToLCD(LCD_TEXT,INDOORHUMIFIELD,livhumi);
    sendToLCD(LCD_TEXT,OUTDOORHUMIFIELD,balhumi);
    sendToLCD(LCD_TEXT,OUTDOORTEMPFIELD,baltemp);
    sendToLCD(LCD_TEXT,INDOORAQIFIELD,livaqi);
    sendToLCD(LCD_TEXT,OUTDOORAQIFIELD,balaqi);
    sendToLCD(LCD_TEXT,STATUSFIELD,sstatus);
    if (armed)
        sendToLCD(LCD_TEXT,ALARMFIELD,"ARM");
    else
        sendToLCD(LCD_TEXT,ALARMFIELD,"   ");
}

void changePict(uint8_t pict)
{
    String s;
    
}

void sendToLCD(uint8_t type,String index, String cmd)
{
    uint16_t x=0, y=0;
    uint8_t s = ASCII64;
    
	if (type == LCD_TEXT ){
	    if (index.equals(INDOORTEMPFIELD)) {
            x = INDOORTEMP_X; y = INDOORTEMP_Y;
	    } else if (index.equals(INDOORHUMIFIELD)) {
            x = INDOORHUMI_X; y = INDOORHUMI_Y;
	    } else if (index.equals(OUTDOORHUMIFIELD)) {
            x = OUTDOORHUMI_X; y = OUTDOORHUMI_Y;
	    } else if (index.equals(OUTDOORTEMPFIELD)) {
	        x = OUTDOORTEMP_X; y = OUTDOORTEMP_Y;
	    } else if (index.equals(INDOORAQIFIELD)) {
	        x = INDOORAQI_X; y = INDOORAQI_Y;
	    } else if (index.equals(OUTDOORAQIFIELD)) {
	        x = OUTDOORAQI_X; y = OUTDOORAQI_Y;
	    } else if (index.equals(STATUSFIELD)) {
	        s = ASCII32;
	        x = STATUS_X; y = STATUS_Y;
	    } else if (index.equals(ALARMFIELD)) {
	        s = ASCII48;
	        x = ALARM_X; y = ALARM_Y;
	    } else if (index.equals(CLOCKFIELD)) {
	        x = CLOCK_X; y = CLOCK_Y;
	    } else if (index.equals(DATEFIELD)) {
	        x = DATE_X; y = DATE_Y;
	    }
	    draw_text(cmd, x, y, s);
	}
	else if (type == LCD_VALUE){
	}
	else if (type == LCD_PICT){
	}
	else if (type == LCD_PAGESELECT ){
	} else if (type == LCD_FONTCOLOR){
	} else if (type == LCD_DIRECT){         // 99 pass over the command
	}
	
}

void lcdOn() {
    epd_wakeup();
}

void lcdOff() {
    epd_enter_stopmode();
}

void setMqtt(void)
{
    client.publish(TP_STATUS,"ready");
    client.subscribe(TP_HOME);
    client.subscribe(TP_HOME_ALARM);
    client.subscribe(TP_COMMAND);
    client.subscribe(TP_TIMESTR);
}

bool updateBkgPict(void)
{
    bool updated = false;
    int th = Time.hour();
    if (th>20 || th<6) {    // Night
        if (lasttimepict != timpicts[3]) {
            changePict(timpicts[3]);
            sendToLCD(LCD_FONTCOLOR,OUTDOORTEMPFIELD,"65535");
            sendToLCD(LCD_FONTCOLOR,OUTDOORHUMIFIELD,"65535");
            sendToLCD(LCD_FONTCOLOR,CLOCKFIELD,"64487");
            sendToLCD(LCD_FONTCOLOR,DATEFIELD,"64487");
            lasttimepict = timpicts[3];
            updated= true;
        }
    } else if (th > 15 && th <= 20) {
        if (lasttimepict != timpicts[2]) {
            changePict(timpicts[2]);
            lasttimepict = timpicts[2];
            sendToLCD(LCD_FONTCOLOR,OUTDOORTEMPFIELD,"0");
            sendToLCD(LCD_FONTCOLOR,OUTDOORHUMIFIELD,"0");
            sendToLCD(LCD_FONTCOLOR,CLOCKFIELD,"0");
            sendToLCD(LCD_FONTCOLOR,DATEFIELD,"0");
            updated= true;
        }
    } else if (th >= 12 && th <= 15) {
        if (lasttimepict != timpicts[1]) {
            changePict(timpicts[1]);
            sendToLCD(LCD_FONTCOLOR,OUTDOORTEMPFIELD,"0");
            sendToLCD(LCD_FONTCOLOR,OUTDOORHUMIFIELD,"0");
            sendToLCD(LCD_FONTCOLOR,CLOCKFIELD,"64487");
            sendToLCD(LCD_FONTCOLOR,DATEFIELD,"64487");
            lasttimepict = timpicts[1];
            updated= true;
        }
    } else {
        if (lasttimepict != timpicts[0]) {
            changePict(timpicts[0]);
            sendToLCD(LCD_FONTCOLOR,OUTDOORTEMPFIELD,"0");
            sendToLCD(LCD_FONTCOLOR,OUTDOORHUMIFIELD,"0");
            sendToLCD(LCD_FONTCOLOR,CLOCKFIELD,"64487");
            sendToLCD(LCD_FONTCOLOR,DATEFIELD,"64487");
            lasttimepict = timpicts[0];
            updated= true;
        }
    }
    if (updated) sendToLCD(LCD_PICT,"q0","8");
    return updated;
}


void draw_text(String str, uint16_t x, uint16_t y, uint8_t s)
{
//  String s = "谭丹儿小朋友";
//  char buff[13];
//  s.toCharArray(buff, 12);
//  char buff[] = {'H', 'y', 0xcc, 0xb7, 0xb5, 0xa4, 0xb6, 0xf9, 0x00};
    char buff[128];
    str.toCharArray(buff, 128);
    epd_set_ch_font(s);
    epd_set_en_font(s);
    epd_disp_string(buff, x, y);
}


void setup() 
{
    Time.zone(+8);
    Serial.begin(9600);
    pinMode(HBLED, OUTPUT);
    pinMode(BEEPER, OUTPUT);
    digitalWrite(BEEPER, HIGH);

    d7Timer = millis();
    ledTimer = millis();
    delay(50);
    
//    sendToLCD(LCD_TEXT,STATUSFIELD,"Initializing");
    digitalWrite(HBLED, HIGH);
    WiFi.on();
    delay(500);
    WiFi.connect();
    delay(500);
    
    // Init ePaper
    epd_init();
    epd_reset();
    epd_wakeup();
    epd_set_memory(MEM_NAND);
    epd_set_color(BLACK, WHITE);

    lcdTimer = millis();
    digitalWrite(HBLED, LOW);

}




void loop() 
{
//    char cTime[10];

    // Maintain connection
    if (millis() - ledTimer > RGBLEDRELASETIME) {
        ledTimer = millis();
        RGB.control(false);
        d7Rate = SLOW;
    }
    
    if (WiFi.ready() && init) {
        // connect to the server
        client.connect(CLIENT_NAME);
        delay(500);
        // publish/subscribe
        setMqtt();
        delay(200);
        init = false;
    }

    // Flash D7 according to the statue (controlled by d7Rate)
    if (millis() - d7Timer > d7Rate) {
        d7Timer = millis();
        if (d7On) {
            digitalWrite(HBLED,LOW);
        } else {
            digitalWrite(HBLED,HIGH);
        }
        d7On = !d7On;

        if (client.isConnected()) {
            client.loop();
            connected = true;
            sstatus = "Connected";
        } else if (WiFi.ready()) {
            connected = false;
            sstatus = "Not connected";
            RGB.control(true);
            RGB.color(255,0,0);
            d7Rate = FAST;
            ledTimer = millis();
            if (millis() - lastConnect > RECONNECT) {
                lastConnect = millis();

                RGB.color(40,0,0);
                WiFi.off();
                digitalWrite(D7, HIGH);
                delay(1000);
                WiFi.on();
                RGB.color(255,255,0);
                delay(500);
                WiFi.connect();
                RGB.color(0,0,255);
                delay(500);
                digitalWrite(D7,LOW);
                init = true;
            }
        } else {
            RGB.control(true);
            RGB.color(255,0,255);
            ledTimer = millis();
        }
    }


/*
    if (millis() - hbTimer > (connected?HBFLASHTIME:ERRFLASHTIME)) {
        hbTimer = millis();
    }
*/
    
    if (virgin || (millis() - lcdTimer > LCDREFRESHTIME)) {
        lcdTimer = millis();
        virgin = false;         // Only once per life time ;)

        lcdOn();
        epd_clear();
        updateAll();
//        sprintf(cTime, "%02d:%02d", Time.hour(), Time.minute());
//        sendToLCD(LCD_TEXT, CLOCKFIELD, String(cTime));
//        sendToLCD(LCD_TEXT, DATEFIELD, Time.format(Time.now(),"%h %d"));
        sendToLCD(LCD_TEXT, DATEFIELD, timeStr);
        sendToLCD(LCD_TEXT,STATUSFIELD,sstatus);
        epd_udpate();
        lcdOff();
    }

}


