// controlEvents tab, Calibtrometer_2020
// clear
// select
// set

void clear(){    // clear error flag
 iError = 0;
 errorFlag = false;
}

int select(){
 boolean gotOne = false;
 int specNo = 0;
 for (int i = 0; i<spectra; i++){
  if(selectBox[i] == true){
  specNo = i;
  if(gotOne == true){
   specNo = 21; 
  }
  }
 }
 return specNo;
}

void set(){
 println("setting x lims");
 sMin = cp5.get(Textfield.class, "min").getText();
 sMax = cp5.get(Textfield.class,"max").getText();
// textBoxInfo[5] = s0;
// textBoxInfo[6] = s1;
 float f0 = float(sMin);
 float f1 = float(sMax);
 println("x min: "+f0+", x Max: "+f1);
 for (int h = 0; h<4; h++){
   xMin[h] = f0;
   xMax[h] = f1;
 }
}
