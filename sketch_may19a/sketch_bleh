#include <arduinoFFT.h>
#include <FastLED.h>
#include <math.h>       /* log10 */

#define LED_PIN 2
#define NUM_LEDS 150
#define SIGNAL_PIN 0

arduinoFFT FFT = arduinoFFT();

/* configuration */
const uint16_t FRAME_SIZE = 16;      // 1 by te, milliseconds
const byte MAX_DOTS = 12;        // 8 bytes * 10 dots = 80 bytes
const unsigned int READ_PERIOD = 100;  //1000 us; 1 kHz
const double READ_FREQ = 10000;  // want 20000 here and 50 above as read period
const float AVG_HIST = 0.99;  // weighting of average to new data
const float TRIGGER_THRESHOLD = 1.6;
const byte WALK_RESISTANCE = 2;
const byte WALK_SPEED = 1;

int color_range = 50;
int color_off = 0;

CRGB leds[NUM_LEDS];            // 3 bytes * 150 LEDs = 450 bytes
double fft_real[FRAME_SIZE];  // 2 bytes * 50 = 100 bytes
double fft_imag[FRAME_SIZE];  // 2 bytes * 50 = 100 bytes
byte n_dots = 0;

class Dot {
  // Represent one single dot (8 bytes)
  public:
    byte line_length;
    byte position;
    byte lifetime;
    byte color;
    int direction;
    int stutter;
    byte stutter_counter;
    
    Dot() {
      line_length = 5;
      stutter = 1;
      stutter_counter = 0;
    }

    void reset(
        byte a, int b, int c, byte d
      ) {
        position = a;
        color = b;
        direction = c;
        lifetime = d;
        
        if (lifetime > 0) {
          n_dots += 1;
        }
    }

    void draw(CRGB * leds) {
      // Draw dot on background
      for (int tail = 0; tail < line_length; tail++) {
//        byte pos = position - direction * tail;
        leds[position - direction * tail] = CHSV(color, 255 - (40 * tail), 255 - (40 * tail));  // TODO: fade the tail brightness / mix with background
      }
    }

    bool is_alive() {
      return lifetime > 0;
    }

    void step(byte n) {
      if (!is_alive()) {
        return;
      }
      
      if (!stutter || stutter_counter == 0) {
//        Serial.println(direction);
        position += direction * n;
        lifetime -= 1;

        if ((position < 0) || (NUM_LEDS <= position)) {
          lifetime = 0;
        }

        if (lifetime <= 0) {
          n_dots -= 1;
        }
      }
      stutter_counter = (stutter_counter + 1) % stutter;
    }
};

Dot * dots[MAX_DOTS];

Dot * last_low = NULL;
Dot * last_mid = NULL;
Dot * last_high = NULL;

double low_average = 0;
double mid_average = 0;
double high_average = 0;

Dot * create_dot() {
  for (byte i; i < MAX_DOTS; i++) {
    if (n_dots >= MAX_DOTS) {
      return NULL;
    }
    
    if (!(dots[i] -> is_alive())) {
//      Serial.println("dot is not alive");
      dots[i] -> reset(
        random(NUM_LEDS),   // position
        (random(color_range+1) + color_off) % 256, // pick color
        2 * random(2) - 1,  // direction
        20 // lifetime
      );  // TODO parameters
      return dots[i];
    }
  }
}

void extend_dot(Dot * prev_dot) {
  if (prev_dot -> line_length < 6) {
    prev_dot -> line_length++;
  } else {
    return;
  }
//  prev_dot -> lifetime += 0;
}

void analyze_music() {
  // Decompose data
  FFT.Windowing(fft_real, FRAME_SIZE, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.Compute(fft_real, fft_imag, FRAME_SIZE, FFT_FORWARD);
  FFT.ComplexToMagnitude(fft_real, fft_imag, FRAME_SIZE);
  
  double low_total = 0;
  double mid_total = 0;
  double high_total = 0;
  
  // Group recording into frequency ranges
  for (uint16_t i = 0; i < FRAME_SIZE; i++) {
    double freq = ((abs((int)((FRAME_SIZE / 2) - i)) * 1.0 * READ_FREQ) / (FRAME_SIZE));
//    Serial.println(log10(fft_real[i]));
//    leds[i + 100] = CHSV(255 - pow(log10(fft_real[i]) , 4) * 5, 255, pow(log10(fft_real[i]) , 4) * 5);
//    Serial.println(abs((int)((FRAME_SIZE / 2) - i)));
    if (freq < 100) {
      
    } else if (freq < 1000) {
      // bass
      low_total += fft_real[i];
    } else if ((1000 <= freq) && (freq < 3000)) {
      mid_total += fft_real[i];
      // mids
    } else {
      high_total += fft_real[i];
      // highs
    }
  }

  // Update averages
//  Serial.println("here");
//  Serial.println(low_total);
//  Serial.println(low_average);
  if (low_total > low_average * TRIGGER_THRESHOLD) {
    if (!last_low) {
      last_low = create_dot();
    } else {
      extend_dot(last_low);
    }
  } else {
    last_low = NULL;
  }

  if (mid_total > mid_average * TRIGGER_THRESHOLD) {
    if (!last_mid) {
      last_mid = create_dot();
    } else {
      extend_dot(last_mid);
    }
  } else {
    last_low = NULL;
  }

  if (high_total > high_average * TRIGGER_THRESHOLD) {
    if (!last_high) {
      last_high = create_dot();
    } else {
      extend_dot(last_high);
    }
  } else {
    last_low = NULL;
  }

    last_low = NULL;
    last_mid = NULL;
    last_high = NULL;
  low_average = low_average * AVG_HIST + low_total * (1 - AVG_HIST);
  mid_average = mid_average * AVG_HIST + mid_total * (1 - AVG_HIST);
  high_average = high_average * AVG_HIST + high_total * (1 - AVG_HIST);
}

void draw_leds() {
  FastLED.show();
}

void clear_leds() {
  for (byte i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(0, 150, 4);  // TODO: get backgorund color
  }
}

void setup() {
  delay( 1000 ); // power-up safety delay

  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);

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
  draw_leds();

  Serial.begin(9600);
}

static unsigned long last_read;

void listen() {
    for (int i = 0; i < FRAME_SIZE; i++) {
      if (micros() - last_read >= READ_PERIOD) {
        last_read += READ_PERIOD;
        fft_real[i] = (double)analogRead(SIGNAL_PIN);
//        fft_real[i] = random(1024);
        fft_imag[i] = 0;
      }
    }

//    double sum = 0;
//    for(int i = 1; i < FRAME_SIZE; i++){
//       //^^should be 0
//       sum += fft_real[i];
//    }
//    double avg = sum / FRAME_SIZE;  //pay attention to truncation when doing integer division
//
//    Serial.println(avg);
}

void loop() {
  // Do everything
  clear_leds();
  listen();  // Fills recording array with data
  analyze_music(); // Analyzes recording array
  
  for (byte i = 0; i < MAX_DOTS; i++) {
    if (dots[i] -> lifetime > 0) {
//      Serial.println("ghere");
//      Serial.println(dots[i] -> color);
//      Serial.println(dots[i] -> position);

      dots[i] -> draw(leds);
      dots[i] -> step(1);
    }
  }
  
  if (random(WALK_RESISTANCE) == 0)
    color_off += random(-WALK_SPEED, WALK_SPEED);

  draw_leds();
}
