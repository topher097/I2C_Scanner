#define U8X8_HAVE_HW_I2C_T3

#include <Arduino.h>
#include <SPI.h>
#include <Bounce2.h>
#include <U8g2lib.h>
#include <i2c_t3.h>

// OLED declaration
U8G2_SSD1306_128X32_UNIVISION_F_SW_I2C g_OLED(U8G2_R0, 5, 4);
int g_lineHeight = 0;


// scans devices from 50 to 800KHz I2C speeds.
// lower than 50 is not possible
// DS3231 RTC works on 800 KHz. 
//int TWBR = 2;
uint32_t speed[] = {10, 50, 100, 200, 250, 400, 500, 800};
const int speeds = sizeof(speed)/sizeof(speed[0]);
uint8_t count = 0;
uint8_t screenAddress = 60;
struct foundAddresses{};
struct foundSpeeds{};
struct foundHex{};


// DELAY BETWEEN TESTS
#define RESTORE_LATENCY  5    // for delay between tests of found devices.
bool delayFlag = false;

// BUTTON
#define BUTTON_PIN 14
Bounce b = Bounce();

// BLINK LED
#define BLINK 10              // blink LED pin
u_int16_t blinkDelay = 500;   // ms delay
u_int64_t blinkTimer;         // blink timer
bool blinkState = false;      // init LED state to low/false

uint32_t startScan;
uint32_t stopScan;

void blinkLED(){
  if (blinkState){
    digitalWrite(BLINK, LOW);
    blinkState = !blinkState;
  }
  else {
    digitalWrite(BLINK, HIGH);
    blinkState = !blinkState;
    Serial.println("BLINK!");
  }
}


void I2Cscan(){
  Serial.println("starting scan");
  count = 0;

  // if (header)
  // {
  //   Serial.print(F("TIME\tDEC\tHEX\t"));
  //   for (uint8_t s = 0; s < speeds; s++)
  //   {
  //     Serial.print(F("\t"));
  //     Serial.print(speed[s]);
  //   }
  //   Serial.println(F("\t[KHz]"));
  //   for (uint8_t s = 0; s < speeds + 5; s++)
  //   {
  //     Serial.print(F("--------"));
  //   }
  //   Serial.println();
  // }

  // TEST
  // 0.1.04: tests only address range 8..120
  // --------------------------------------------
  // Address  R/W Bit Description
  // 0000 000   0 General call address
  // 0000 000   1 START byte
  // 0000 001   X CBUS address
  // 0000 010   X reserved - different bus format
  // 0000 011   X reserved - future purposes
  // 0000 1XX   X High Speed master code
  // 1111 1XX   X reserved - future purposes
  // 1111 0XX   X 10-bit slave addressing
  for (uint8_t address = 8; address < 127; address++)
  {
    bool found[speeds];
    bool fnd = false;
    bool printLine = false;
    byte error;

    if (address != screenAddress-1)       // Scip screen address
    {
      Serial.printf("\n%d\t", address);
      for (uint8_t s = 0; s < speeds ; s++)
      {
        //Serial.printf("%d\t", speed[s]);
        Wire.setClock(speed[s]*1000);
        Wire.beginTransmission(address);
        //error = Wire.endTransmission();
        found[s] = (Wire.endTransmission() == 0);
        fnd |= found[s];
        // give device 5 millis
        if (fnd && delayFlag) delay(RESTORE_LATENCY);
      }
    }
    else {Serial.printf("%d is screen address", address);}

    if (fnd) count++;;
    if (1)
    {
      Serial.print(millis());
      Serial.print(F("\t"));
      Serial.print(address, DEC);
      Serial.print(F("\t0x"));
      Serial.print(address, HEX);
      Serial.print(F("\t"));

      for (uint8_t s = 0; s < speeds ; s++)
      {
        Serial.print(F("\t"));
        Serial.print(found[s]? F("V"):F("."));
      }
      Serial.println();
      //printLine = false;
    }

  }

  Serial.println("Done");

    


}


void updateOLED(){
  g_OLED.setCursor(0, g_lineHeight);
  g_OLED.printf("Found %d address(es)", count);
  g_OLED.setCursor(0, g_lineHeight*2);
  g_OLED.printf("W: %d", g_OLED.getDisplayWidth());
  g_OLED.setCursor(0, g_lineHeight*3);
  g_OLED.printf("H: %d", g_OLED.getDisplayHeight());
  g_OLED.sendBuffer();
}


void setup()
{
  // Button setup
  pinMode(BUTTON_PIN, INPUT);
  b.attach(BUTTON_PIN,INPUT_PULLUP);
  b.interval(25);

  // Serial port setup
  Serial.begin(115200);
  Wire.begin(I2C_MASTER, 16, 17);
  Wire1.begin(I2C_MASTER, 29, 30);

  // Start blink timer
  pinMode(BLINK, OUTPUT);
  blinkTimer = millis();

  // OLED setup
  g_OLED.begin();
  g_OLED.clear();
  g_OLED.setFont(u8g2_font_profont12_tf);

  g_lineHeight = g_OLED.getFontAscent() - g_OLED.getFontDescent();
  g_OLED.setCursor(0, g_lineHeight);
  g_OLED.print("Press button to scan");
  g_OLED.sendBuffer();
}


void loop(){
  // Restart/Start button
  b.update();
  if (b.fell()){
    Serial.println("Press");
    I2Cscan();
    updateOLED();   // update the OLED after run
  }
  
  // Blink status LED
  if (millis() - blinkTimer >= blinkDelay){
    blinkLED();
    blinkTimer = millis();
    delay(5);
  }
}

