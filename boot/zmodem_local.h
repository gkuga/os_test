#ifndef _ZMODEM_LOCAL_H_INCLUDED_
#define _ZMODEM_LOCAL_H_INCLUDED_

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

/* ターミネータ */
#define ZCRCE 'h'
#define ZCRCG 'i'
#define ZCRCQ 'j'
#define ZCRCW 'k'

/* ZRINITのオプション */
#define CANFDX   (1<<0)
#define CANOVUI  (1<<1)
#define CANBRK   (1<<2)
#define CANCRY   (1<<3)
#define CANLZW   (1<<4)
#define CANFC32  (1<<5)
#define ESCCTL   (1<<6)
#define ESC8     (1<<7)

#endif
