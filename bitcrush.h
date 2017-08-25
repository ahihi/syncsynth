class Bitcrush {
 public:
  uint16_t decimate = 1;
  uint16_t quantize = 0;
  uint16_t sample = INT16_MAX;
  uint16_t decimate_counter = 0;
  
  void set_decimate(uint16_t decimate) {
    this->decimate = decimate;
  }

  void set_quantize(uint16_t quantize) {
    this->quantize = quantize;
  }
  
  void advance(uint16_t sample) {
    if(this->decimate_counter == 0) {
      this->sample = sample >> this->quantize << this->quantize;
    }

    this->decimate_counter++;
    if(this->decimate_counter >= this->decimate) {
      this->decimate_counter = 0;
    }
  }

  uint16_t value() {
    return this->sample;
  }
};
