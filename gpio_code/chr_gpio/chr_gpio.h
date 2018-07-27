
#ifndef CHR_GPIO_H
#define CHR_GPIO_H

typedef unsigned char Uint8;
typedef unsigned short Uint16;
typedef unsigned int Uint32;
#define 	HIGH 1
#define 	LOW 0

#define GPIO4_BASE_ADDR			0x20190000	// GPIO4 base addr
#define GPIO3_BASE_ADDR			0x20180000	// GPIO3 base addr

#define MULT_GPIO_BASE_ADDR     0X200f0000  //fuyong  gpio   

#define REG_READ(reg)         *((volatile unsigned int *)(reg))
#define REG_WRITE(reg,val)    *((volatile unsigned int *)(reg)) = val  

#define HELLO_MAGIC 'k' 
#define CMD_SET_GPIO3_0          _IO(HELLO_MAGIC,0x30)
#define CMD_SET_GPIO3_1          _IO(HELLO_MAGIC,0x31)
#define CMD_SET_GPIO3_2          _IO(HELLO_MAGIC,0x32)
#define CMD_SET_GPIO3_3          _IO(HELLO_MAGIC,0x33)
#define CMD_GET_VALUE           _IO(HELLO_MAGIC,0x1)
#define CMD_SET_VALUE           _IO(HELLO_MAGIC,0x2)

#endif
