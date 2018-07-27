#ifndef _GPIO_H
#define _GPIO_H

typedef unsigned char Uint8;
typedef unsigned short Uint16;
typedef unsigned int Uint32;

#define HELLO_MAGIC 'k' 
#define CMD_SET_GPIO3_0          _IO(HELLO_MAGIC,0x30)
#define CMD_SET_GPIO3_1          _IO(HELLO_MAGIC,0x31)
#define CMD_SET_GPIO3_2          _IO(HELLO_MAGIC,0x32)
#define CMD_SET_GPIO3_3          _IO(HELLO_MAGIC,0x33)
#define CMD_GET_VALUE           _IO(HELLO_MAGIC,0x1)
#define CMD_SET_VALUE           _IO(HELLO_MAGIC,0x2)

typedef struct tagGpio{
    Uint32 di_value;   
    Uint32 do_value;   
    int do_pin;     
    int output_val; 
    int do_period;  
}GpioS;



#endif
