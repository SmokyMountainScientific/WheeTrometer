// mouse tab Calibtrometer_2020
// to figure this out, you need to know the x,y position of the chart: 20,80
// you also need to know the margins: top: 40, left: 70, bottom: 60, right: 30
// width of chart: plotWidth is 660
// plotHeight is 375.

// on this tab: 
//     mousePressed()
//     mouseReleased()
//     mouseDragged()
//     zoom
//     findCursor



 void mousePressed(){
// part 1, selecting a spectrum by clicking on it's name
//println("mouse x: "+mouseX+", mouse y: "+mouseY);

 for (int q = 1; q<=spectra; q++){
   int maxY = deltaY*(q) + specY+6;    // size of box enlarged by 6 to fill area        
   int minY = deltaY*(q) + specY-14;

if (mouseX > specX-40 && mouseX <specX+100 && mouseY > minY && mouseY < maxY){  

  selectBox[q-1] =! selectBox[q-1];
  print("file "+q);
  println(" selected, state = "+selectBox[q-1]);  // stuff
  
}  // end of if statement

 }

 for (int r = 0; r<spectra; r++) {
   println("Select box "+r+": "+selectBox[r]);
 }
// part 2, calibration wavelengths
int calX = 520;  
int calY = 160;
int xDelta = 170;
int yDelta = 20;  


   for (int q = 1; q<6; q++){
   int maxY = yDelta*(q) + height-calY;    // size of box enlarged by 6 to fill area        
   int minY = yDelta*(q) + height-calY - 20;

if (mouseX > calX && mouseX <calX+xDelta && mouseY > minY && mouseY < maxY){  

  useLambda[q-1] =! useLambda[q-1];
  print("wavelength "+q);
  println(" selected, state = "+useLambda[q-1]);  // stuff
  
}  // end of if statement

 }
 }

 
 void mouseReleased(){
   mouse0 = false;
 }
 void mouseDragged(){
   int delta = 40;
   // new stuff for peak picking
  if(mouseX > xPick-delta && mouseX < xPick+delta && mouseY >yPick-delta && mouseY < yPick+delta){
   xPick = mouseX;
   yPick = mouseY;
//   float xFract = (xPick-90)/660;

 findCursor();
  }
   
  if(mouseX > xZoom[0]-delta && mouseX < xZoom[0]+delta && mouseY >yZoom[0]-delta && mouseY < yZoom[0]+delta){
   xZoom[0] = mouseX;
   yZoom[0] = mouseY;
   mouse0 = true;
  }
    if(mouseX > xZoom[1]-delta && mouseX < xZoom[1]+delta && mouseY >yZoom[1]-delta && mouseY < yZoom[1]+delta &&mouse0==false){
   xZoom[1] = mouseX;
   yZoom[1] = mouseY;
  }
    if(xPick < xZoomLim[0]){
    xPick = xZoomLim[0];
 }
     if(xPick > xZoomLim[1]){
    xPick = xZoomLim[1];
 }
     if(yPick > yZoomLim[0]){
    yPick = yZoomLim[0];
 }
      if(yPick < yZoomLim[1]){
    yPick = yZoomLim[1];
 }
  if(xZoom[0] < xZoomLim[0]){
    xZoom[0] = xZoomLim[0];
    println("outside x limits, line 75");
 }
   if(xZoom[0] > xZoomLim[1]){
    xZoom[0] = xZoomLim[1];
//    println("outside x limits, line 79");
   }
   
   if(xZoom[1] < xZoomLim[2]){
    xZoom[1] = xZoomLim[2];
  //      println("outside x limits, line 84");
 }
   if(xZoom[1] > xZoomLim[3]){
    xZoom[1] = xZoomLim[3];
//        println("outside x limits, line 88");
 }
   if(yZoom[0] > yZoomLim[0]){
    yZoom[0] = yZoomLim[0];
  //      println("outside y limits, line 92");
 }
   if(yZoom[0] < yZoomLim[1]){
    yZoom[0] = yZoomLim[1];
  //  println("outside y limits, line 96");
   }
   
   if(yZoom[1] > yZoomLim[2]){
    yZoom[1] = yZoomLim[2];
 //   println("outside y limits, line 101");
 }
   if(yZoom[1] < yZoomLim[3]){
    yZoom[1] = yZoomLim[3];
   // println("outside y limits, line 105");
 }
 }
 
 void zoom(){
     if(zoomed == false){
       oldViewVal = viewVal;
       viewVal = 3;
     println("zooming");
     
     float iDx = 660; //xBox[1] - xBox[0];  // width of the plot
     float iDy = 375;  //yBox[1] - yBox[0];   // height of the plot
     float xZ0 = xZoom[0]- 90; //xBox[0];    // how far is zoom box 0 from edge
     float yZ0 = 495-yZoom[0];   //- yBox[0];
     float xZ1  = xZoom[1]- 90;  //xBox[0];  // how far is zoom box 1 from edge
     float yZ1 = 495-yZoom[1];    //- yBox[0];
     println("x shifts: "+xZ0+", "+xZ1);
     
     float f0 = xZ0/iDx;
     println("f0 calculated: "+xZ0+" / "+iDx+" = "+f0);
     fBox[0] = xZ0/iDx;  //(xBox[1]-xBox[0]);        //fraction x0
     fBox[1] = yZ0/iDy;  //(yBox[1]-yBox[0]);        // fraction y0
     fBox[2] = xZ1/iDx;  //(xBox[1]-xBox[0]);        // fraction x1
     fBox[3] = yZ1/iDy;  //(yBox[1]-yBox[0]);         // fraction y1
   //  fBox[1] = 1-fBox[1];
   //  fBox[3] = 1-fBox[3];
   println("x zoom values: "+xZoom[0]+", "+xZoom[1]); 
   println("box x dimensions: "+xBox[0]+", "+xBox[1]);
      println("y zoom values: "+yZoom[0]+", "+yZoom[1]);
    println("box y dimensions: "+yBox[0]+", "+yBox[1]);  
    float deltaX = xMax[0] - xMin[0];
    float deltaY = yMax[0] - yMin[0];
   
   xMin[3] = xMin[0] +(fBox[0]*deltaX);   // was zoomXMin
   xMax[3] = xMin[0] +(fBox[2]*deltaX);
   yMin[3] = yMin[0] +(fBox[1]*deltaY);
   yMax[3] = yMin[0] +(fBox[3]*deltaY);
   
   println("delta x: "+deltaX+" delta y: "+deltaY);
   println("x fractions: "+fBox[0]+", "+fBox[2]);
      println("y fractions: "+fBox[1]+", "+fBox[3]);
   print("x min: "+xMin[3]);
   println(", x max: "+xMax[3]);
   print("y min: "+yMin[3]);
   println(", y max: "+yMax[3]);
 //     zoomed = true;  
 //    displayCharts();  // cut 6/22/2019
     println("past charts displayed");

   } else {
      viewVal = oldViewVal;
    }

   zoomed =! zoomed;
   findCursor();
 }
 
int findPixel(float nm){  // finds the first pixel where the wavelength is above nm
GPointsArray array = RawSpecList.get(0);  // get gpointsArray[0]
int end = array.getNPoints();
println("finding pixel for "+nm+" nm wavelength");
  int p = 0;
  boolean found = false;
  while(found == false){
    if(p==end){
      found = true;
    }
    if(xDataAvail == false){
      pixAv = int(cp5.get(Textfield.class, "data_Av").getText());
      wavelengthCalc();
      newXVals();
    }
    float value = array.getX(p);
    if(value>nm){  // was xData[p]
    found = true;
    println("first wavelength above "+nm+": "+value+", pixel "+p);
  } else {
  p++;
  }
  }
  return p;
}
 
 void findCursor(){
   int pixWidth = 3646;  // width  of display in pixels
   if(zoomed == false){
   linePix = int((xPick-90)*3646/660);
   yCenter = 495-yPick;
   lineWav = (xMax[0]-xMin[0])*(xPick-90)/660+xMin[0];
   }else {  // if zoomed is true
   pixWidth = xZoom[1]-xZoom[0];  // need pixel width, must be in zoom somewhere. 
   linePix = int((xPick-90)*pixWidth/660);
   lineWav = (xMax[3]-xMin[3])*(xPick-90)/660+xMin[3];
   }
   println("cursor at "+lineWav);
 }
