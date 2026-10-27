#include "../firmware/VehicleHealthComputer/obd.h"
#include <assert.h>
#include <math.h>
#include <string.h>
#include <iostream>
int main() {
  float v=0; uint8_t rpm[]={0x1A,0xF8};
  assert(obd::decode(0x0C,rpm,2,v) && v==1726);
  assert(!obd::decode(0x0C,rpm,1,v));
  uint8_t temp[]={0}; assert(obd::decode(5,temp,1,v) && v==-40);
  temp[0]=130; assert(obd::decode(5,temp,1,v) && v==90);
  uint8_t full[]={255,255}; assert(obd::decode(4,full,1,v) && v==100);
  assert(obd::decode(0x0D,full,1,v) && v==255);
  assert(obd::decode(0x10,full,2,v) && fabs(v-655.35f)<0.01f);
  assert(obd::decode(0x42,full,2,v) && fabs(v-65.535f)<0.001f);
  assert(!obd::decode(0xFF,full,2,v));
  assert(obd::supported(0x80000001,0,1)); assert(obd::supported(1,0,32));
  assert(!obd::supported(1,0,0)); assert(!obd::supported(1,0,33));
  assert(obd::supported(0x40000000,0x40,0x42));
  char c[6]; obd::dtc(3,0,c); assert(!strcmp(c,"P0300"));
  obd::dtc(0xC1,0x23,c); assert(!strcmp(c,"U0123"));
  obd::dtc(0x52,0xAB,c); assert(!strcmp(c,"C12AB"));
  obd::IsoTp t;
  uint8_t sf[]={4,0x41,0x0C,0x1A,0xF8,0,0,0};
  assert(t.feed(sf,8)==1 && t.size==4 && t.data[3]==0xF8);
  assert(t.feed(sf,4)==-1); // Truncated single frame
  uint8_t ff[]={0x10,10,0x43,4,1,0x33,3,0};
  uint8_t cf[]={0x21,4,0x20,0xC1,0x23,0,0,0};
  assert(t.feed(ff,8)==2); assert(t.feed(cf,8)==1 && t.size==10 && t.data[9]==0x23);
  assert(t.feed(ff,8)==2); cf[0]=0x22; assert(t.feed(cf,8)==-1 && !t.active);
  ff[1]=129; assert(t.feed(ff,8)==-1); ff[1]=7; assert(t.feed(ff,8)==-1);
  uint8_t zero[]={0,0,0,0,0,0,0,0}; assert(t.feed(zero,8)==-1);
  // 128-byte limit with sequence rollover and padding in final frame.
  ff[1]=128; assert(t.feed(ff,8)==2);
  for(int i=1;t.active;i++){uint8_t frame[8]={uint8_t(0x20|(i&15)),1,2,3,4,5,6,7};int r=t.feed(frame,8);assert(r==3 || r==1);}
  assert(t.used==128);
  std::cout << "All decoder, support-mask, DTC and ISO-TP tests passed\n";
}
