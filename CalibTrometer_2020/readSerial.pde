// readSerial tab in CalibTrometer_2020 sketch
// methods: serialEvent
//      convertData  puts data in rawData file
//      testFileSave
//      getParams
//      readPar
//  signals from spectrometer:
//         $ - parameters recieved
//         % - end of data packet
//         ! - end of spectrum
//  Signal to spectrometer:
//         # - data packet recieved, ready for more


void serialEvent(Serial p){

  if(Comselected == false){
  } else {  // if comSelected is true
 buffer = null;
  buffer = p.readStringUntil(LINE_FEED);
  if(buffer != null){
    int j = buffer.indexOf("$");      // parameters recieved by ucontroller
    int k = buffer.indexOf("!");      // end signal  was !
    if(j!=-1){
      running = true;   // start acquiring
      println("$ signal received");
 
    }
      else if(k!=-1){
      running = false;
      println("! signal received");
        strData[counter] = buffer;
  println("end string "+counter+": "+buffer);
  dataAcq = true;

    }
   
    
  else if(running == true) {
  strData[counter] = buffer;
  counter++;

  p.write('#');    // signal u-controller to send next line
  println("next line called");
  }
    else{
    strParams[pCount] = buffer;
    pCount++;
  println("collecting parameters");
  }
   }  // end of if buffer not null loop
  } // end of com selected is true loop
} // end of serial event

void  convertData() {   // copied from wheetrometer_2020
  try{
  yMinRaw[spectra-1] = 0;
  yMaxRaw[spectra-1] = 0;
  RawSpecList.add(new GPointsArray());
  GPointsArray array2 = RawSpecList.get(spectra-1); 
  pixelNo = 0;
println("in convertData loop");
println("data strings: "+counter);
for(int h = 1; h<=counter; h++){

     String[] tokens = split(strData[h],".");
     for(int y = 0; y<tokens.length; y++){
     char check = tokens[y].charAt(0);
     if(check == '%'){             //  % indicates pause,
        } else 
        if(check== '!'){      // ! indicates end of data

        println("end signal recieved, line 67");
          array2.removeInvalidPoints();
          println("yMin: "+yMinRaw[spectra-1]+", yMax: "+str(yMaxRaw[spectra-1]));
        }else{
      int value0 = (tokens[y].charAt(0)-48)*80;
      value0 += tokens[y].charAt(1)-48;
      
      if(value0<yMinRaw[spectra-1]){
        yMinRaw[spectra-1] = float(value0);
      }
      if(value0>yMaxRaw[spectra-1]){
        yMaxRaw[spectra-1] = float(value0);
      }
      

     pixelNo++;
      array2.add(xData[pixelNo-1],value0);
     }
     }
}
}catch(Exception e){
  println("Baseline values not calculated");
}
  }

void testFileSave(int pix){
  GPointsArray array2 = RawSpecList.get(spectra-1);
  String[] testFile = new String[pix];
  for (int n = 0; n< pix; n++){
   testFile[n] = str(array2.getY(n)); 
  }
  saveStrings("testFile1",testFile);
  println("test file saved");
  
}

 void getParams(){    
   for (int a = 0; a<pCount; a++){
    int b = strParams[a].indexOf("A");
    int c = strParams[a].indexOf("B");
    int d = strParams[a].indexOf("C");
    int e = strParams[a].indexOf("+");  // indicates amp
    int f = strParams[a].indexOf("=");  // indicates offset
    if(b!=-1){
      readPar(0, strParams[a],b);
     }
    if(c!=-1){
    readPar(1, strParams[a],c);
    }
    if(d!=-1){
    readPar(2, strParams[a],d);
    }
    if(e!=-1){  // ampvalue
     sAmp = strParams[a].substring(e+1);
     amp.setText(sAmp); // amplification

    }
    if(f!=-1){  // offset value
    sOff = strParams[a].substring(f+1);
     offset.setText(sOff);
    }
   }
   if((gotABC[0] == true) && (gotABC[1]== true) && (gotABC[2] == true)){
      wavelengthCalc();  // method in  graph data tab,
      
      newXVals();
      pCount = 0;
  }
  }  // end of getParams method


void readPar(int a, String s, int b){  // read ABC parameters sent from
b++;
 s = s.substring(b);
 try{
 ABC[a] = Float.parseFloat(s);
 println("ABC["+a+"]: "+ABC[a]);
  } catch(Exception e){
 println("ABC values not read");
 }
 gotABC[a] = true;
}
 
