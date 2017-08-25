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
