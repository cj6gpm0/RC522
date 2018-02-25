let rc522= {
  init : ffi('void rc522_init_set(void)'),
  near_c2mjs: ffi('int rc522_card_near_c2mjs(int)'),
  near: function(){
    let cdata=[0,0,0,0,0,0,0];
    this.card_c2mjs(cdata);
    return cdata;
  },
  card_c2mjs :function(card_data){
    let count =0;
    while(count>7){
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