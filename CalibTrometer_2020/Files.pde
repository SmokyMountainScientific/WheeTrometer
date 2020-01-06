// Files tab, calibrtrometer
//  new tab June /2019
//  original load data code copied from spectral analysis 2
// note: xVal only used in files tab
// note:  What to do if saved data not same number of pixels as current data?
// note: yVal and yCalc removed, June 2019.  need to put values into arrays

// move save method code from buttons tab
public void Save_run() {             // set path bang   
  selectOutput("Select a file to process:", "fileSelected");
}
  // saving file
void fileSelected(File selection) {
  println("in fileSelected");
  int strings = 0;
   if(pixAv == 1){
     strings = 3648;
   } else if (pixAv == 2){
     strings = 1824;
   } else if (pixAv == 4){
     strings = 912;
   }
  String[] files = new String[strings];  

  /////////////// check to make sure something is selected
  boolean check = false;
  for(int w = 0; w<spectra; w++){
    if(selectBox[w] == true){
    check = true;
    println("check is true for spec "+w);
    }
  }
  if(check == false){
    errorFlag = true;
    iError = 3;
  }
  else{       //// save selected files
  file2 = selection.getAbsolutePath();
  boolean xDat = false;           //
  for(int p = 0; p<spectra; p++){   // for each spectrum ...
   if(selectBox[p] == true){        // ... determine if selected
     println("saving spectrum "+p);
  GPointsArray array = RawSpecList.get(p);   // get array
  if(xDat == false){
  nPixels = array.getNPoints();
  files[0] = "wavelength,";
    println("nPixels: "+nPixels);
 }
  files[0] = files[0]+headers[p+1]+",";
  println("line 0: "+files[0]);
for(int point = 1; point<nPixels; point++){
  if(xDat == false){
  try{
    String xPoint =str(array.getX(point-1));
    files[point] = xPoint;
  }
 catch(Exception e){
 println("save error, line 58");
 }
  }
  try{
 files[point] = files[point]+","+ str(array.getY(point));
 //println(files[point]);
 if(point > nPixels*0.97){
println("files "+point+": "+files[point]);
}
  } catch(Exception e){
    println("save error, line 68");
  }
 }   // end of selectBox loop
  xDat = true;
 println("saved spectra "+p);
 }   // end of spectra loop

}    // end of pixels loop
println("file: "+file2);
 saveStrings(file2,files);  // file2 from  save method in buttons tab
}    // end of else loop
}



void LoadFile(){
  println("load file button pressed");
     selectInput("Select a data file:", "fileToLoad");

}

void fileToLoad(File selection) {
  if (selection == null) {
    println("Window was closed or the user hit cancel.");
  } 
  else {

    String[] file2str = loadStrings(selection.getAbsolutePath());    // load file
    headers = split(file2str[0],',');  // split the first line
    println("first header: "+ headers[0]);
    String[] sText = split(file2str[1],','); 
    nSpectra = sText.length-1;
    for(int h = 0; h<nSpectra; h++){
       RawSpecList.add(new GPointsArray()); 
    }
    if(nSpectra+spectra >20){
    iError = 2;  // error message
    errorFlag = true;
    } else {
          print("display spectra: ");
for (int i=spectra; i<nSpectra+spectra; i++){  // set all new spectra visible
  selectBox[i] = true;
  print(i+", ");
}
println("");
    println("number of spectra to add: "+nSpectra);
    nPixels = file2str.length;
    println("data size: "+nPixels);  //1400
    
         ///  for each spectrum ///
    for (int p = spectra; p< spectra+nSpectra; p++){
     sFileName[p] = headers[p+1];
     println("File name "+p+": "+sFileName[p]);
     cHeader[p] = sFileName[p];  // file name for processed data, change later
     println("line 137");
     GPointsArray array = RawSpecList.get(p+spectra);  // spectra the old number?

     yMinRaw[p+spectra] = 0;
     yMaxRaw[p+spectra] = 0;
          println("line 142");
          
          /// for each wavelength  ///
         for (int j = 0; j<nPixels-2; j++){

       String[] tokens = split(file2str[j+1],',');
           if(tokens[p] == null){
             println("empty thing");
           }
           else{
             
  float xValue = float(tokens[0]);
  float yValue = float(tokens[p+1]);
  // set limits
  try{
      if(j==0){  // this buggers stuff up.
               xMin[0] = xValue;
               sMin = str(xValue);
               println("xMin: "+xMin[0]+", "+sMin);
             }
             if(j==nPixels-3){
               xMax[0] = xValue;
               sMin = str(xValue);
             }
      if(yValue < yMinRaw[p+spectra]){
      yMinRaw[p+spectra] = yValue;
      }
      if(yValue > yMaxRaw[p+spectra]){
     yMaxRaw[p+spectra] = yValue*1.15;
      }
  } catch(Exception e){
    println("fucked up setting limits, line 172, spectrum "+p+", pixel "+j);
  }
      try{
  array.add(xValue,yValue);
      }
      catch(Exception e){
        println("fucked up line 172");
      }
     }

    }  // end of pixels loop
    println("size of data set "+p+": "+ array.getNPoints());
     println("Min Y: "+yMinRaw[p+spectra]+", Max Y: "+yMaxRaw[p+spectra]);
    }
  spectra += nSpectra;                // add new spectra to list 
  println("spectra: "+spectra);
  }
  }
  getYLims();
  findCursor();
}
