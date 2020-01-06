// Com_Port tab on Calibtrometer 2020
// code on this tab sets up communications with the microcontroller

void setupComPort() {
  /////////  connect button ////////////
  cp5Com = new ControlP5(this); 
  cp5Com.addButton("connect")
    .setPosition(20, 20)
      .setSize(50, 30)
        ;
}

/////////// connect button program //////////
public void connect() {
  pCount = 0;
    for (int a = 0; a<3; a++){
  gotABC[a] = false;
   }
  Comselected = false;
 // background(bkgnd);
  println("connect button pressed");
  try {
       serialPort.clear();
       serialPort.stop();
  }
    catch(Exception e) {}
  comList = null;
  comList = Serial.list(); // correct method, January 2019  
  int n = comList.length;
  println("com list length = "+n);
  if (n == 0) { 
    comStatTxt = "No com ports detected";
  }
  else {
    int k = 9999;
    for (int m = 0; m <= n-1; m++) {
      try {
      serialPort = new Serial(this, comList[m], 115200);
      serialPort.write('*');   // initiate contact
      // listen for return character '&'
      delay(100);
      if (serialPort.available () <= 0) {
        println (comList[m]+" not responsive");
      }
      else {
  //    if (serialPort.available() > 0)
    //  {
        buffer = null;
        buffer = serialPort.readStringUntil(LINE_FEED);
        int y = buffer.indexOf("&");
        if(y!=-1){
          println (comList[m]+" responsive");
          k = m;
                    try{
            serialNo = buffer.substring(1,5);
          println("Serial: "+serialNo);
          }
          catch(Exception e){
            println("no serial no avialable");
          }
        }else {
          println("Com port says: "+buffer);
        }
        serialPort.clear();
        serialPort.stop();
      }
    }                       //  end of try loop
          catch(Exception e) {

      print(comList[m]);
      println(" not responsive");
    }    /// end of catch thing ///////////////

    }  // end of itterative look at ports
    if (k == 9999) {
      comStatTxt = "No response";
    } else {
      serialPort = new Serial(this, comList[k], 115200); 
      comStatTxt = "Connected on "+comList[k];
      Comselected = true;
    }
    try{
    serial.setText(serialNo);
    serialPort.write('@');
    }
    catch(Exception e){
      println("problem while looking for com ports");
    }
  }
}
