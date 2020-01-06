// controllers tab, Calibtrometer_2020
// spectrometerSetup
// cp5_controllers_setup

void spectrometerSetup(){
 specName.setText(spectName);

 if(Comselected == true){
   serial.setText(serialNo);
 }else{
 serial.setText(serialNo);
 }
 loops.setText(sLoops);  // loops
   println("spectrometer loops set up");
 integration_Time.setText(integTime);  // integration time
   println("inside spectrometer setup, line 8");
 Average.setText(sAvg);// averaging
}

void cp5_controllers_setup(){
 ////////////////////////////////////////////////Text Fields//////////////////////////////
  cp5 = new ControlP5(this);  //cp5 = new ControlP5(this);
  cp5Files = new ControlP5(this);
  cp5Cal = new ControlP5(this);
  cp5Spec = new ControlP5(this);  // setup fresh when new spectrometer selected
  
  PFont font = createFont("arial", 18);
  int buX = 60;  // button size
  int buY = 30;

  int buttonC = #E3CA85;
  int runC = #C64F2E;
 
  int loadX = 270;
int deltaX = 60;
  int viewX = loadX+deltaX;
  
  //////// input controllers
  
  // note: connect button in comPort tab
  
    cp5.addBang("Start_Run")
    .setColorBackground(255)//#FFFEFC 
        .setColorCaptionLabel(#030302) //#030302
          .setColorForeground(runC)
          .setColorActive(#06CB49)
            .setPosition(40, height-200)
              .setSize(buX,buY)
                .setTriggerEvent(Bang.RELEASE)
                  .setLabel("Start Run") //
                    .getCaptionLabel().align(ControlP5.CENTER, ControlP5.CENTER)  //.setLabel("Start_Run")
                      ;
                      
    /////////////// amp and offset //////////
    
    int ampX =40;
    int ampY = height-105;
    
     amp = cp5Cal.addTextfield("amp")  
    .setColor(#030302) 
      .setColorBackground(#CEC6C6)  
        .setColorForeground(#AA8A16)  
          .setPosition(ampX, ampY)
            .setSize(35, 25)
              .setFont(font)
                .setFocus(false) 
                   .setText(sAmp);   
    
     offset = cp5Cal.addTextfield("offset")  
    .setColor(#030302) 
      .setColorBackground(#CEC6C6)  
        .setColorForeground(#AA8A16)  
          .setPosition(ampX+50, ampY)
            .setSize(35, 25)
              .setFont(font)
                .setFocus(false)  
                    .setText(sOff); 
    
 cp5.addButton("WriteAmp")
  .setPosition(ampX+100,ampY)
  .setSize(buX,buY)
  .setLabel("Write Amp");    
                    
                      /////   Serial number
 int serX =40;
 int serY = height-50;
 serial = cp5Spec.addTextfield("serial_No")
    .setColor(#030302) 
      .setColorBackground(#CEC6C6)//(#FFFEFC) 
        .setColorForeground(#AA8A16) 
          .setPosition(serX, serY)               // new
            .setSize(50, 20)
              .setFont(font)
                .setFocus(false)               
                    .setText(serialNo);               
                    
      
  cp5.addButton("WriteSer")
  .setPosition(serX+100,serY)
  .setSize(buX,buY)
  .setLabel("Write Serial");     
    
    cp5.addButton("Save_run")
          .setPosition(110, height-160)
            .setSize(buX,buY)
                .setLabel("Save Run")
                    ;
  
  cp5.addButton("LoadFile")
          .setPosition(40, height-160)
            .setSize(buX,buY)
                .setLabel("Load Data")
                  .getCaptionLabel().align(ControlP5.CENTER, ControlP5.CENTER)  //.setLabel("Start_Run")
                    ;
                    
  /////////////////////////////////////////////////////
  
  fileName = cp5.addTextfield("file_Name")
    .setColor(#030302) 
      .setColorBackground(#CEC6C6)//(#FFFEFC) 
        .setColorForeground(#AA8A16) 
          .setPosition(800, 40)               // new
            .setSize(100, 20)
              .setFont(font)
                .setFocus(false)               
                    .setText("file");
                    
 /////// spectrometer name, set params, integration time, loops, average, min, max                  

 int nameX = 280;
 int nameY = height-200;
 
 specName = cp5Spec.addTextfield("spectrometer_Name")
    .setColor(#030302) 
      .setColorBackground(#CEC6C6)//(#FFFEFC) 
        .setColorForeground(#AA8A16) 
          .setPosition(nameX, nameY)               // new
            .setSize(160, 25)
              .setFont(font)
                .setFocus(false)               
                    .setText(spectName);
                    
   cp5.addButton("setParams")
  .setPosition(nameX+ 100,nameY+150)
  .setSize(buX,buY)
  .setLabel("set params"); 
  
     cp5.addButton("readParams")
  .setPosition(nameX,nameY+150)
  .setSize(buX,buY)
  .setLabel("read params"); 
  
integration_Time = cp5.addTextfield("Integration_Time")
    .setColor(#030302) 
      .setColorBackground(#CEC6C6)//(#FFFEFC) 
        .setColorForeground(#AA8A16) 
          .setPosition(nameX, nameY+50)               // new
            .setSize(50, 25)
              .setFont(font)
                .setFocus(false)           
                    .setText(integTime);  
                            
   loops = cp5.addTextfield("Loops")
    .setColor(#030302) 
      .setColorBackground(#CEC6C6)
        .setColorForeground(#AA8A16) 
          .setPosition(nameX+75, nameY+50)               // new
            .setSize(25, 25)
              .setFont(font)
                .setFocus(false)
                  .setText(sLoops);                
                                
    Average = cp5.addTextfield("data_Av")  // time based txt field
    .setColor(#030302) 
      .setColorBackground(#CEC6C6) 
        .setColorForeground(#AA8A16) 
          .setPosition(nameX+125, nameY+50)
            .setSize(25, 25)
              .setFont(font)
                .setFocus(false)
                     .setText(sAvg); 
                     
  min = cp5.addTextfield("min")  
    .setColor(#030302) 
      .setColorBackground(#CEC6C6)  
        .setColorForeground(#AA8A16)  
          .setPosition(nameX, nameY+100)
            .setSize(35, 25)
              .setFont(font)
                .setFocus(false)
                  .setText(sMin); 
                    
   max = cp5.addTextfield("max")  
    .setColor(#030302) 
      .setColorBackground(#CEC6C6)  
        .setColorForeground(#AA8A16)  
          .setPosition(nameX+ 60, nameY+100)
            .setSize(35, 25)
              .setFont(font)
                .setFocus(false)
                   .setText(sMax);
                            
 cp5.addButton("clear")
    .setColorBackground(buttonC)//#FFFEFC 
        .setColorCaptionLabel(#030302) //#030302
          .setColorForeground(buttonC)  
          .setPosition(viewX, 10)
            .setSize(buX,buY)
                .setLabel("Clear Flag")
                  .getCaptionLabel().align(ControlP5.CENTER, ControlP5.CENTER)  
                    ;
                    

cp5.addButton("zoom")
    .setColorBackground(buttonC)//#FFFEFC 
        .setColorCaptionLabel(#030302) //#030302
          .setColorForeground(buttonC)  
          .setPosition(width-250, 40)
            .setSize(buX,buY)
                .setLabel("Zoom")
                  .getCaptionLabel().align(ControlP5.CENTER, ControlP5.CENTER)  //.setLabel("Start_Run")
                    ;
                    
//////////////////////////  peak pick, calculate, write params
 int peakX = width -200;
 int peakY = height -200;
 
    cp5.addButton("peak_pick")
     .setPosition(peakX, peakY) 
      .setSize(buX,buY)
      ;
      
   cp5.addButton("ref_set")
     .setPosition(peakX-110, peakY)
     .setSize(buX,buY)
      ; 
      
  cp5Cal.addButton("calculate")
     .setPosition(peakX, peakY+30)
     .setSize(buX,buY)
      ; 
      
   cp5Cal.addButton("write_ABC")  //save")
     .setPosition(peakX, peakY+60)
     .setSize(buX,buY)
      ; 
      
   sourceName = cp5Cal.addTextfield("source")  
    .setColor(#030302) 
      .setColorBackground(#CEC6C6)  
        .setColorForeground(#AA8A16)  
          .setPosition(peakX-220,peakY)
            .setSize(100,25)
              .setFont(font)
                .setFocus(false)
                   .setText(source[sourceNo]);                  
   ;
  
}
