#include "mot.h"
#include "lib.h"
#include "defines.h"

uint8 *mot(uint8 c)
{
  static enum {TYPE, SIZE, ADDRESS, DATA, SAM, END, S0} state = TYPE;
  static uint8 buf[2];
  static uint8 buf_count = 0;
  static uint8 *address;
  static uint8 data_size, add_size;
  uint8 value=0;

  buf[buf_count++] = c;

  if (buf_count == 1)
    return 0;

  buf_count = 0;


  value=get_hex_value(buf[0],buf[1]);

  switch (state) {
  case TYPE:
    address=0;
    switch (buf[1]) {
    case '0' :state = S0;add_size=2;break;
    case '1' :
    case '2' :
    case '3' :state = SIZE;add_size=buf[1]-'0'+1;break;
    case '7' :
    case '8' :
    case '9' :state = SIZE;add_size=10-(buf[1]-'0')+1;break;
    default : ;
    }
    break;
  case SIZE:
    data_size = value-add_size-1;
    state = ADDRESS;
    break;
  case ADDRESS:
    if (add_size > 0) {
      address = (uint8 *)((uint32)address | value);
      add_size--;
      if (add_size == 0) {state = DATA;}
      else               {address = (uint8 *)((uint32)address << 8);}
    }
    break;
  case DATA:
    if (data_size > 0) {
      *address++ = value;
      data_size--;
      if (data_size == 0) {state = SAM;}
    } else {state = SAM;}
    break;
  case SAM:
    buf_count=1;
    state = END;
    break;
  case END:
  if (buf[1]=='S') {
    buf[0]='S';
    state=TYPE;
  }
  buf_count=1;
  break;
  case S0:
    if (add_size == 2)  {data_size=value-add_size-1;}
    if (add_size > 0)  {add_size--;break;}
    if (data_size > 0) {data_size--;break;}
    state = SAM;
    break;
  default: ;
  }

  return address;
}
