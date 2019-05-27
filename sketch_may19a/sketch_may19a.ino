#include <FastLED.h>

int sampleWindow = 50; //in ms
const int min_sample_window = 50;
const int max_sample_window = 150;
float sample_window_out_fac = 1;
float spk_amt = 3;
//loat spk_amt = 1;
const float sample_window_fall_speed = 0.95;
const int spike_chance = 1000;
const int signalPin = 0;
#define LED_PIN 2
#define NUM_LEDS 150
const int CORNER_LIGHT = 8;
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
const float THRESH_FAC_MAX = 1.5;
float thresh_fac = 1.5;
const float thresh_fac_fac = 0.03;
const CRGB OFF_COLOR = CRGB(4,4,4);
bool dot_creating = false;
int8_t color_off = 0;
unsigned int color_range = 50;
const int walk_speed = 1;
const int walk_fac = 2;
bool jump_down;
unsigned int last_millis;

const float vtfac = 0.03;
const int MV_SPEED = 2;
const int DOT_WIDTH = 4;
const int MAX_LIFETIME = 15;
const int COLOR_SHIFT = 5;

const int SAT_MIN = 150;
const int SAT_MAX = 152;
const int BRT_MIN = 250;
const int BRT_MAX = 152;

unsigned int avg_diff;
const float speed_fac = 0.3;

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
        leds[i] = CHSV(color+i, 255, color + NUM_LEDS < 32 ? color + NUM_LEDS : 32);
    }
    FastLED.show();
    color ++;
  }


  for (int i = 0; i < NUM_LEDS; i++)
    leds[i] = CHSV(0,0,0);

  for (int i = 0; i < CORNER_LIGHT; i++)
    leds[i] = CHSV(0,0,50);
  FastLED.show();
  
  Serial.begin(9600);
}

void loop()
{
  unsigned long startMillis = millis(); // Start of sample window

  unsigned int signalMax = 0;
  unsigned int signalMin = 1024;

  if (random(spike_chance) == 1) {
    sample_window_out_fac = spk_amt;
  } else {
    if (sample_window_out_fac > 1) {
      sample_window_out_fac = sample_window_out_fac * sample_window_fall_speed;
    } 
    if (sample_window_out_fac < 1)
      sample_window_out_fac = 1;
  }

/*
  if (random(spike_chance) == 1) {
    sampleWindow = max_sample_window;
  } else {
    if (sampleWindow > min_sample_window) {
      sampleWindow = sampleWindow * sample_window_fall_speed;
    } 
    if (sampleWindow < min_sample_window)
      sampleWindow = min_sample_window;
  }
*/

const int SLOW_BEATS_THRESH = 1000;
  if (avg_diff < SLOW_BEATS_THRESH) {
    sampleWindow = max_sample_window;
  } else {
    sampleWindow = min_sample_window;
  }

  if (jump_down)
    sampleWindow = min_sample_window;
  jump_down = false;

  // collect data for 50 mS
  while (millis() - startMillis < sampleWindow * sample_window_out_fac)
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
  //peakToPeak = random(1024);
  
  analyze_music();
  analyze_midi();
  update_dot_locations();
}

void analyze_music()
{
  //crossed thresh
  const int ABS_MIN_THRESH = 10;
  if (peakToPeak > volt_thresh && peakToPeak > ABS_MIN_THRESH) {
    //if so, add new dot
    if (dot_creating) {
      avg_diff = avg_diff * (1-speed_fac) + (millis() - last_millis) * speed_fac;
      last_millis = millis();
    }
    
    add_dot(int(volt_thresh) - int(peakToPeak));

    thresh_fac = (thresh_fac_fac) * THRESH_FAC_MIN + thresh_fac * (1-thresh_fac_fac);
    
    if (peakToPeak > volt_thresh * 2)
      jump_down = true;

  } else {
    end_dot();
    thresh_fac = (thresh_fac_fac) * THRESH_FAC_MAX + thresh_fac * (1-thresh_fac_fac);
  }

  //update thresh based on current volume and number of recent dots
  //...or to make it easy, let's just go with increases in volume make dots!
  volt_avg = vtfac * peakToPeak + (1-vtfac) * volt_avg;
  volt_thresh = volt_avg * thresh_fac;

  if (random(walk_fac) == 0)
    color_off += random(-walk_speed, walk_speed);
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
      light_on(dot[i].position+j, dot[i].color, float(dot[i].lifetime) / float(MAX_LIFETIME));
  }

  FastLED.show();
  //Serial.println(num_dots_active);
}

bool is_corner(uint32_t pos) {
  return (pos < DOT_WIDTH || pos > NUM_LEDS - DOT_WIDTH);
}

void light_on(uint32_t pos, uint32_t color, float fade)
{
  if (pos > NUM_LEDS || pos < CORNER_LIGHT)
    return;
  
  //leds[pos] = CHSV(color & 0xFF, (color & 0xFF00) >> 8, (color & 0xFF0000) >> 16);
  leds[pos] = CHSV(color + fade * 100, SAT_MIN * (1- fade * fade * fade * fade), BRT_MIN * (1-fade));
}

void light_off(uint32_t pos)
{
  if (pos > NUM_LEDS || pos < CORNER_LIGHT)
    return;
  
  //leds[pos] = OFF_COLOR;
  //uint8_t cc = (color_off + 100) % 256;
  uint8_t cc = 210;
  leds[pos] = CHSV(cc, 150, 1);
  //Serial.println(cc);
}

void add_dot(int color_param)
{
  if (num_dots_active == MAX_NUM_DOTS)
    return;

  if (dot_creating) {
    dot[num_dots_active].position = dot[num_dots_active-1].position + (dot[num_dots_active].direction ? DOT_WIDTH : -DOT_WIDTH);
    dot[num_dots_active].color = dot[num_dots_active-1].color;
    dot[num_dots_active].direction = dot[num_dots_active-1].direction;
    dot[num_dots_active].lifetime = dot[num_dots_active-1].lifetime+3;
  } else {
    dot[num_dots_active].position = random(NUM_LEDS);
    //dot[num_dots_active].color = uint8_t((random(color_range+1) + color_off) % 256);// | (random(SAT_MIN,SAT_MAX+1) << 8) | (random(BRT_MIN,BRT_MAX+1) << 16);
    dot[num_dots_active].color = (random(color_range+1) + color_off);// | (random(SAT_MIN,SAT_MAX+1) << 8) | (random(BRT_MIN,BRT_MAX+1) << 16);
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
