#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <i2s.h>
#include <i2s_reg.h>

#define LED     D0        // Led in NodeMCU at pin GPIO16 (D0).
#define SAMPLE_RATE 44100

//Pulse Density Modulated 16-bit I2S DAC
uint32_t i2sACC;
uint16_t DAC=0x8000;
uint16_t err;

void writeDAC(uint16_t DAC) {
  for (uint8_t i=0;i<32;i++) {
    i2sACC=i2sACC<<1;
    if(DAC >= err) {
      i2sACC|=1;
      err += 0xFFFF-DAC;
    } else {
      err -= DAC;
    }
  }
  bool flag=i2s_write_sample(i2sACC);
}

#define LFSR_INIT  0xfeedfaceUL
/* Choose bits 32, 30, 26, 24 from  http://arduino.stackexchange.com/a/6725/6628
 *  or 32, 22, 2, 1 from 
 *  http://www.xilinx.com/support/documentation/application_notes/xapp052.pdf
 *  or bits 32, 16, 3,2  or 0x80010006UL per http://users.ece.cmu.edu/~koopman/lfsr/index.html 
 *  and http://users.ece.cmu.edu/~koopman/lfsr/32.dat.gz
 */  
#define LFSR_MASK  ((uint32_t)( 1UL<<31 | 1UL <<15 | 1UL <<2 | 1UL <<1  ))

class LFSR {
 public:
  uint32_t lfsr = LFSR_INIT;

  uint32_t next_bit() {
    if(lfsr & 1) { lfsr =  (lfsr >>1) ^ LFSR_MASK ; return 1; }
    else         { lfsr >>= 1;                      return 0; }
  }
};

class Lerp {
 public:
  bool negative = false;
  int32_t dy = 0;
  int32_t dt = 0;
  uint32_t y_t_int = 0;
  uint32_t y_t_frac = 0;
  uint32_t inc_int = 0;
  uint32_t inc_frac = 0;
  uint16_t y0 = 0;
  uint32_t t = 0;
  uint16_t y = 0;
  
  void reset(uint32_t dt, uint16_t y0, uint16_t y1) {
    this->negative = y1 < y0;
    this->dy = abs(y1 - y0);
    this->dt = dt;
    this->y_t_int = 0;
    this->y_t_frac = 0;
    this->inc_int = dt > 0 ? dy / dt : 0;
    this->inc_frac = dt > 0 ? dy % dt : 0;
    this->y0 = y0;
    this->t = 0;
    this->y = 0;
  }
  
  bool ongoing() {
    bool yes = this->t < this->dt;
    //Serial.print("ongoing: "); Serial.println(yes);
    return yes;
  }
  
  void advance() {
    if(this->ongoing()) {
      this->y = this->y0;
      if(negative) {
	this->y -= this->y_t_int;
      } else {
	this->y += this->y_t_int;
      }

      this->y_t_int += this->inc_int;
      this->y_t_frac += this->inc_frac;
      if(this->y_t_frac >= this->dt) {
	this->y_t_int++;
	this->y_t_frac -= dt;
      }
      
      this->t++;
    }
  }

  uint16_t value() {
    uint16_t v = this->y;
    //Serial.print("value: "); Serial.println(v);
    return v;
  }
};

enum ARState { idle, attack, release };

class AREnvelope {
 public:
  ARState state = idle;
  uint16_t peak;
  uint32_t attack_msecs;
  uint32_t attack_samples;
  uint32_t release_msecs;
  uint32_t release_samples;
  Lerp lerp;

  void set_attack(uint32_t attack_msecs) {
    this->attack_msecs = attack_msecs;
    this->attack_samples = SAMPLE_RATE * attack_msecs / 1000;
  }

  void set_release(uint32_t release_msecs) {
    this->release_msecs = release_msecs;
    this->release_samples = SAMPLE_RATE * release_msecs / 1000;
  }

  void set_peak(uint32_t peak) {
    this->peak = peak;
  }
  
  void trigger() {
    //Serial.println("-> attack");
    this->state = attack;
    this->lerp.reset(this->attack_samples, 0, this->peak);
  }

  void advance() {
    this->lerp.advance();
    
    if(!this->lerp.ongoing()) {
      switch(this->state) {
      case attack:
        //Serial.println("-> release");
	this->state = release;
	this->lerp.reset(this->release_samples, this->peak, 0);
	break;

      case release:
        //Serial.println("-> idle");
	this->state = idle;
	this->lerp.reset(0, 0, 0);
	break;
      }
    }
  }

  uint16_t value() {
    return this->lerp.value();
  }
};

struct Step {
  uint8_t note;
  uint8_t velocity;
  uint32_t attack_msecs;
  uint32_t release_msecs;
};


const int16_t high_threshold = 20;
bool high = false;
uint8_t seq_step = 0;
LFSR lfsr;
AREnvelope lfsr_env;


uint8 hh_vel1 = 160;
uint8 hh_vel2 = 90;
uint32_t hh_atk = 30;
uint32_t hh_rel = 30;

uint8 sn_vel = 255;
uint32_t sn_atk = 0;
uint32_t sn_rel = 200;
const Step seq[8] = {
  {1, hh_vel1, hh_atk, hh_rel}, {1, hh_vel2, hh_atk, hh_rel},
  {1, sn_vel, sn_atk, sn_rel}, {1, hh_vel2, hh_atk, hh_rel},
  {1, hh_vel1, hh_atk, hh_rel}, {1, hh_vel2, hh_atk, hh_rel},
  {1, sn_vel, sn_atk, sn_rel}, {0, 0, 0, 0}
};

uint8_t read_interval = 32;
uint8_t read_counter;

void setup(void) {
  // ESP8266 Low power
  WiFi.forceSleepBegin(); // turn off ESP8266 RF
  delay(1); // give RF section time to shutdown
  i2s_begin();
  i2s_set_rate(SAMPLE_RATE);
  pinMode(2, INPUT);
  pinMode(15, INPUT);

  pinMode(A0, INPUT);
  pinMode(LED, OUTPUT);

  //Serial.begin(115200);
}

void loop() {
  if(read_counter == 0) {
    int sync = analogRead(A0);
    if (!high && sync >= high_threshold) {
      high = true;
      digitalWrite(LED, LOW);
  
      seq_step = seq_step + 1;
      if(seq_step >= 8) {
        seq_step = 0;
      }
  
      Step s = seq[seq_step];
  
      /*Serial.print("seq_step: "); Serial.print(seq_step);
      Serial.print(", note: "); Serial.print(note);
      Serial.println("");*/

      uint16_t peak = ((uint16_t) s.velocity) << 8;
      
      switch(s.note) {
      case 1:
	lfsr_env.set_attack(s.attack_msecs);
	lfsr_env.set_release(s.release_msecs);
	lfsr_env.set_peak(peak);
	lfsr_env.trigger();
	break;	
      }      
    } else if (high && sync < high_threshold) {
      high = false;
      digitalWrite(LED, HIGH);
    }
  }

  read_counter = (read_counter + 1) % read_interval;

  uint16_t amp = lfsr_env.value();
  lfsr_env.advance();
  
  int16_t osc = 0;
  for(uint8_t i = 0; i < 16; i++) {
    osc = (osc << 1) | lfsr.next_bit();
  }

  /*
    
    (int32_t) amp - [0, 2^16 - 1]
    osc - [-2^15, 2^15 - 1]
    ((int32_t) amp) * osc - [-2^15 * (2^16 - 1), (2^15 - 1) * (2^16 - 1)]
    / UINT16_MAX - [-2^15, 2^15 - 1]
    - INT16_MIN - [0, 2^16 - 1]

   */
  int32_t sample32 = ((int32_t) amp) * osc / UINT16_MAX - INT16_MIN;
  
  uint16_t sample = (uint16_t) sample32;
  //uint16_t sample = osc;

  /*Serial.print("amp: "); Serial.print(amp);
  Serial.print(", osc: "); Serial.print(osc);
  Serial.print(", sample: "); Serial.print(sample);
  Serial.println("");*/
  
  writeDAC(sample);
}
