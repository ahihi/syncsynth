#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <i2s.h>
#include <i2s_reg.h>

#define LED     D0        // Led in NodeMCU at pin GPIO16 (D0).
#define SAMPLE_RATE 44100

#include "lerp.h"
#include "lfsr.h"
#include "saw.h"
#include "envelope.h"
#include "bitcrush.h"

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

struct Step {
  uint8_t note;
  uint8_t velocity;
  uint32_t attack_msecs;
  uint32_t release_msecs;
  uint16_t crush_decimate;
};

const int16_t high_threshold = 20;
bool high = false;
uint8_t seq_step = 0;
Saw saw;
LFSR lfsr;
AREnvelope lfsr_env;
Bitcrush lfsr_crush;

uint8 hh_vel1 = 160;
uint8 hh_vel2 = 90;
uint32_t hh_atk = 30;
uint32_t hh_rel1 = 50;
uint32_t hh_rel2 = 30;

uint8 sn_vel = 255;
uint32_t sn_atk = 0;
uint32_t sn_rel = 175;
uint16_t sn_decimate1 = 4;
uint16_t sn_decimate2 = 8;
const Step seq[8] = {
  {1, hh_vel1, hh_atk, hh_rel1, 1}, {1, hh_vel2, hh_atk, hh_rel2, 1},
  {1, sn_vel, sn_atk, sn_rel, sn_decimate1}, {1, hh_vel2, hh_atk, hh_rel2, 1},
  {1, hh_vel1, hh_atk, hh_rel1, 1}, {1, hh_vel2, hh_atk, hh_rel2, 1},
  {1, sn_vel, sn_atk, sn_rel, sn_decimate2}, {0, 0, 0, 0, 1}
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

  saw.set_period(200);
  
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
      lfsr_crush.set_decimate(s.crush_decimate);
      
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


  int16_t lfsr_osc = 0;
  for(uint8_t i = 0; i < 16; i++) {
    lfsr_osc = (lfsr_osc << 1) | lfsr.next_bit();
  }


  int16_t saw_osc = static_cast<int16_t>(static_cast<uint32_t>(saw.value()) - INT16_MAX);
  saw.advance();

  /*
    
    (int32_t) amp - [0, 2^16 - 1]
    osc - [-2^15, 2^15 - 1]
    ((int32_t) amp) * osc - [-2^15 * (2^16 - 1), (2^15 - 1) * (2^16 - 1)]
    / UINT16_MAX - [-2^15, 2^15 - 1]
    - INT16_MIN - [0, 2^16 - 1]

  */

  uint16_t amp = lfsr_env.value();
  lfsr_env.advance();

  int32_t amp32 = static_cast<int32_t>(amp);
  int32_t lfsr_sample32 = amp32 * lfsr_osc / UINT16_MAX - INT16_MIN;
  int32_t saw_sample32 = amp32 * saw_osc / UINT16_MAX - INT16_MIN;
  
  int32_t mix32 = /* saw_sample32 / 2 + */ lfsr_sample32;
  int16_t mix = static_cast<int16_t>(mix32);
  
  lfsr_crush.advance(mix);
  
  uint16_t sample = lfsr_crush.value();
  //uint16_t sample = osc;

  /*Serial.print("amp: "); Serial.print(amp);
  Serial.print(", osc: "); Serial.print(osc);
  Serial.print(", sample: "); Serial.print(sample);
  Serial.println("");*/
  
  writeDAC(sample);
}
