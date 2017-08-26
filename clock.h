class Clock {
 public:
  uint32_t tick_period = 0;
  uint32_t tick_sustain = 100;
  uint32_t t = 0;

  void set_tempo(uint32_t millibpm) {
    uint32_t mins_per_millib = 1 / millibpm;
    uint32_t secs_per_millib = mins_per_millib * 60;
    uint32_t samples_per_millib = secs_per_millib * SAMPLE_RATE;
    uint32_t samples_per_b = samples_per_millib * 1000;
    uint32_t samples_per_eighth = samples_per_b / 2;

    this->tick_period = (60 * SAMPLE_RATE) / (millibpm * 2) * 1000;
  }

  void advance() {
    this->t++;
    if(this->tick_period < this->t) {
      this->t = 0;
    }
  }
    
  int32_t value() {
    return this->t < this->tick_sustain ? 50 : 0;
  }
};
