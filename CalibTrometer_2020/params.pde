// params tab, Calibtrometer_2020


public void params() {                 // get parameters from text box
  sMillis = cp5.get(Textfield.class, "Integration_Time").getText();
  iIntTime = round(float(sMillis)*1000);
  IntTime = nf(iIntTime, 6);   // make ScanR have 3 digits. pad with zero if no digits
  println("integration: "+IntTime+" microseconds");
  getLoops = cp5.get(Textfield.class, "Loops").getText();
  
  getAvg = cp5.get(Textfield.class, "data_Av").getText();
  getMin = cp5.get(Textfield.class, "min").getText();
  getMax = cp5.get(Textfield.class, "max").getText();
  try{
  fMin = float(getMin);
  xMin[0] = fMin;
  fMax = float(getMax);
  xMax[0] = fMax;
  int counter = 0;
  while (xData[counter] < fMin){
  
  counter++;

  }
  pixMin = counter-1;  // a pix + b + c*sin((pix-p0)/(p1-p0))
  while (xData[counter] <= fMax){
  counter++;
  }
  pixMax = counter;
} catch(Exception e){
  }
println("through params loop");
}
