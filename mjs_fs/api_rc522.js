load('api_mqtt.js');
load('api_gpio.js');
load('api_pwm.js');
let rc522= {
  init : ffi('void rc522_init_set(void)'),
  near_c2mjs: ffi('int rc522_card_near_c2mjs(int)'),
  near: function(){
    let cdata=[0,0,0,0,0,0,0];
    let pnum=
    this.card_c2mjs(cdata);
    if((cdata[0]|cdata[1]|cdata[2]|cdata[3]|cdata[4]|cdata[5]|cdata[6])!==0){
      print('Serial Number :'+this.c2hex(cdata[0])+' '+this.c2hex(cdata[1])+' '+this.c2hex(cdata[2])+' '+this.c2hex(cdata[3])+' '+this.c2hex(cdata[4]));
      print('Card Type :'+this.c2hex(cdata[5])+' '+this.c2hex(cdata[6]));

      let res = MQTT.pub('my/topic', JSON.stringify({ "STATE": GPIO.read(0),"\nSN1": cdata[0], "\nSN2": cdata[1],"\nSN3": cdata[2], "\nSN4": cdata[3],"\nSN5": cdata[4], "\nCT1": cdata[5],"\nCT2": cdata[6]}), 1);
      print('Published:', res ? 'yes' : 'no');

    }
    return cdata;
  },
  card_c2mjs :function(card_data){
    let count =0;
    while(count<7){
      card_data[count]=this.near_c2mjs(count);
      count++;
    }
  },
  c2hex: function (value){
    let i = 0;
    while(value >= 16){
      value = value - 16;
      i++;
    }
    if(i < 10){
      i = i + 48;
      i = chr(i);
    }
    else{
      i = i + 55;
      i = chr(i);
    }
    if(value < 10){
      value = value + 48;
      value = chr(value);
    }
    else{
      value = value + 55;
      value = chr(value);
    }
    let strtem = ' 0x' + i + value;
    return strtem;
  },
};