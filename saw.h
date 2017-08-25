class Saw {
 public:
  Lerp lerp;
  uint32_t period = 0;

  void set_period(uint32_t period) {
    this->period = period;
    this->reset();
  }
  
  void advance() {
    this->lerp.advance();
    if(!this->lerp.ongoing()) {
      this->reset();
    }
  }

  void reset() {
    this->lerp.reset(this->period, 0, UINT16_MAX);
  }

  uint16_t value() {
    return this->lerp.value();
  }
};
