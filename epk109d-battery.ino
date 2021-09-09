#include <Wire.h>
#include <U8g2lib.h>
#include <INA219_WE.h>
#include <FastLED.h> 

#define CELL_COUNT 3
#define CELL_V_MIN 2.9
#define CELL_V_MAX 4.2

#define I2C_ADDRESS_INA 0x40

// LED
#define NUM_LEDS 1
#define DATA_PIN 23
#define CLOCK_PIN 22
CRGB leds[NUM_LEDS];


// OLED setup
U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE);  

// INA setup
INA219_WE ina219 = INA219_WE(I2C_ADDRESS_INA);

// LED COLORS IN HUE
#define led_green_hue 42
#define led_red_hue   95
#define led_blue_hue  200
#define led_saturation 250
bool is_sensor_init = false;
bool is_charging = false;
float batt_voltage_min = CELL_V_MIN * CELL_COUNT;
float batt_voltage_max = CELL_V_MAX * CELL_COUNT;
int led_brightness = 0;

void setup() {
  // LED
  FastLED.addLeds<WS2812B, DATA_PIN, RGB>(leds, NUM_LEDS);

  // OLED
  u8x8.begin();
  
  Serial.begin(9600);
  
  // I2C
  Wire.begin();
  if(!ina219.init()){
    Serial.println("INA219 not connected!");
    // TODO put error message on screen
  } else {
    is_sensor_init = true;
  }

  ina219.setMeasureMode(CONTINUOUS); // choose mode and uncomment for change of default
  
}

// graphics helpers
void draw_bar(uint8_t x, uint8_t is_inverse, bool is_cross)
{ 
  uint8_t y;
  int charge_bar_y = 2;
  int charge_bar_height = 5;
  
    
  // draw vertical line
  for( y = charge_bar_y; y < charge_bar_height; y++ )
  {
    u8x8.setCursor(x, y);
    u8x8.setInverseFont(is_inverse);
    if (is_cross) {
      u8x8.print(" ");
    } else {
      u8x8.print(" ");
    }
    u8x8.setInverseFont(!is_inverse);
  }

}

// TODO finish dot animation
void draw_dot(uint8_t x)
{ 
  u8x8.setInverseFont(false);
  uint8_t r;
  int charge_bar_y = 2;
  int charge_bar_height = 5;
//  // clear bar
  for(int r = charge_bar_y; r < charge_bar_height; r++ ) {
      u8x8.setCursor(x, r);
      u8x8.print(" ");
  }
  
  // draw random dot
  int dot_y = random(charge_bar_y, charge_bar_height);
  u8x8.setCursor(x, dot_y);
  u8x8.print("+");
}


void loop() {

  // TODO put it somewhere else to fix text overlaping issue
  // if kept like this it will flicker
  //  u8x8.clear(); 

  // ===== CURRENT SENSOR READINGS =====
  if (!is_sensor_init) {
    // TODO put error message on screen
    exit(0);
  }
  float shuntVoltage_mV = 0.0;
  float loadVoltage_V = 0.0;
  float busVoltage_V = 0.0;
  float current_mA = 0.0;
  float power_mW = 0.0; 
  bool ina219_overflow = false;
  
  shuntVoltage_mV = ina219.getShuntVoltage_mV();
  busVoltage_V = ina219.getBusVoltage_V();
  current_mA = ina219.getCurrent_mA();
  loadVoltage_V  = busVoltage_V + (shuntVoltage_mV/1000);
  ina219_overflow = ina219.getOverflow();

  // detect charging
  is_charging = false;
  if (current_mA < -10) {
    is_charging = true;
  }

  // calc percents
  float percents = (loadVoltage_V - batt_voltage_min) / (batt_voltage_max - batt_voltage_min) * 100;


  // ===== OLED GRAPHICS =====
  
  u8x8.noInverse();
  u8x8.setFont(u8x8_font_amstrad_cpc_extended_f);    

  // AMPS + V
  u8x8.setCursor(1,6); // x, y
  u8x8.print(int(current_mA));
  u8x8.print("mA    ");

  u8x8.setCursor(9,6);
  u8x8.print(busVoltage_V);
  u8x8.print("V");

  // CHARGE %
  u8x8.setCursor(5,0);
  u8x8.print(percents);
  u8x8.print("%");


  // LINE 2
  // PERCENTS BARS ANIMATION
  // total cols = u8x8.getCols() = 16 cols for SSD1306_128X64
  // Serial.print("u8x8.getCols(): "); Serial.println(u8x8.getCols()); 
  // 1 col = 1/16 charge = 6.25%
  int total_rows = 16;
  int bar_count = int(percents)/(100/total_rows);
  for(int x = 0; x < bar_count; x++ )
  {
    draw_bar(x, 1, is_charging);
  }

  // charging animation
  if (is_charging) {
    for(int x = bar_count-1; x < total_rows; x++ ) {
      draw_bar(x, 1, true);
      delay(150);
    }
    // clear
    for(int x = bar_count-1; x < total_rows; x++ ) {
      draw_bar(x, 0, false); // set to black
    }
  } 
  
 
  // CHARGING (blue)
  if (is_charging) {
    led_animate(led_blue_hue);

  // LOW BATTERY (red)
  } else if (percents <= 10) {
    led_animate(led_red_hue);

  // DONE CHARGING (green)
  } else if (is_charging && percents >= 99) {
    led_animate(led_green_hue);
  } else {
      leds[0] = CRGB::Black;
      FastLED.show();
  }

  // delay for readability 
  delay(500);
}


void led_animate(int color) {
    int brightness_start = 00;
    int brightness_end = 255;
    for (int i = brightness_start; i <= brightness_end; i++) {
      leds[0] = CHSV(color, led_saturation, i);
      FastLED.show();
      delay(3);
    }
    
}
