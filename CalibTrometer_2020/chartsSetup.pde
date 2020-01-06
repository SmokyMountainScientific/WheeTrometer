//chartsSetup tab, Calibtrometer_2020
// methods: setupCharts
//     setAxes
//     addRawChart
//     addProcessedChart
//     setupRawPlots

void setupCharts(){

 ////////////////////////////////grafica uses GPlots
  plot1 = new GPlot(this);
  plot1.setPos(plotX, plotY);
  plot1.setDim(plotWidth, plotHeight);
  plot1.setXLim(xMin[0], xMax[0]);
  print(wavelength);
  plot1.setYLim(0, 800);
  plot1.setPoints(data);
  for(int i = 0; i<768; i++){
  data.add(i,i);
  }
  
}/////////////////end setupChart/////

void setAxes(){
  String[] title = {"Raw Data","Absorbance Data","Absorbance Data"};
  String[] yLabelTxt = {"Intensity","Absorbance","Absorbance"};
  plot1.getXAxis().getAxisLabel().setText("Wavelength (nm)");
  plot1.getYAxis().getAxisLabel().setText(yLabelTxt[viewVal]);
  plot1.getTitle().setText(title[viewVal]);
  plot1.setXLim(xMin[viewVal],xMax[viewVal]);
  plot1.setYLim(yMin[viewVal],yMax[viewVal]);
  }
  
