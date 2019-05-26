#include <FastLED.h>

const int sampleWindow = 50; //in ms
const int signalPin = 0;
#define LED_PIN 2
#define NUM_LEDS 150
CRGB leds[NUM_LEDS];

typedef struct
{
  uint32_t position;
  uint32_t color;
  uint8_t lifetime;
  bool direction;
} dot_t;

const int MAX_NUM_DOTS = 40;
dot_t dot[MAX_NUM_DOTS];
dot_t old_dot[MAX_NUM_DOTS];
int num_dots_active;
int num_old_dots_active;
unsigned int peakToPeak;
unsigned int volt_thresh = 0;
unsigned int volt_avg = 500;
const float THRESH_FAC_MIN = 1.2;
const float THRESH_FAC_MAX = 1.2;
float thresh_fac = 1.5;
const float thresh_fac_fac = 0.03;
const CRGB OFF_COLOR = CRGB(4,4,4);
bool dot_creating = false;

const float vtfac = 0.03;
const int MV_SPEED = 2;
const int DOT_WIDTH = 4;
const int MAX_LIFETIME = 10;
const int COLOR_SHIFT = 5;

const int SAT_MIN = 150;
const int SAT_MAX = 152;
const int BRT_MIN = 150;
const int BRT_MAX = 152;

void setup()
{
  // put your setup code here, to run once:
  delay(1000);
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  
  num_dots_active = 0;
  num_old_dots_active = 0;
  randomSeed(420);

  delay(500);
  uint32_t color = 0 - NUM_LEDS;
  while (color >= 0 - NUM_LEDS || color < 0xFF) {
    for (int i = 0; i < NUM_LEDS; i++) {
      if (color + i > 0xFF)
        leds[i] = CRGB(0,0,0);
      else
        leds[i] = CHSV(color+i, 255, color + NUM_LEDS < 0xFF ? color + NUM_LEDS : 0xFF);
    }
    FastLED.show();
    color ++;
  }


  for (int i = 0; i < NUM_LEDS; i++)
    leds[i] = OFF_COLOR;
  FastLED.show();
  
  //Serial.begin(9600);
}

void loop()
{
  unsigned long startMillis = millis(); // Start of sample window

  unsigned int signalMax = 0;
  unsigned int signalMin = 1024;

  // collect data for 50 mS
  while (millis() - startMillis < sampleWindow)
  {
    unsigned int sample = analogRead(signalPin);
    if (sample < 1024) // toss out spurious readings
    {
      if (sample > signalMax)
      {
        signalMax = sample; // save just the max levels
      }
      else if (sample < signalMin)
      {
        signalMin = sample; // save just the min levels
      }
    }
  }
  peakToPeak = signalMax - signalMin;       // max - min = peak-peak amplitude
  
  analyze_music();
  analyze_midi();
  update_dot_locations();
}

void analyze_music()
{
  //crossed thresh?
  if (peakToPeak > volt_thresh) {
    //if so, add new dot
    add_dot(int(volt_thresh) - int(peakToPeak));
    thresh_fac = (thresh_fac_fac) * THRESH_FAC_MIN + thresh_fac * (1-thresh_fac_fac);
  } else {
    end_dot();
    thresh_fac = (thresh_fac_fac) * THRESH_FAC_MAX + thresh_fac * (1-thresh_fac_fac);
  }

  //update thresh based on current volume and number of recent dots
  //...or to make it easy, let's just go with increases in volume make dots!
  volt_avg = vtfac * peakToPeak + (1-vtfac) * volt_avg;
  volt_thresh = volt_avg * thresh_fac;
}

void analyze_midi()
{
  //get velocity of on signals

  //number of new dots, add em!
}

void update_dot_locations()
{
  num_old_dots_active = num_dots_active;
  int old_iter = 0;

  for (int i = 0; i < num_dots_active; i++)
  {
    old_dot[old_iter++] = dot[i];

    if (is_corner(dot[i].position))
    {
      dot[i].direction = !dot[i].direction;
    }
 
    dot[i].position += dot[i].direction ? MV_SPEED : -MV_SPEED;
    
    dot[i].lifetime++;

    if (dot[i].position > NUM_LEDS || dot[i].lifetime > MAX_LIFETIME) {
      for (int j = i; j < num_dots_active - 1; j++) {
        dot[j] = dot[j+1];
      }
      num_dots_active--;
      i--;
    } else {
      //update color
      //int r = random(3);
      //int shift = r * 8;
      //uint8_t shift_amount = 5;
      //uint8_t oldc = dot[i].color & (0xFF << shift);
      //uint8_t newc = oldc + shift_amount;
      //dot[i].color = (dot[i].color & (~0xFF << shift)) | (newc << shift);
      
    }
  }

  for (int i = 0; i < num_old_dots_active; i++)
  {
    for (int j = 0; j < DOT_WIDTH; j++)
      light_off(old_dot[i].position+j);
  }
  for (int i = 0; i < num_dots_active; i++)
  {
    for (int j = 0; j < DOT_WIDTH; j++)
      light_on(dot[i].position+j, dot[i].color);
  }

  FastLED.show();
  //Serial.println(num_dots_active);
}

bool is_corner(uint32_t pos) {
  return (pos < DOT_WIDTH || pos > NUM_LEDS - DOT_WIDTH);
}

void light_on(uint32_t pos, uint32_t color)
{
  if (pos > NUM_LEDS)
    return;
  
  //leds[pos] = CHSV(color & 0xFF, (color & 0xFF00) >> 8, (color & 0xFF0000) >> 16);
  leds[pos] = CHSV(color, SAT_MIN, BRT_MIN);
}

void light_off(uint32_t pos)
{
  if (pos > NUM_LEDS)
    return;
  
  leds[pos] = OFF_COLOR;
}

void add_dot(int color_param)
{
  if (num_dots_active == MAX_NUM_DOTS)
    return;

  if (dot_creating) {
    dot[num_dots_active].position = dot[num_dots_active-1].position + (dot[num_dots_active].direction ? DOT_WIDTH : -DOT_WIDTH);
    dot[num_dots_active].color = dot[num_dots_active-1].color;
    dot[num_dots_active].direction = dot[num_dots_active-1].direction;
    dot[num_dots_active].lifetime = 0;
  } else {
    dot[num_dots_active].position = random(NUM_LEDS);
    dot[num_dots_active].color = random(0xFF+1);// | (random(SAT_MIN,SAT_MAX+1) << 8) | (random(BRT_MIN,BRT_MAX+1) << 16);
    dot[num_dots_active].direction = random(2);
    dot[num_dots_active].lifetime = 0;
  
    //dot[num_dots_active].position = 0;
    //dot[num_dots_active].color = 0xFF;
    //dot[num_dots_active].direction = true;
    dot[num_dots_active].lifetime = 0;
  }
  
  num_dots_active++;
  dot_creating = true;
}

void end_dot() {
  dot_creating = false;
}
