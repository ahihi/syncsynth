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
