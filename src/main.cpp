// CODE MODIFIED FROM ADVANCED_SCANNER.INO ON I2C_T3 GITHUB //

#include <Arduino.h>
#include <SPI.h>
#include <Bounce2.h>
#include <U8g2lib.h>
#include <i2c_t3.h>

// OLED declaration
U8G2_SSD1306_128X32_UNIVISION_F_SW_I2C g_OLED(U8G2_R0, 19, 18);
int g_lineHeight = 0;

// BUTTON
#define BUTTON_PIN 14
Bounce b = Bounce();
bool firstScan = false;

// BLINK LED
#define BLINK 10              // blink LED pin
IntervalTimer blinkTimer;
int blinkDelay = 500;         // ms delay
bool blinkState = false;      // init LED state to low/false

// Blink status LED
void blinkLED(){
  if (blinkState){
    blinkState = !blinkState;
  }
  else {
    blinkState = !blinkState;
  }
  digitalWrite(BLINK, blinkState);
}


byte targetFound = 0;
uint8_t found = 0;



void updateOLED(){
  g_OLED.clearDisplay();
  g_OLED.setCursor(0, g_lineHeight);
  if(!found){
    g_OLED.print("No devices found");
  }
  else{
    g_OLED.print("Found an address!");
    g_OLED.setCursor(0, g_lineHeight*2);
    g_OLED.printf("Addr: 0x%02X", targetFound);
  }
  
  g_OLED.setCursor(0, g_lineHeight*3);
  g_OLED.printf("Press button to scan");
  g_OLED.sendBuffer();
}

#define TARGET_START 0x01
#define TARGET_END   0x7F

#define WIRE_PINS   I2C_PINS_18_19
#if defined(__MKL26Z64__)               // LC
#define WIRE1_PINS   I2C_PINS_22_23
#endif
#if defined(__MK20DX256__)              // 3.1-3.2
#define WIRE1_PINS   I2C_PINS_29_30
#endif
#if defined(__MK64FX512__) || defined(__MK66FX1M0__)  // 3.5/3.6
#define WIRE1_PINS   I2C_PINS_37_38
#define WIRE2_PINS   I2C_PINS_3_4
#endif
#if defined(__MK66FX1M0__)              // 3.6
#define WIRE3_PINS   I2C_PINS_56_57
#endif


// -------------------------------------------------------------------------------------------
// Function prototypes
void scan_bus(i2c_t3& Wire, uint8_t all);
void print_bus_status(i2c_t3& Wire);
void print_scan_status(struct i2cStruct* i2c, uint8_t target, uint8_t& found, uint8_t all);

// -------------------------------------------------------------------------------------------
void setup()
{
  // Power for I2C device
  pinMode(2, HIGH);
  pinMode(3, LOW);

  // Setup for Master mode, all buses, external pullups, 400kHz, 10ms default timeout
  //
  Wire.begin(I2C_MASTER, 0x00, WIRE_PINS, I2C_PULLUP_EXT, 400000);
  Wire.setDefaultTimeout(10000); // 10ms
  #if I2C_BUS_NUM >= 2
  Wire1.begin(I2C_MASTER, 0x00, WIRE1_PINS, I2C_PULLUP_EXT, 400000);
  Wire1.setDefaultTimeout(10000); // 10ms
  #endif

  // Begin serial
  Serial.begin(115200);

  // Button setup
  pinMode(BUTTON_PIN, INPUT);
  b.attach(BUTTON_PIN, INPUT_PULLUP);
  b.interval(25);

  // Start blink timer
  pinMode(BLINK, OUTPUT);
  blinkTimer.begin(blinkLED, blinkDelay*1000);
  blinkTimer.priority(1);

  // OLED setup
  g_OLED.begin();
  g_OLED.clear();
  g_OLED.setFont(u8g2_font_profont12_tf);
  g_lineHeight = g_OLED.getFontAscent() - g_OLED.getFontDescent();
  g_OLED.setCursor(0, g_lineHeight*2);
  g_OLED.print("Press button to scan");
  g_OLED.sendBuffer();
}


// -------------------------------------------------------------------------------------------
void loop()
{
  // Scan I2C addresses
  //
  b.update();
  if (b.fell()){
    uint8_t all = 0;

    Serial.print("---------------------------------------------------\n");
    Serial.print("Bus Status Summary\n");
    Serial.print("==================\n");
    Serial.print(" Bus    Mode   SCL  SDA   Pullup   Clock\n");
    print_bus_status(Wire);
    #if I2C_BUS_NUM >= 2
    print_bus_status(Wire1);
    #endif

    scan_bus(Wire, all);
    #if I2C_BUS_NUM >= 2
    scan_bus(Wire1, all);
    updateOLED();             // only display Wire1 bus address
    #endif

    Serial.print("---------------------------------------------------\n\n\n");

    delay(500); // delay to space out tests
  }
}

// -------------------------------------------------------------------------------------------
// scan bus
//
void scan_bus(i2c_t3& Wire, uint8_t all)
{
    uint8_t target = 0;
    found = 0;
    targetFound = 0;
    
    Serial.print("---------------------------------------------------\n");
    if(Wire.bus == 0)
        Serial.print("Starting scan: Wire\n");
    else
        Serial.printf("Starting scan: Wire%d\n",Wire.bus);
    
    for(target = TARGET_START; target <= TARGET_END; target++) // sweep addr, skip general call
    {
        Wire.beginTransmission(target);       // slave addr
        Wire.endTransmission();               // no data, just addr
        print_scan_status(Wire.i2c, target, found, all);
    }
    
    if(!found) Serial.print("No devices found.\n");
}

// -------------------------------------------------------------------------------------------
// print bus status
//
void print_bus_status(i2c_t3& Wire)
{
    struct i2cStruct* i2c = Wire.i2c;
    if(Wire.bus == 0)
        Serial.print("Wire   ");
    else
        Serial.printf("Wire%d  ",Wire.bus);
    switch(i2c->currentMode)
    {
    case I2C_MASTER: Serial.print("MASTER  "); break;
    case I2C_SLAVE:  Serial.print(" SLAVE  "); break;
    }
    Serial.printf(" %2d   %2d  ", Wire.i2c->currentSCL, Wire.i2c->currentSDA);
    switch(i2c->currentPullup)
    {
    case I2C_PULLUP_EXT: Serial.print("External  "); break;
    case I2C_PULLUP_INT: Serial.print("Internal  "); break;
    }
    Serial.printf("%d Hz\n",i2c->currentRate);
}

// -------------------------------------------------------------------------------------------
// print scan status
//
void print_scan_status(struct i2cStruct* i2c, uint8_t target, uint8_t& found, uint8_t all)
{
    switch(i2c->currentStatus)
    {
    case I2C_WAITING:  Serial.printf("Addr: 0x%02X ACK\n",target); found=1; targetFound = target; break;
    case I2C_ADDR_NAK: if(all) { Serial.printf("Addr: 0x%02X\n",target); } break;
    case I2C_TIMEOUT:  if(all) { Serial.printf("Addr: 0x%02X Timeout\n",target); } break;
    default: break;
    }

}
