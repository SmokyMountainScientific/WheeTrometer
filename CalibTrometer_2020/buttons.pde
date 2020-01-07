/* buttons tab, Calibtrometer_2020
  contains code for the following methods:
     start run
     clearArray- removes data from array
     calibrate 
     pick - picks the largest y value in view, returns x 
     calculate
     calculate residual
     refine
     writeFile    - write param files to computer
     readParams   - read parameter files from computer
     ref_set      - change reference set
     setSpec   
     WriteABC     - write wavelength parameters
     WriteAmp     - write amp params
     WriteSer      - write serial number to uCont
     readParLine   -reads parameters from uCont
*/


public void Start_Run() {  // start run 
  storedDeltas = deltaABC;  // deltas returned to original values on line 242
  if(spectra == 20){
    iError = 2;  // error message
    errorFlag = true;
  }else{
  reading = true;
  startRun = true;             // starts actual run
  dataCounter = 0;             // reset data counter
  counter = 0;                 // strings of data recieved from u-cont.
  if(iMode !=0 || spectra == 0){
    println("spectra is zero");
  spectra++;
  println("spectrum: "+spectra);
  headers[spectra] = cp5.get(Textfield.class, "file_Name").getText()+spectra;  // need file name
  println("spectra: "+headers[spectra]);
  selectBox[spectra-1] = true;  // not sure about the -1
  RawSpecList.add(new GPointsArray());
  yMinRaw[spectra-1] = 0;
  yMaxRaw[spectra-1] = 0;

  }else{   // if iMode is zero, need to clear arrAy
  clearArray(0);
    }
  }
  println("through start button method");
}


void clearArray(int y){
  GPointsArray array = RawSpecList.get(y);
  int a = array.getNPoints(); //1000;
  array.removeRange(0,a);
}

void peak_pick(){
  float maxY = 0;
  println("pick, line 56");  // gets here
  int spectrum = 0;
  if(spectra == 0){
    errorFlag = true;
    iError =4;
  }else{             // we have spectra
  
   // finding peak number, 
  int peakNo = 0;
 int p = 0;   // find number of selected wavelengths
 for(int r = 0; r<5; r++){  // assumes five peaks
  if(useLambda[r] == true){  // peak selected
   p++; 
   peakNo = r;
   println("finding peak "+r);
  }
 }
 if(p != 1){
   errorFlag = true;
   iError = 5;
 }
  if(errorFlag == false){    // still in baseline, if only one peak selected
  
    for(int h = 0; h<spectra; h++){
    if(selectBox[h] == true){ // figure out which spectrum to measure
    spectrum = h;
    println("checking spectrum "+spectrum);

 int j = 0;
   println("before pic method, line 86");
 j= pick(spectrum);
   if(peakYVal > maxY){
     maxY = peakYVal;
   println("peak number "+peakNo+", pixel "+j);
   xPixels[sourceNo][peakNo] = j;
   }
  }  // end of check selectBox h
}  // end of cycle through spectra
   useLambda[peakNo] = false;  // turn off useLambda
  }
  }
}  // end of peak_pick routine

int pick(int g){  // pick largest y value in zoom and return x position
println("line 99, pick method");
GPointsArray array = RawSpecList.get(g);
int min=0;  // pixel numbers
int n = array.getNPoints();   // number of data
int max = n;
int peakPix = 0;
float peakWavelength = 0;
float thisYVal = 0;
peakYVal = 0;

// deal with error of no spectra recorded
if(spectra == 0){
  errorFlag = true;
  iError = 4;
}
if(errorFlag == true){
  println("errorFlag");
}
// if there is a spectrum recorded do this
else{
try{
println("in pick routine, find pixel for "+lineWav+" nm");
int delta = 100;
min = findPixel(lineWav)- delta;
max = 2*delta + min; 

println("pixel range: "+min+", "+max);     
float a = array.getX(min);
float b = array.getX(max);      // this gives wrong answer, falue from xData[]
println("wavelength range: "+a+", "+b);
   
println("array got, line 123");
for(int x = min;x<max; x++){
  thisYVal = array.getY(x);
  if(thisYVal> peakYVal){
   peakPix = x;
   peakYVal = thisYVal;
   
   peakWavelength = array.getX(x);
  
  }
}
println("peak at "+peakWavelength+": pixel "+peakPix); 
}
catch(Exception e){
  println("exception in pick routine");
}
}
return peakPix;
}

void calculate(){     // use peak positions to calculate wavelengths
 refined[3] = false;
 GPointsArray array = RawSpecList.get(0);
 println("got array");
 int size = array.getNPoints();     // get number of data points in array
 float[] xVals = new float[size];    // set up array of x values
  int nPeaks = 0;                    // number of peaks to use
  for (int y = 0; y<5; y++){
   if(useLambda[y] == true){
     peakObs[nPeaks] = xLambda[sourceNo][y];
     peakPix[nPeaks] = xPixels[sourceNo][y];
   nPeaks++;
 }
  }
  if(nPeaks == 0){
    errorFlag = true;
    iError = 5;
  }
  else if(nPeaks == 1){  // shift using existing slope
  float minX = array.getX(0);
  float maxX = array.getX(size-1);

  ABC[1] = (maxX - minX)/size;              //  current slope
  }else if(nPeaks == 2){
   ABC[1] = (peakObs[1] - peakObs[0])/(peakPix[1] - peakPix[0]);   // estimate slope
  
  ABC[0] = - ABC[1]*peakPix[0]+peakObs[0];  // estimate intercept
  } else{

  // begin least squares refinement
    
  float R = calculateResid(nPeaks);
  println("initial constant values:");
  println("zero order: "+ABC[0]+", 1st order: "+ABC[1]+", sin correction: "+ABC[2]); 
  println("initial R^2: "+R);
  
  float resid0 = calculateResid(nPeaks);
  float resid1 = 0;
  
  
  for(int p = 0; p<5; p++){
    refined[3] = false;
  while(refined[3] == false){

  refine(0,nPeaks);
  refine(1,nPeaks);
  refine(0,nPeaks);
  refine(2,nPeaks);
  refine(0, nPeaks);
  refine(1, nPeaks);
  refine(0, nPeaks);
  resid1 = calculateResid(nPeaks);
  if(resid1>=resid0){
    refined[3] = true;
  }else{
  resid0 = resid1;

   }
  }
        for (int w = 0; w<3; w++){   // moved from above
      deltaABC[w] = deltaABC[w]/3;   // change size of iteration for finer refinement
      refined[w] = false;
    }
      println("zero order: "+ABC[0]+", 1st order: "+ABC[1]+", sin correction final: "+ABC[2]); 
  println("initial R^2: "+resid0);
  }  // end of p loop

 }
 float f;
 for(int t = 0; t<size; t++){
   f = sin(PI*t/size);
  xVals[t] = (ABC[1]*t)+ABC[0] + ABC[2]*f;     //  calculate wavelengths
  for(int w = 0; w<spectra; w++){     // set wavelengths in each array
    GPointsArray array2 = RawSpecList.get(w);
    array2.setX(t,xVals[t]);
  }
 }
 for(int i = 0; i<3; i++){  // dont reset zoom
 xMin[i] = xVals[0];    // reset x axes
 xMax[i] = xVals[size-1];

  }
  println("storedDeltas[0]: "+storedDeltas[0]);
  deltaABC = storedDeltas;   // restore deltas to original values
  println("deltaABC[0] restored: "+storedDeltas[0]);
  params();
  wavelengthCalc();  // not sure about this one
  newXVals();        // this one either
  
  }


float calculateResid(int nPeaks){
  float residSq = 0;
  float sQ = 0;
  float f = 0;
  for(int a = 0; a <nPeaks; a++){
    f = sin(PI*peakPix[a]/3647);
  peakCalc[a] = (peakPix[a]*ABC[1])+(f*ABC[2])+ABC[0];
  sQ = sq(peakCalc[a] - peakObs[a]);
  residSq = residSq + sQ; 
  }
//  println ("R squared: "+residSq);
  return residSq;
 }
 
   
 float refine(int a,int nPeaks){
   float oldResid = calculateResid(nPeaks);
   float newResid = 0;
   float oldConst = ABC[a];
   refined[a] = false;
       boolean increment = true;
     while(refined[a] == false){
     if(increment == true){
       ABC[a] += deltaABC[a];    // increment constant
     } else {
       ABC[a] -= deltaABC[a];    // decrement constant
     }
     newResid = calculateResid(nPeaks);
     if(newResid < oldResid){
     oldResid = newResid;
     oldConst = ABC[a];   // ... and we go round again...
     } else {
      ABC[a] = oldConst;
      increment =! increment;
      if(increment == true){
        refined[a] = true;
     }
     }  //  end of else
     }  // end of refined is false loop
 //    println("new R squared: "+oldResid+", new parameter "+a+" params: "+ABC[0]+", "+ABC[1]+", "+ ABC[2]);
        return oldResid; 
 }  // end of refine method
 


  void setParams(){  // create param file
     selectOutput("Save parameter file location:", "writeFile");
 }  
 void writeFile(File selection){
     file2 = selection.getAbsolutePath();
     Setspect();  // gets current spectrometer parameters and sets to current spectrometer
     int u = (2*lamps)+10; 
     String[] lines = new String[u];
     lines[0] = "  // R: refinement parameters:";
     lines[1] = "R,1.0,.0002,0.2";  // may want to adjust in future, not now
     lines[2] = "  // S: spectrometer name and serial number";
     lines[3] = "S,"+serialNo+","+spectName;
     lines[4] = "  // C: wavelength calibration parameters";
     lines[5] = "C,"+str(ABC[0])+","+str(ABC[1])+","+str(ABC[2]);
     lines[6] = "  // P: data collection parameters";
     lines[7] = "P,"+integTime+","+sLoops+","+sAvg+","+getMin+","+getMax;
     lines[8] = "  // W: source name and wavelengths";
     lines[lamps+9] = "  // X: source pixel numbers";

 for(int h = 0; h<lamps; h++){
   // need to add xLambda to list
   String lambda = "";
   String pix = "";
   for (int n = 0; n<5; n++){
    lambda = lambda+ str(xLambda[h][n])+",";
    pix = pix + str(xPixels[h][n])+",";
   }
   int k = h +8;
        lines[k+1] = "W,"+source[h]+","+lambda;
        lines[k+2+lamps] = "X,"+pix;
      }
 
      saveStrings(file2+".txt",lines);
 }
 
 void readParams(){
   selectInput("Select parameter file location:", "getFile");
 }
 
 void getFile(File selection){
      if (selection == null) {
    println("Window was closed or the user hit cancel.");
  } 
  else {
    try{
 String[] file2str = loadStrings(selection.getAbsolutePath());    // load file
 int lines = file2str.length;
 for(int j = 0; j<lines; j++){
   int  u = file2str[j].indexOf("//");
   if(u==-1){                          // if not a comment
   readParLine(file2str[j]);
    }
    }  // end of j loop
    spectrometerSetup();
    } // end of try
    catch(Exception e){
      println("error in getFile, buttons, line 404");
    }
  }
 }
 

 void ref_set(){   // change reference line set
   int sources = 0;
   int totSources = source.length;
   for (int h = 0; h<totSources; h++){
    if(source[h] != null){
      sources++;
   }
   }
   sourceNo++;
   sourceNo = sourceNo%sources;
   sourceName.setText(source[sourceNo]);
  println("sources: "+sources+" source number: "+sourceNo); 
 }
 

 void Setspect(){
   params();
   sLoops = getLoops;
   integTime = sMillis;
   sAvg = getAvg;
   sAmp = AmpVal;
   sOff = Offset;
   sMin = getMin;
   sMax = getMax;
//  println("setting spectrometer "+specNo); 
 }
 
 void WriteAmp(){
  sAmp =  cp5Cal.get(Textfield.class, "amp").getText();
  sAmp = sAmp.substring(0,3);  // cuts extraneous stuff off
  sOff = cp5Cal.get(Textfield.class, "offset").getText();  
  sOff = sOff.substring(0,3);
  String sent = "A"+sAmp+sOff;
  println("serial port write: "+sent);
      serialPort.write(sent);
 }
 
 void WriteSer(){
    // set serial no to text from textfield
    serialNo = cp5Spec.get(Textfield.class, "serial_No").getText();
     String sent = "W"+serialNo;
    // write to microcontroller with "W" as first char
    println("serial port write: "+sent);
    serialPort.write(sent);

 }
 
   void write_ABC(){
   String commands = "B";
   try{ // turn ABC values into six char strings
   println("ABC[0]: "+ABC[0]);

   for(int n = 0; n<3;n++){
   String aStr = str(ABC[n]);
   int r = aStr.indexOf("0");
   if(r == 0){
     aStr = aStr.substring(1,7);
   }else{
   aStr = aStr.substring(0,6);
   }
   println("ABC["+n+"]: "+aStr);
   commands = commands+aStr;
   }
   println("command string: "+commands);
   } catch(Exception e){
     println("Problem in writeABC method");
   }
   try{
     serialPort.write(commands);
   } catch(Exception e){
   println("error in writing baseline parameters");
 }
 }
 
  void readParLine(String line){
    try{
      if(line.indexOf("//")==-1){  // if not a comment
      String[] tokens =line.split(",");
      if(tokens[0].indexOf("R")!=-1){  // refinement parameters
        println("R found");
        for(int y = 1; y<4; y++){
        deltaABC[y-1] = float(tokens[y]); // get refinement params
     }
      println("refine parameter[0]: "+deltaABC[0]);
     }
     else if(tokens[0].indexOf("S")!=-1){  // Serial and name
      serialNo = tokens[1];
      spectName = tokens[2];
      println("spectrometer "+serialNo+": "+spectName);
     }
     else if(tokens[0].indexOf("C")!=-1){  // baseline calibration parameters
     
     }
     else if(tokens[0].indexOf("P")!=-1){  // data collection parameters
       integTime = tokens[1];
       sLoops = tokens[2];
       sAvg = tokens[3];
       sMin = tokens[4];
       sMax = tokens[5];  
     }
     else if(tokens[0].indexOf("W")!=-1){  // Wavelength parameters
     lamps++;
     println("reading info for lamp "+lamps);
     source[lamps-1] = tokens[1];
     int s = tokens.length;
     for(int u = 2; u<s; u++){
        xLambda[lamps-1][u-2] = float(tokens[u]);
     }
     }
     else if(tokens[0].indexOf("X")!=-1){  // refinement parameters
    xPix++;
    int s = tokens.length;
  for(int u = 1; u<s; u++){
    int v = u-1;
    xPixels[xPix-1][v] = int(tokens[u]);
   }
     } else {}
      }
    }  // end of try
    catch(Exception e){
      println("problem in read parameters method");
    }
  }
  //   end of readParLine
