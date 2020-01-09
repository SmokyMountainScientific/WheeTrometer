
// Calibtrometer_2020
// Saved to GitHub 01/06/2020
// need to deal with comparing number of data points for saved and scanned spectra.
// loading saved spectra with different number of pixels will not work.
// multi-spectra files from thor kinetics analysis sketch on desktop
// July 9, 2019- re-wrote setup to read values from setupData.txt file
//    wavelength values calculated from second order polynomial using
//      constants read from setupData file
//    increment values for least squares refinement taken from file as well
//    Default values for integration time, loops and data averaging in there too. 

/////////////////////////////////////// Imports/////////////////////////////////
import java.awt.Toolkit;
import java.awt.GraphicsEnvironment;
import java.awt.GraphicsDevice;
import java.awt.DisplayMode;
import java.awt.*;
import java.util.*;
import java.io.*;                    // for file stuff
import grafica.*;                    // For chart classes.
import grafica.GPlot;
import controlP5.*;                  // for buttons and txt boxes
import processing.serial.*;          // for serial
//import PFrame.*;      // for extra windows

  boolean gotABC[] = {false,false,false};  // recieved baseline params from uCont
  String[] strParams = new String[10];  // parameters read from uController?
  int pCount = 0;
  boolean errorReport = false;
  String serialNo = "";  // serial number read from u-controller in comPort tab
  String spectName;  // = new String[0];  // name for spectrometer
  int lamps = 0;  // number of light sources
  int sourceNo = 0;  // index of light source
  String[] source = new String[5];

  String integTime;  // = new String[0];
  String sLoops;  // = new String[0];
  String sAvg;  // = new String[0];
  String sAmp = "100";
  String sOff = "100";
  String sMin; // = new String[0];
  String sMax;  // = new String[0];
  boolean boolAdd = true;
  String sMillis;
  int xPix = 0;  // number of source pixel lists read
  float peakYVal = 0;  // max intensity for peak pick

// variables for peak picking
 int xPick;
 int pickDelta = 15;
 int yPick;
 int xCenter;
 int yCenter;
 int linePix = 1000;
 float lineWav;// = 500;
 
  int inputX = 40;
  int inputY = height - 160;
  
// new stuff 06/09/19 for use in multi-spectra files
String[] headers = new String[20];
int spectra = 0;  // number of spectra
int nSpectra = 0;  // spectra added from file
boolean reading = false;
public GPlot plot, plot1, plot2;   // not sure how many of these are used
GPointsArray data = new GPointsArray(3694);
ArrayList<GPointsArray> RawSpecList = new ArrayList<GPointsArray>();
ArrayList<GPointsArray> ProcessedSpecList = new ArrayList<GPointsArray>();
float[] yMinRaw = new float[20];
float[] yMaxRaw = new float[20];

  // params for displaying file names
int specX = 800; 
int specY =100;
int deltaY = 20;
String[] sFileName = new String[20];
String[] cHeader = new String[20]; 
int nPixels;

   // Error stuff
  int iError=0;   // which error message to display
  boolean errorFlag = false;
  boolean refFlag = false; // no reference spectrum
  String[] errorTxt = {"","Error: Select one reference spectrum","Error: Too many spectra","Save Error: No spectra selected",
       "Pick error: must record spectrum to pick","Calibrate error: select calibration points", "Error: Select background spectrum"};

//   Display stuff
  String saved = "working on: write param file.";  // displayed on lower right of GUI
  int plotWidth;    // position and dimensions of plot calc'ed in setup()
  int plotHeight;
  int plotX = 20;
  int plotY = 80;
int iView = 0;  // 0: intensity, 1: Abs
//String[] viewTxt = {"Intensity","Absorbance"};
int runCount = 0;  // experiment count
int viewVal = 0;  // 0-raw, 1 is processed data, 3 is zoom
int oldViewVal = 0;  // used to return from zoom

boolean[] selectBox = {false,false,false,false,false,false,false,false,false,false,
        false,false,false,false,false,false,false,false,false,false};
        //color parameters for displaying spectra
int[] red = {  255, 0, 0, 85, 0, 170, 0, 170, 85, 85}; 
int[] green = {  0, 0, 255, 0, 85, 85, 170, 0, 170, 85};
int[] blue = {  0, 255, 0, 170, 170, 0, 85, 85, 0, 85};

//   limits for displays
 float[] xMin = {0,400,400,400};      // raw, abs, calc, zoom
  float[] xMax = {4000,900,900,900};
  float[] yMin = {0,0,0,0};
  float[] yMax = {4000,1,1,1};
  boolean xDataAvail = false;  // needed for using data from files
 /*
//   Calculations stuff
int bkground_scan = 999; // spectrum to use for background 
int ref_scan = 999;      // spectrum for 100% transmitance
boolean selected = false;            // spectrum for setting background or reference
boolean processFlag = false;  // has data been processed to give abs spec? 
    // processFlag set false when reference scan set

float trimXMin;
float trimXMax;
boolean trimmed = false;
boolean firstChart = true;
*/

//  Zoom stuff  (should be cleaned up)
float zoomXMax;
float zoomXMin;
float zoomYMax;
float zoomYMin;
int c0 = 90;    //265;  left x value for box
int c1 = 660;   // width of box
int c2 = 495;  // y position of box bottom
int c3 = 374;  // height of box
int deltaXbox = 50;
int deltaXwidth = 100;
int deltaYbox = 50;
int deltaYheight = 100;
float[] limits = new float[4];
int[] xZoom = {c0,c0+c1,c0,c0+c1};
int[] yZoom = {c2,c2-c3,c2,c2-c3};
 int[] xZoomLim = {c0,c0+c1,c0,c0+c1};  //neither box can go below c0 or wider than c1, 
 int[] yZoomLim = {c2,c2-c3,c2,c2-c3};  // y limits for the two boxes
 int[] xBox = {270,670};
 int[] yBox = {473,75};
 float[] fBox = {1.0,1.0,1.0,1.0};
 boolean zoomed = false;
 
 float[] oldChartLim = {0,1,0,1};

//int chartMode;
//float[] refScan = new float[3694];     // file to hold reference values
//float[] backgrScan = new float[3694];  // file to hold background values

// for mouse tab
  int selVal;
  boolean mouse0 = false;  // is box zero being dragged?
  String selTxt;
  
float chartXMax;
float chartXMin;
float chartYMax;
float chartYMin;

////////////// wavelength calibration stuff, added July, 2019
int peaks = 0;      // number of calibration peaks
int thisPeak = 0;
int[][] xPixels = new int[5][5];   // pixel positions for peaks
float[][] xLambda = new float[5][5];  // CFL wavelengths from wikipedia ref below
  // from https://commons.wikimedia.org/wiki/File:Fluorescent_lighting_spectrum_peaks_labelled.png
boolean[] useLambda = {false,false,false,false,false};


//  parameters for data acq and baseline calc and refinement:
  int iLoops = 4;  // loops
  int iAv = 4;     // 
  float intTime = 2;  // integration time
  float[] ABC = {249.123,.1495223,32.47777};  // baseline calcs
  float[] deltaABC = {1,.0002,.2};  // wavelength refinement
  float[] storedDeltas = new float[3];
  boolean[] refined = {false,false,false,false};  // 0-2 are for ABC params, 3 is overall
//  String[] textBoxInfo = {"","","","",""};  // integration time, loops, data averaging
  int[] peakPix = new int[5];        // pixel numbers that go with the peaks are xPixels
  float[] peakObs = new float[5];
  float[] peakCalc = new float[5];   // peak positions from calibration are xLambda[]
  
  String AmpVal = "";
  String Offset = "";
    String getMin = "";
 String getMax = "";
  float fMin = 0;
  float fMax = 0;
  int pixMin = 0;
  int pixMax = 3647;
  
////////////// data files
  int pixelNo = 0;   // total number of pixels recorded
  int nDataPts = 3694;    // maximum number of data points

ControlP5 cp5,cp5b,cp5c,cp5d, cp5Com, cp5Files, cp5Cal, cp5Spec;
Textfield  integration_Time, loops, Average, fileName,  amp, offset, min, max, sourceName,specName, serial;
  // Run_Interval,Number_of_Runs,peakLambda, peakPixel,,serial
/////////// variables for reading data ///////////////////////////////////////////
int dataCounter;
boolean running = false;
boolean dataAcq = false;
boolean graph = false;

//String[] wavedata ;   // wavelengths from file
float[] wavelength; //  calculated from ABC params

String buffer; // data read from serial
String[] strData = new String[200];  
int counter;  // counter for buffer strings
int LINE_FEED=10; 
//char[] sMode = {'0','1','2','3'};
String getLoops;
String getAvg;
                    

boolean startRun = false;
String IntTime;
String file2;
int iIntTime;


//////////////font varialbes///////////////////////////////
  PFont font;
  PFont font2;
  PFont font3;
  PFont font4;
  
/////     Information text  ////////
//  String[] modeList = {"Absorbance","Transmittance","Series"};
  String comStatTxt = "not connected";

////////////// colors /////////////////
  int bckgnd = #1754E8;  // #3C88A5;
  int bkgnd = #577193;
  int sel = #E59615;   // color for selected spectra

  /////// serial communications /////////////
  Serial serialPort;
static String[] comList ;               //A string to hold the ports in.
String[] comList2;               // string to compare comlist to and update
boolean serialSet;               //A value to test if we have setup the Serial port.
boolean Comselected = false;     //A value to test if you have chosen a port in the list.


//// controll variables
  int iMode = 1;  // changed to multiple scans  
  int pixAv = 1;   // number of pixels to average, calc'd in params tab
  int oldAv = 1;  // last value of pixAv
  float[] xData;

///////////////////////////////Setup////////////////////////////////////////////////////
void setup() {   // line 257
  size(950, 775); 

  plotWidth = width-290;    // position and dimensions of plot
  plotHeight = height-400;
  xCenter = plotWidth/2 + plotX;
  yCenter = plotHeight/2+ plotY+45;
  xPick = xCenter;
  yPick = yCenter;
  xZoom[0] =90;
  xZoom[1] = plotWidth+xZoom[0];
  
  selectBox[0] = true;       // display first spectrum
  font = createFont("ArialMT", 20);
    font2 = createFont("ArialMT", 20);
      font3 = createFont("ArialMT", 20);
        font4 = createFont("Andalus", 16);
        
//     
// read setup file from text //////////
//
String[] headerInfo = loadStrings("calSetupData.txt");
//println("cal setup data file read");
int size = headerInfo.length;
for (int g = 0; g<size; g++){
  readParLine(headerInfo[g]);  // in buttons tab, 499
}
  surface.setResizable(true);  // not currently used
  setupCharts();
  cp5_controllers_setup();
  setupComPort(); 
  connect();
//  println("initial value of deltaABC[0]: "+deltaABC[0]);
  findCursor();  // find position of cursor line in spectrum
   
}

////////////////////////////Draw/////////////////////////////   

void draw() {
  rectMode(CORNER);
  /////  why is this in draw loop?  //
  // read parameters from serial ///
   if(pCount !=0){
     getParams(); // in readSerial tab
   }
  try{
    background(bckgnd); 
  stroke(255);
  noFill();
    rect(30,height-210, 180, 90);    // run button box
    rect(30,height-110, 180, 100);   // serial box
    rect(260,height-210, 210, 200);  // param box
    rect(520,height-210, 210, 200);  // wavelengths & pixels box
fill(255);
//   fill(0);
   textFont(font2,16);
 //  fill(#080606);
   text("SmokyMtSci.com",width-220,height-30);
   text(saved,width-220,height-10);
   text("Spectra: "+spectra,specX,specY);  // display number of spectra
//  show spectra names
   for(int w = 1; w<=spectra; w++){
     if(selectBox[w-1] == true){  // if spectrum color: sel
      fill(sel);
    } else{
      fill(0);
    }
    text(headers[w],specX,specY+(w*deltaY));
    }
fill(0);

// error text
  if(errorFlag == true){
    fill(255);
    text(errorTxt[iError],400,26);
  }
  }
 catch(Exception e){
   if(errorReport == false){
   println(" error before plot");
   errorReport = true;
   }
 }
cp5Cal.show();
int p = xLambda[sourceNo].length;
for(int h = 0; h<p; h++){
  if(useLambda[h] == true){
  fill(sel);
  }else{
    fill(0);
  }
 text(xLambda[sourceNo][h]+" nm: pixel "+xPixels[sourceNo][h],540, height-140+(h*20));
 fill(0);
}
   textFont(font2);
 try{
if(reading == false){
  try{
     plot1.beginDraw();
      plot1.drawBackground();
      plot1.drawBox();
      plot1.drawXAxis();
      plot1.drawYAxis();
      plot1.drawTitle();
      plot1.endDraw();
  getYLims();
} catch(Exception e){
//  println("problem with y limits");
}

  int end = spectra;
         for(int h = 0; h< end; h++){
        if(selectBox[h] == true){
          GPointsArray array2 = RawSpecList.get(h);
          plot1.setPoints(array2);
          plot1.setXLim(xMin[viewVal],xMax[viewVal]);
          plot1.setYLim(yMin[viewVal],yMax[viewVal]);
          plot1.setPos(plotX, plotY);
          plot1.setDim(plotWidth, plotHeight);
          if(h==spectra-1){
            plot1.setLineColor(0);   // latest spectrum is black
          } else {
          plot1.setLineColor(color(red[h],green[h],blue[h],100));
          }
          plot1.beginDraw();
          plot1.drawLines();
          plot1.endDraw();
                  }  // end of selectBox[h] loop
      }    // end of h loop  
 
   }
 }
 catch(Exception e){
//   println("in plot");
 }

// pick lines
stroke(0);
int plotLo = plotY+40;
line(xPick,plotLo,xPick, plotLo+plotHeight);
rectMode(CENTER);
noFill();
rect(xPick,yPick,15,15);
rect(xPick,yPick,2*pickDelta,6*pickDelta);
fill(0);

text(nf(lineWav,0,1)+" nm", xPick,plotY+30);
stroke(255);
  textFont(font4,18); 
  text(comStatTxt, 100, 35);  // displays com port state, added May 18


////////////  zoom box
    if(zoomed == false){
    stroke(0);

  line(xZoom[0],yZoom[0],xZoom[0],yZoom[1]);
  line(xZoom[0],yZoom[0],xZoom[1],yZoom[0]);
  line(xZoom[1],yZoom[1],xZoom[1],yZoom[0]);
  line(xZoom[1],yZoom[1],xZoom[0],yZoom[1]);
  
  noFill();
  rectMode(CENTER);
  rect(xZoom[0],yZoom[0],10,-10);
  fill(229,120,21,100);              // semi-transparent yellow box
    rect(xZoom[1],yZoom[1],-10,10);
  fill(255);
  rectMode(CORNER);
  stroke(255);
  }
///// end of zoom box
  
  if (startRun == true){    // start run//////////////////////////  
  counter =0;  // no strings of data
  params();  // gets parameters for spectrum
   if(pixAv != oldAv){ // if number of pixels to average has changed  
     newXVals();
     }
    // send commands to microcontroller
      String commands = "&0,"+IntTime+","+getAvg+","+getLoops+",\n";  // mode zero
   //   String commands = "&"+sMode[iMode]+","+IntTime+","+sAvg+","+sLoops+",\n";
      println("mode: 0, integration: "+IntTime+", loops: "+getLoops+", sAv: "+getAvg);
      serialPort.write(commands);
      delay(100);
      serialPort.clear();
    startRun = false;  // do not re-run "if run Started" loop

  }  // end of if startRun is true loop

    ///////////////////////////////////////////////////////////////////////////read serial data //////////////////////////////////////////////////////////

   if(dataAcq == true){  // if data acquired but not converted
   //   println("line 322");
      convertData();  // in readSerial tab
      println("out of convertData loop");
      dataAcq = false;
      reading = false;
//      if(pixAv != oldAv){  // recalculate wavelength data if new averaging
  //      newXVals();
    //  }
    findCursor();
   }
}
