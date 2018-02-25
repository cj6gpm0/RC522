#ifndef RC522_H
#define RC522_H


#include<stdlib.h>
#include<stdio.h>
#include "mgos_hal.h"
#include "mgos_spi.h"
#include "mgos_gpio.h"
#include "mgos.h"

#ifdef __cplusplus
extern "C" {
#endif



#define rc522_freq  1000000
#define rc522_mode  0
#define rc522_cs  -1
#define rc522_ss  15

#define mode_idle  0x00
#define mode_auth  0x0E
#define mode_receive  0x08
#define mode_transmit  0x04
#define mode_transrec  0x0C
#define mode_reset  0x0F
#define mode_crc  0x03

#define auth_a  0x60
#define auth_b  0x61

#define act_read  0x30
#define act_write  0xA0
#define act_increment  0xC1
#define act_decrement  0xC0
#define act_restore  0xC2
#define act_transfer  0xB0

#define act_reqidl  0x26
#define act_reqall  0x52
#define act_anticl  0x93
#define act_select  0x93
#define act_end  0x50

#define reg_tx_control  0x14
#define length  16
#define num_write  0

int keyA[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF } ;


//convert data to MSB format.

int cvmsb(int data){
  data = data << 1;
  data = data & 0x7E;
  data = data | 0x80;
  return data;
}

//convert data to hex format.

void cvhex(int data){
  printf("0x%x", data);
}



int rc522_read(int addr){
  
  struct mgos_spi *spi;
  spi = mgos_spi_get_global();
  
  addr = cvmsb(addr);
  
  uint8_t tx_data[1] = {addr};
  uint8_t rx_data[2] = {0};
  
  struct mgos_spi_txn txn = {
    .cs = rc522_cs,
    .mode = rc522_mode,
    .freq = rc522_freq,
  };

  txn.hd.tx_len = 1;
  txn.hd.tx_data = tx_data;
  txn.hd.dummy_len = 0;
  txn.hd.rx_len = 1;
  txn.hd.rx_data = rx_data;
  
  mgos_gpio_write(rc522_ss, 0);
  mgos_spi_run_txn(spi, false , &txn);
  mgos_gpio_write(rc522_ss, 1);
  
  return rx_data[0];
}

void rc522_write(int addr ,int data){

  struct mgos_spi *spi;
  spi = mgos_spi_get_global();
  
  addr = cvmsb(addr) & 0x7E;
  
  uint8_t tx_data[2] = {addr, data};

  
  struct mgos_spi_txn txn = {
    .cs = rc522_cs,
    .mode = rc522_mode,
    .freq = rc522_freq,
  };
  txn.hd.tx_len = 2;
  txn.hd.tx_data = tx_data;
  txn.hd.dummy_len = 0;
  txn.hd.rx_len = 0;

  mgos_gpio_write(rc522_ss, 0);
  mgos_spi_run_txn(spi, false , &txn);
  mgos_gpio_write(rc522_ss, 1);
}

void set_mask(int addr, int mask){
  int tem = rc522_read(addr);
  rc522_write(addr, tem | mask);
}

void clear_mask(int addr, int mask){
  int tem = rc522_read(addr);
  rc522_write(addr, tem & (!mask));
}

bool rc522_card_write(int command, int *data, int data_size, int *back_data, int *back_length){
  *back_length = 0;
  bool err = false;
  int irq = 0x00;
  int irq_wait = 0x00;
  int last_bits = 0;
  int n = 0;
  
  if(command == mode_auth){
    irq = 0x12;
    irq_wait = 0x10;  
  }
  
  if(command == mode_transrec){
    irq = 0x77;
    irq_wait = 0x30;  
  }
  
  rc522_write(0x02, (irq | 0x80));
  clear_mask(0x04, 0x80);
  set_mask(0x0A, 0x80);
  rc522_write(0x01, mode_idle);
  
  for(int i = 0; i != data_size; i++){
    rc522_write(0x09, data[i]);
  }
  
  rc522_write(0x01, command);
  
  if(command == mode_transrec){
    set_mask(0x0D, 0x80);
  }
  
  int i = 25;
  
  while(true){
    mgos_usleep(1000);
    n = rc522_read(0x04);
    i--;
    if(!((i != 0) && ((n & 0x01) == 0) && ((n & irq_wait) == 0))){
      break;
    }
  }
  clear_mask(0x0D, 0x80);
  
  if(i != 0){
    if((rc522_read(0x06) & 0x1B) == 0x00){
      err = false;
      if(command == mode_transrec){
        n = rc522_read(0x0A);
        last_bits = (rc522_read(0x0C) & 0x07);
        if (last_bits != 0){
          *back_length = (n - 1) * 8 + last_bits;
        }
        else{
          *back_length = n * 8;
        }
        if(n == 0){
          n = 1;
        }
        if(n > length){
          n = length;
        }
        for(int i = 0; i != n; i++){
          back_data[i] = rc522_read(0x09);
        }
      }
    }
    else{
      err = true; 
    }
  }
  return  err;
}

bool rc522_request(int *return_data){
  int req_mode[1] = {0x26};
  int back_data[2];
  bool err = true;
  int back_bits = 0;
  rc522_write(0x0D, 0x07);
  err =  rc522_card_write(mode_transrec, req_mode, 1, back_data, &back_bits);
  if(err || (back_bits != 0x10)){
    for(int i = 0; i != back_bits; i++){
      return_data[i] = 0;
    }
    return false;
  }
  for(int j = 0; j != 2; j++){
    return_data[j] = back_data[j];
  }
  return true;
}

bool rc522_anticoll(int *return_data){
  int back_data[5];
  int back_bits = 0;
  int serial_number[2];
  int serial_number_check = 0;
  bool err = false;
  rc522_write(0x0D, 0x00);
  serial_number[0] = act_anticl;
  serial_number[1] = 0x20;
  err =  rc522_card_write(mode_transrec, serial_number, 2, back_data, &back_bits);
  if(!err){
    if(back_bits == 5){
      for(int i = 0; i!=5; i++){
        serial_number_check = serial_number_check ^ back_data[i];
      }
      if(serial_number_check != back_data[3]){
        err = true; 
      }
    }
    else{
      err = true;
    }
  }
  for(int j = 0; j != 5; j++){
    return_data[j] = back_data[j];
  }
  return err;
}

int rc522_card_near(void){
  int type_data[2];
  int serial_data[5];
  if (rc522_request(type_data)){
    rc522_anticoll(serial_data);
    printf("serial number: 0x%x 0x%x 0x%x 0x%x 0x%x\n", serial_data[0], serial_data[1], serial_data[2], serial_data[3], serial_data[4]);
    printf("card type : 0x%x 0x%x\n", type_data[0], type_data[1]);
    return 1;
  }
  return 0;
}

int rc522_card_near_c2mjs(int data_count)
{
  int a=0;
  if(data_count==7)
  {
    a=rc522_card_near();

  }
  return a;
}


void rc522_init_set(void){
  mgos_gpio_set_mode(rc522_ss, 1);
  mgos_gpio_write(rc522_ss, 1);
  rc522_write(0x01, 0x0F);
  rc522_write(0x2A, 0x8D);
  rc522_write(0x2B, 0x3E);  
  rc522_write(0x2D, 30);  
  rc522_write(0x2C, 0);  
  rc522_write(0x15, 0x40);  
  rc522_write(0x11, 0x3D);
  int current = rc522_read(0x14);
  if (!(current & 0x03)){
    set_mask(0x14, 0x03);
  }
}



#ifdef __cplusplus
}
#endif

#endif 