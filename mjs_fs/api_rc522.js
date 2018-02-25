let rc522= {
  init : ffi('void rc522_init_set(void)'),
  near: ffi('void rc522_card_near(int*)'),
  
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