#include "defines.h"
#include "interrupt.h"
#include "serial.h"
#include "lib.h"
#include "zmodem.h"

#define H8_3069F_P1DDR  ((volatile uint8 *)0xfee000)
#define H8_3069F_P2DDR  ((volatile uint8 *)0xfee001)
#define H8_3069F_P8DDR  ((volatile uint8 *)0xfee007)

#define H8_3069F_ISCR   ((volatile uint8 *)0xfee014)
#define H8_3069F_IER    ((volatile uint8 *)0xfee015)
#define H8_3069F_IPRA   ((volatile uint8 *)0xfee018)

#define H8_3069F_ABWCR  ((volatile uint8 *)0xfee020)
#define H8_3069F_ASTCR  ((volatile uint8 *)0xfee021)
#define H8_3069F_WCRH   ((volatile uint8 *)0xfee022)
#define H8_3069F_WCRL   ((volatile uint8 *)0xfee023)

#define H8_3069F_RTCOR  ((volatile uint8 *)0xfee02a)
#define H8_3069F_RTMCSR ((volatile uint8 *)0xfee028)
#define H8_3069F_DRCRA  ((volatile uint8 *)0xfee026)
#define H8_3069F_DRCRB  ((volatile uint8 *)0xfee027)

static int init(void)
{
  /* 以下はリンカ・スクリプトで定義してあるシンボル */
  extern int erodata, data_start, edata, bss_start, ebss;

  /*
   * データ領域とBSS領域を初期化する．この処理以降でないと，
   * グローバル変数が初期化されていないので注意．
   */
  memcpy(&data_start, &erodata, (long)&edata - (long)&data_start);
  memset(&bss_start, 0, (long)&ebss - (long)&bss_start);

  /* バスコントローラの設定 */
  *H8_3069F_ABWCR  = 0xff; /* 8ビットバスモード(全エリア8ビットアクセス空間) */
  *H8_3069F_WCRL = 0xcf; //CS2にプログラムウエイトを挿入しない
  *H8_3069F_ASTCR = 0xfb; //CS2を2ステートアクセスにする(最小値)

  /* DRAMの初期化 */
  *H8_3069F_DRCRA  = 0x30;
  *H8_3069F_DRCRB  = 0x98;
  *H8_3069F_RTCOR  = 0x03; //7カウントでリフレッシュ
  *H8_3069F_RTMCSR = 0x37;

  /* 割り込みコントローラの設定 */
  *H8_3069F_ISCR = 0x00; /* lowレベルで割り込み */
  *H8_3069F_IPRA = 0x00;
  *H8_3069F_IER  = 0x20; /* IRQ5割り込み有効化 */

  /* アドレスバス、チップセレクト信号の設定 */
  *H8_3069F_P1DDR  = 0xff; /* アドレスバス有効化 */
  *H8_3069F_P2DDR  = 0xff; /* アドレスバス有効化 */
  *H8_3069F_P8DDR  = 0xec; /* CS1,CS2有効化 */


  /* シリアルの初期化 */
  serial_init(SERIAL_DEFAULT_DEVICE);

  /* ソフトウエア・割り込みベクタを初期化する */
  softvec_init();

  return 0;
}

int main(void)
{
  extern uint8 buffer_start; /* リンカ・スクリプトで定義されているバッファ */
  extern uint8 dram_start; /* リンカ・スクリプトで定義されているバッファ */
  static char input[16];
  int i,j,ret;
  uint8 c[2],*p,*p2;
  uint32 entry = 0;
  void (*os)(void);

  uint8 *x = (uint8 *)0xffc40e;

  init();

  while(1)
    {
      gets(input);
      if(!strcmp(input, "load")) {
	  puts("loading start\n");
	  entry=zmodem_recv(&buffer_start);
      }
      else if(!strcmp(input, "run")) {
	  puts("loading start");
	  entry=zmodem_recv(NULL);
	  os = (void (*)(void))entry;
      }
      else if(!strcmp(input, "put")) {
	putxval((uint32)entry,0);
	putxval((uint32)ret,0);
	
	for(i = 0; i < 20; i++)
	  putxval(*x++,0);
      }
      else if(!strcmp(input, "puts")) {
	gets(input);
	x=NULL;
	for(i=0; i < 6; i+=2) {
	  x = (uint8 *)((uint32)x | get_hex_value(input[i], input[i+1]));
	  if (i != 4)
	    x = (uint8 *)((uint32)x << 8);
	}
	
	
	for(i = 0; i < 20; i++)
	  putxval(*x++,2);
      }
      else if(!strcmp(input, "start")) {
	os();
      }
      else if (!strcmp(input, "check")) {
	p = &buffer_start;
	p2 = &dram_start;
	for(i = 0; i < 200; i++) {
	  *p++ = serial_recv_byte(SERIAL_DEFAULT_DEVICE);
	  serial_send_byte(SERIAL_DEFAULT_DEVICE, *p2++);
	  }
      }
      else if (!strcmp(input, "check2")) {
	p = (uint8 *)&buffer_start;
	p2 = (uint8 *)&dram_start;
	for(i = 0; i < 200; i++) {
	  putxval(i,0); putc(' ');
	  putxval((uint32)p,0); putc(':'); putxval(*p++,0); putc(' ');
	  putxval((uint32)p2,0);putc(':'); putxval(*p2++,0); putc('\n');
	}
      }
      else if (!strcmp(input, "check3")) {
	p = &dram_start;
	for(i = 0; i < 200; i++) {
	  for(j=0; j<2; j++) {
	    c[j]=serial_recv_byte(SERIAL_DEFAULT_DEVICE);
	  }
	  if(c[0] >= 'a')
	    *p = c[0] - 'a' + 10;
	  else
	    *p = c[0] - '0';
	  *p <<= 4;
	  if(c[1] >= 'a')
	    *p |= c[1] - 'a' + 10;
	  else
	    *p |= c[1] - '0';
	  p++;
	}
      }
     
    }

  return 0;
}
