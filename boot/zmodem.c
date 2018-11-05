#include "zmodem.h"
#include "defines.h"
#include "serial.h"
#include "lib.h"
#include "mot.h"


/* 制御コード */
#define ZDLE 0x18
#define ZPAD '*'

/* ヘッダの種類 */
#define ZBIN     'A'
#define ZHEX     'B'
#define ZBIN32   'C'
#define ZBINR32  'D'
#define ZVBIN    'a'
#define ZVHEX    'b'
#define ZVBIN32  'c'
#define ZVBINR32 'd'

/* フレーム・タイプ */
#define ZRQINIT   0
#define ZRINIT    1
#define ZSINIT    2
#define ZACK      3
#define ZFILE     4
#define ZSKIP     5
#define ZNAK      6
#define ZABORT    7
#define ZFIN      8
#define ZRPOS     9
#define ZDATA    10
#define ZEOF     11
#define ZFERR    12
#define ZCRC     13
#define ZCHALLENGE 14
#define ZCOMPL   15
#define ZCAN     16
#define ZFREECNT 17
#define ZCOMMAND 18

/* ヘッダ補足情報 */
#define ZF0 3
#define ZF1 2
#define ZF2 1
#define ZF3 0
#define ZP0 0
#define ZP1 1
#define ZP2 2
#define ZP3 3

/* ターミネータ */
#define ZCRCE 'h'
#define ZCRCG 'i'
#define ZCRCQ 'j'
#define ZCRCW 'k'

/* ZRINITのオプション */
#define CANFDX   (1<<0)
#define CANOVIO  (1<<1)
#define CANBRK   (1<<2)
#define CANCRY   (1<<3)
#define CANLZW   (1<<4)
#define CANFC32  (1<<5)
#define ESCCTL   (1<<6)
#define ESC8     (1<<7)

typedef struct _zframe {
  uint8 hdrtype;
  uint8 frmtype;
  uint8 info[4];
  uint8 crc[4];
} zframe;

uint16 updcrc16(uint8 data, uint16 crc)
{
  int i;

  /*CRC ITU-T   */
  for(i=0; i<8; i++) {
    if((crc & 0x8000) != 0) {
      crc = crc << 1;
      crc = crc ^ 0x1021;
    }
    else
      crc = crc << 1;
      
    if((data & 0x80) != 0) {
      data = data << 1;
      crc = crc ^ 0x0001;
    }
    else
      data = data << 1;
  }
  return crc;
}

static int zmodem_recv_frame(zframe *zf)
{
  uint8 c,h,l;

 again:
  switch (c = serial_recv_byte(SERIAL_DEFAULT_DEVICE)) {
  case ZPAD:
    break;
  default:
    goto again;
  }

 again2:
  switch (c = serial_recv_byte(SERIAL_DEFAULT_DEVICE)) {
  case ZPAD:
    goto again2;
  default:
    goto again;
  case ZDLE:
    break;
  }

  zf->hdrtype = serial_recv_byte(SERIAL_DEFAULT_DEVICE);
  switch(zf->hdrtype) {
    case ZBIN:
      zf->frmtype = serial_recv_byte(SERIAL_DEFAULT_DEVICE);
      zf->info[0] = serial_recv_byte(SERIAL_DEFAULT_DEVICE);
      zf->info[1] = serial_recv_byte(SERIAL_DEFAULT_DEVICE);
      zf->info[2] = serial_recv_byte(SERIAL_DEFAULT_DEVICE);
      zf->info[3] = serial_recv_byte(SERIAL_DEFAULT_DEVICE);
      zf->crc[0] = serial_recv_byte(SERIAL_DEFAULT_DEVICE);
      zf->crc[1] = serial_recv_byte(SERIAL_DEFAULT_DEVICE);
      break;
    case ZHEX:
      h = serial_recv_byte(SERIAL_DEFAULT_DEVICE); l = serial_recv_byte(SERIAL_DEFAULT_DEVICE);
      zf->frmtype = get_hex_value(h,l);
      h = serial_recv_byte(SERIAL_DEFAULT_DEVICE); l = serial_recv_byte(SERIAL_DEFAULT_DEVICE);
      zf->info[0] = get_hex_value(h,l);
      h = serial_recv_byte(SERIAL_DEFAULT_DEVICE); l = serial_recv_byte(SERIAL_DEFAULT_DEVICE);
      zf->info[1] = get_hex_value(h,l);
      h = serial_recv_byte(SERIAL_DEFAULT_DEVICE); l = serial_recv_byte(SERIAL_DEFAULT_DEVICE);
      zf->info[2] = get_hex_value(h,l);
      h = serial_recv_byte(SERIAL_DEFAULT_DEVICE); l = serial_recv_byte(SERIAL_DEFAULT_DEVICE);
      zf->info[3] = get_hex_value(h,l);
      h = serial_recv_byte(SERIAL_DEFAULT_DEVICE); l = serial_recv_byte(SERIAL_DEFAULT_DEVICE);
      zf->crc[0] = get_hex_value(h,l);
      h = serial_recv_byte(SERIAL_DEFAULT_DEVICE); l = serial_recv_byte(SERIAL_DEFAULT_DEVICE);
      zf->crc[1] = get_hex_value(h,l);
      serial_recv_byte(SERIAL_DEFAULT_DEVICE); /* CR */
      serial_recv_byte(SERIAL_DEFAULT_DEVICE); /* LF */
      if(zf->frmtype != ZFIN && zf->frmtype != ZACK)
	serial_recv_byte(SERIAL_DEFAULT_DEVICE); /* XON */
      break;
    case ZBIN32:
      zf->frmtype = serial_recv_byte(SERIAL_DEFAULT_DEVICE);
      zf->info[0] = serial_recv_byte(SERIAL_DEFAULT_DEVICE);
      zf->info[1] = serial_recv_byte(SERIAL_DEFAULT_DEVICE);
      zf->info[2] = serial_recv_byte(SERIAL_DEFAULT_DEVICE);
      zf->info[3] = serial_recv_byte(SERIAL_DEFAULT_DEVICE);
      zf->crc[0] = serial_recv_byte(SERIAL_DEFAULT_DEVICE);
      zf->crc[1] = serial_recv_byte(SERIAL_DEFAULT_DEVICE);
      zf->crc[2] = serial_recv_byte(SERIAL_DEFAULT_DEVICE);
      zf->crc[3] = serial_recv_byte(SERIAL_DEFAULT_DEVICE);
      break;
  }
  return 0;

}


void zmodem_send_frame(uint8 type, uint8 *info)
{
  uint16 crc;

  crc = updcrc16(type,0);
  crc = updcrc16(info[0],crc);
  crc = updcrc16(info[1],crc);
  crc = updcrc16(info[2],crc);
  crc = updcrc16(info[3],crc);
  crc = updcrc16(0,crc);
  crc = updcrc16(0,crc);

  serial_send_byte(SERIAL_DEFAULT_DEVICE, ZPAD);
  serial_send_byte(SERIAL_DEFAULT_DEVICE, ZDLE);
  serial_send_byte(SERIAL_DEFAULT_DEVICE, ZBIN);
  serial_send_byte(SERIAL_DEFAULT_DEVICE, type);
  serial_send_byte(SERIAL_DEFAULT_DEVICE, info[0]);
  serial_send_byte(SERIAL_DEFAULT_DEVICE, info[1]);
  serial_send_byte(SERIAL_DEFAULT_DEVICE, info[2]);
  serial_send_byte(SERIAL_DEFAULT_DEVICE, info[3]);
  serial_send_byte(SERIAL_DEFAULT_DEVICE, (uint8)(crc >> 8));
  serial_send_byte(SERIAL_DEFAULT_DEVICE, (uint8)(crc & 0x00ff));
}


/*
void put_hex(uint8 data)
{

  char *digits = "0123456789abcdef";
  serial_send_byte(SERIAL_DEFAULT_DEVICE, digits[data >> 4]);
  serial_send_byte(SERIAL_DEFAULT_DEVICE, digits[data & 0x0f]);

}

void zmodem_send_hex(uint8 type, uint8 *info)
{
  uint16 crc;

  crc = updcrc16(type,0);
  crc = updcrc16(info[0],crc);
  crc = updcrc16(info[1],crc);
  crc = updcrc16(info[2],crc);
  crc = updcrc16(info[3],crc);
  crc = updcrc16(0,crc);
  crc = updcrc16(0,crc);

  serial_send_byte(SERIAL_DEFAULT_DEVICE, ZPAD);
  serial_send_byte(SERIAL_DEFAULT_DEVICE, ZPAD);
  serial_send_byte(SERIAL_DEFAULT_DEVICE, ZDLE);
  serial_send_byte(SERIAL_DEFAULT_DEVICE, ZHEX);
  put_hex(type);
  put_hex(info[0]);
  put_hex(info[1]);
  put_hex(info[2]);
  put_hex(info[3]);
  put_hex((uint8)(crc >> 8));
  put_hex((uint8)(crc & 0x00ff));
  serial_send_byte(SERIAL_DEFAULT_DEVICE, 0x0d);
  serial_send_byte(SERIAL_DEFAULT_DEVICE, 0x8a);
  //serial_send_byte(SERIAL_DEFAULT_DEVICE, 0x11);
}
*/


int check_crc(type)
{
  int i;
  uint8 c;

  if (type == ZBIN32) i = 4;
  else i = 2;
  for( ; i > 0;i--) {
    c = serial_recv_byte(SERIAL_DEFAULT_DEVICE);
    if (c == ZDLE) {
      c = serial_recv_byte(SERIAL_DEFAULT_DEVICE);
      c ^= 0x40;
    }
  }
  return 0;
}


uint8 * zmodem_recv(uint8 *buf)
{
  zframe rzf;
  uint8 info[4];
  uint8 *entry=NULL;

  while(1) {
 
    zmodem_recv_frame(&rzf);

    switch (rzf.frmtype) {
      uint8 c;
    case ZRQINIT:
      info[0]=info[1]=info[2]=0;
      info[3]=CANFDX | CANOVIO;
      serial_send_byte(SERIAL_DEFAULT_DEVICE, ZPAD);
      zmodem_send_frame(ZRINIT,info);
      break;
    case ZFILE:
      do {
	c = serial_recv_byte(SERIAL_DEFAULT_DEVICE);
      } while (c != ZDLE);
      serial_recv_byte(SERIAL_DEFAULT_DEVICE);
      check_crc(rzf.hdrtype);
      /* reply */
      info[0]=info[1]=info[2]=info[3]=0;
      zmodem_send_frame(ZRPOS,info);
      break;
    case ZDATA:
      while (1) {
	c = serial_recv_byte(SERIAL_DEFAULT_DEVICE);
	if (c == ZDLE) {
	  c = serial_recv_byte(SERIAL_DEFAULT_DEVICE);
	  if ('h' <= c && c <= 'k') {
	    check_crc(rzf.hdrtype);
	    if (c == ZCRCE || c == ZCRCW)
	      break;
	    continue;
	  } else {
	    c ^= 0x40;
	  }
	}
	if (buf == NULL)
	  entry=mot(c);
	else
	  *buf++=c;
      }
      break;
    case ZEOF:
      info[0]=info[1]=info[2]=0;
      info[3]=CANFDX | CANOVIO;
      zmodem_send_frame(ZRINIT,info);
      break;
    case ZFIN:
      info[0]=info[1]=info[2]=info[3]=0;
      zmodem_send_frame(ZFIN,info);
      return entry;
    default :
      return NULL;
    }
  }

}
