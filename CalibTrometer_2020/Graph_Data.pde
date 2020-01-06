// Graph_Data tab, calibtrometer_2020
// wavelengthCalc() called in setup
// newXVals() method called on line 21 graph_data method
// graph_data() method called on line ~397 main tab, original method
//

void wavelengthCalc(){
      print("wavelengths ");
 int xDataSize = 4000; //3647;                // setup assumes no pixel averaging.
  wavelength = new float[xDataSize];
  float f =0;
for (int h = 0; h<xDataSize; h++){
  f = sin(PI*h/xDataSize);
    wavelength[h] = ABC[0]+h*ABC[1]+ABC[2]*f;
    }
    println("calculated");
    xMin[0] = wavelength[0];
    xMax[0] = wavelength[3646];
}

void newXVals(){

  xData = new float[wavelength.length/pixAv];
  for(int p = 0; p<xData.length; p++){
    xData[p] = 0;
    for(int j = 0; j< pixAv; j++){
      xData[p] += wavelength[p*pixAv+j];
      }
    xData[p] = xData[p]/pixAv;
  }
  oldAv = pixAv;
  xDataAvail = true;
//  println("new x data available");
}


void getYLims(){
  yMin[0] = 0;
  yMax[0] = 0;
 for(int r = 0; r<spectra; r++){
   if(selectBox[r] == true){
   if(yMinRaw[r]<yMin[0]){
    yMin[0] = yMinRaw[r];
  }
    if(yMaxRaw[r]>yMax[0]){
    yMax[0] = yMaxRaw[r];
  }
 }
 }
}
