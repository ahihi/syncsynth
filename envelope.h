enum ARState { idle, attack, release };

class AREnvelope {
 public:
  ARState state = idle;
  int32_t peak;
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

  void set_peak(int32_t peak) {
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
	this->lerp.reset(this->release_samples, this->lerp.value(), 0);
	break;

      case release:
        //Serial.println("-> idle");
	this->state = idle;
	this->lerp.reset(0, 0, 0);
	break;
      }
    }
  }

  int32_t value() {
    return this->lerp.value();
  }
};
