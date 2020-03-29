#include <cstdio>
#include <cstring>
#include <cstdlib>

#include <iostream>
#include <fstream>

#include <map>
#include <vector>
#include <string>

using namespace std;

// Conversion modes
#define OUTCACTUS 1
#define CSVTOCACTUS 2

#define OUTCLICK 3
#define CSVTOCLICK 4
#define CLICKTOROCK 5
#define CONFUSION 6

#define MAXCLUSTERS 1000

int main(int argc, char *argv[]) 
{ 

  int mode;

  int   numItems, id, i, j, k;
  int*  items;

  char buffer[1024];

  ifstream ifile;
  ifstream ifile2;
  ifstream ifile3;
  ofstream ofile;
  ofstream ofile2;

  int currentColumn, outColumn, totalColumns, recordID, outColumns, outValue;
  int labelColumn, numberOfTuples;

  int common, total;

  vector<int> ignoredColumns;
  vector<int>::const_iterator columnIt;
  map<string,int>* attrMap;
  map<string,int>::iterator attrMapIt;

  map<string,int> labelMap;
  int highestLabel;
  vector<int> highestOutValue;
  vector<int> columnOffset;

  map<int, int> confusionMap;
  int mapFrom, mapTo;
  int confusionMatrix[MAXCLUSTERS][MAXCLUSTERS];

  int cluster1, cluster2;  
  string entry;

  int maxDim1, maxDim2;
  int position;
  string tokenizer;
  int clusterNumber;

  int ignore_st= 0;
  bool use_ascii = false;
  
  vector<int> clusters1;
  vector<int> clusters2;
    
  int* tuples;

  if(argc < 2){
    printf("convert: You must give a conversion mode.\n");
    return(1);
  }


  if(!strcmp(argv[1], "OUTCACTUS"))
    mode = OUTCACTUS;
  else if(!strcmp(argv[1], "CSVTOCACTUS"))
    mode = CSVTOCACTUS;
  else if(!strcmp(argv[1], "CSVTOCLICK"))
    mode = CSVTOCLICK;
  else if(!strcmp(argv[1], "CLICKTOROCK"))
    mode = CLICKTOROCK;
  else if(!strcmp(argv[1], "CONFUSION"))
    mode = CONFUSION;
  else if(!strcmp(argv[1], "OUTCLICK"))
    mode = OUTCLICK;
  else{
    printf("convert: I don't know that mode you specified.\n");
    return(1);
  }
  
  ifile.open(argv[2]);
  
  if(!ifile){
    printf("convert: Cannot read input file.\n");
    return(1);
  }
  
  switch(mode){
  case OUTCACTUS:
    items = NULL;

    while(ifile.read((char*)&id, sizeof(int))){
      printf("%d ", id);
      ifile.read((char*)&numItems, sizeof(int));
      if(!items){
	items = new int[numItems];
      }
      
      ifile.read((char*)items, numItems*sizeof(int));
      for(i = 0; i < numItems; ++i){
	printf("%d ", items[i]);
      }
      printf("\n");
 
      // EOL
      ifile.read((char*)&id, sizeof(int));
    }
    
    if(items)
      delete items;

    break;
  case OUTCLICK:
    items = NULL;

    ifile.read((char*)&numberOfTuples, sizeof(int));
    printf("Total %d tuples in the file, ", numberOfTuples);
    
    ifile.read((char*)&numItems, sizeof(int));
    printf("with %d attributes.\n", numItems);
    
    items = new int[numItems];
    ifile.read((char*)items, numItems*sizeof(int));
    printf("Distinct attribute values are\n");
    for(i = 0; i < numItems; i++){
      printf("A%d (%d), ", i+1, items[i]);
    }
    printf("\n");
    delete items;
    
    items = new int[numItems + 3];

    while(ifile.read((char*)items, (numItems+3)*sizeof(int))){
      for(i = 0; i < numItems + 2; ++i){
	printf("%d ", items[i]);
      }
      printf("\n");
    }
    
    break;
  case CSVTOCACTUS:
    if(argc < 4){
      printf("convert: You need to specify the number of columns in the input file\n");
      printf("convert: CSVTOCACTUS <file> <totalColumns> [ascii] {ignoredColumn}*\n");
      exit(1);
    }
    
    use_ascii = false;
    totalColumns = atoi(argv[3]);
    ignore_st = 4;
    if (strcmp(argv[4], "ascii") == 0){
       ignore_st = 5;
       use_ascii = true;
    }
    
    for(i = ignore_st; i < argc; i++){
      ignoredColumns.push_back(atoi(argv[i]));
      // printf("Ignoring %d\n", atoi(argv[i]));
    }

    recordID = 1;
    outColumns = totalColumns - ignoredColumns.size();
    attrMap = new map<string,int>[outColumns];

    // printf("OutColumns = %d\n", outColumns);

    highestOutValue.assign(outColumns, 0);

    while(!ifile.eof()){
      currentColumn = 1;
      outColumn = 0;
      buffer[0] = '\0';

      if (use_ascii) cout << recordID << " ";
      else cout.write((char*)&recordID, sizeof(int));
      recordID++;
      if (use_ascii) cout << outColumns << " ";
      else cout.write((char*)&outColumns, sizeof(int));
      while(outColumn < outColumns){
	ifile >> buffer;
	//printf("'%s' is current column %d, out column %d\n", buffer, currentColumn, outColumn);
	if(buffer[0] == '\n' || buffer[0] == ','){
	}
	else{
	  // Check if this column is blocked
	  for(columnIt = ignoredColumns.begin(); columnIt != ignoredColumns.end(); columnIt++){
	    if(currentColumn == *columnIt)
	      break;
	  }

	  if(columnIt == ignoredColumns.end()){
	    entry = buffer;
	    outValue = attrMap[outColumn][entry];
   
	    if(outValue == 0){
	      outValue = ++highestOutValue[outColumn];
	      attrMap[outColumn][entry] = outValue;
	    }
	    
	    // outValue += outColumn * 100;
            if (use_ascii) cout << outValue << " ";
            else cout.write((char*)&outValue, sizeof(int));
	    
	    outColumn++;
	  }
	  currentColumn++;
	}
      }

      outValue = -1;
      if (use_ascii) cout << outValue << endl;
      else cout.write((char*)&outValue, sizeof(int));
    }
    delete attrMap;
    break;

  case CSVTOCLICK:
    if(argc < 7){
      printf("convert: You need to specify the number of columns in the input file and the label column\n");
      printf("convert: CSVTOCACTUS <sourcefile> <confusionfile> <mappingfile> <totalColumns> <label column> [ascii] {ignoredColumn}*\n");
      exit(1);
    }

    ofile.open(argv[3]);
    ofile2.open(argv[4]);

    if(!ofile.is_open() || !ofile2.is_open()){
      cout << "convert: The confusion file or mapping file could nout be opened." << endl;
      exit(1);
    }

    totalColumns = atoi(argv[5]);
    labelColumn = atoi(argv[6]);

    use_ascii = false;
    ignore_st = 7;
    if (strcmp(argv[7], "ascii") == 0){
       ignore_st = 8;
       use_ascii = true;
    }

    for(i = ignore_st; i < argc; i++){
      ignoredColumns.push_back(atoi(argv[i]));
    }

    outColumns = totalColumns - ignoredColumns.size();
    attrMap = new map<string,int>[outColumns];
    highestOutValue.assign(outColumns, 0);
    columnOffset.assign(outColumns, 0);
    highestLabel = 0;
    numberOfTuples = 0;

    // First pass: Compute attribute -> value mapping and count the number of
    // tuples
    while(!ifile.eof()){
      currentColumn = 0;
      outColumn = 0;
      buffer[0] = '\0';

      while(outColumn < outColumns){
	ifile >> buffer;
	entry = buffer;

	if(currentColumn == labelColumn){
	  if(labelMap[entry] == 0)
	    labelMap[entry] = ++highestLabel;
	}

	if(buffer[0] == '\n' || buffer[0] == ','){
	}
	else{
	  // Check if this column is blocked
	  for(columnIt = ignoredColumns.begin(); columnIt != ignoredColumns.end(); columnIt++){
	    if(currentColumn == *columnIt)
	      break;
	  }

	  if(columnIt == ignoredColumns.end()){
	    outValue = attrMap[outColumn][entry];
	    if(outValue == 0){
	      outValue = ++highestOutValue[outColumn];
	      attrMap[outColumn][entry] = outValue;
	    }
	    outColumn++;
	  }
	  currentColumn++;
	}
      }
      while(currentColumn < totalColumns){
	ifile >> buffer;
	currentColumn++;
      }
      outValue = -1;
      numberOfTuples++;
    }

    // cout << "Number of tuples " << numberOfTuples << endl;

    ifile2.open(argv[2]);

    if (use_ascii) cout << numberOfTuples << " ";
    else cout.write((char*)&numberOfTuples, sizeof(int));
    if (use_ascii) cout << outColumns << " ";
    else cout.write((char*)&outColumns, sizeof(int));
    for(i = 0; i < outColumns; ++i){
      outValue = highestOutValue[i];
      if (use_ascii) cout << outValue << " ";
      else cout.write((char*)&(outValue), sizeof(int));
    }
    if (use_ascii) cout << endl;

    for(i = 1; i < outColumns; i++){
      columnOffset[i] = columnOffset[i-1] + highestOutValue[i-1];
    }

    i = 0;
    // Second pass: Actual conversion using the maps created before
    while(!ifile2.eof()){
      outColumn = 0;
      currentColumn = 0;
      buffer[0] = '\0';

      if (use_ascii) cout << i << " ";
      else cout.write((char*)&i, sizeof(int));
      if (use_ascii) cout << numberOfTuples << " ";
      else cout.write((char*)&numberOfTuples, sizeof(int));

      // cout << "Writing " << i << endl;

      while(outColumn < outColumns){
	ifile2 >> buffer;
	entry = buffer;

	if(currentColumn == labelColumn){
	  // cout << "Label is " << labelMap[entry] - 1 << endl;
	  ofile << labelMap[entry] - 1 << endl;
	}

	if(buffer[0] == '\n' || buffer[0] == ','){
	}
	else{
	  // Check if this column is blocked
	  for(columnIt = ignoredColumns.begin(); columnIt != ignoredColumns.end(); columnIt++){
	    if(currentColumn == *columnIt)
	      break;
	  }

	  
	  if(columnIt == ignoredColumns.end()){
	    outValue = columnOffset[outColumn] + attrMap[outColumn][entry] - 1;
	    // cout << "OV in " << outColumn << " = " << outValue << endl;
	    if (use_ascii) cout << outValue << " ";
            else cout.write((char*)&outValue, sizeof(int));
	    // cout << "DONE" << endl;
	    outColumn++;
	  }
	  currentColumn++;
	}
      }
      while(currentColumn < totalColumns){
	ifile2 >> buffer;
	currentColumn++;
      }
      if (use_ascii) cout << i << endl;
      else cout.write((char*)&i, sizeof(int));

      outValue = -1;
      i++;
    }

    // Finally, create the mapping file so we know what the original attribute
    // values were
    //for(attrMapIt = labelMap.begin(); attrMapIt != labelMap.end(); attrMapIt++){
    //  ofile2 << "Label " << ": '" << attrMapIt->first << "' = " << attrMapIt->second << endl;
    //}

    for(i = 0; i < outColumns; i++){
      ofile2 << "*A* " << i << endl;

      for(attrMapIt = attrMap[i].begin(); attrMapIt != attrMap[i].end(); attrMapIt++){
	if(attrMapIt->first != ""){
	  ofile2 << attrMapIt->first << " " << attrMapIt->second << endl;
	}
	else{
	  ofile2 << "- " << attrMapIt->second << endl;
	}
      }
    }

    ifile2.close();
    ofile.close();
    delete[] attrMap;

    break;
  case CLICKTOROCK:
    if(argc < 4){
      printf("convert: You need to specify the number of values you want in the output file\n");
      printf("convert: CLICKTOROCK <sourcefile> <itemlimit>\n");
      exit(1);
    }

    items = NULL;

    ifile.read((char*)&numberOfTuples, sizeof(int));
    
    ifile.read((char*)&numItems, sizeof(int));
    
    items = new int[numItems];
    ifile.read((char*)items, numItems*sizeof(int));
    delete items;    
    items = new int[numItems + 3];

    int effectiveTuples;
    effectiveTuples = atoi(argv[3]) < numberOfTuples ? atoi(argv[3]) : numberOfTuples;

    tuples = new int[effectiveTuples * (numItems + 3)];
    ifile.read((char*)tuples, effectiveTuples * (numItems+3)*sizeof(int));
      
    cout << "1 " << effectiveTuples << " 1" << endl;
    for(i = 0; i < effectiveTuples - 1; i++){
      for(j = i+1; j < effectiveTuples; j++){
	common = total = 0;
	for(k = 2; k < numItems + 2; k++){
	  if(tuples[i * (numItems+3) + k] == tuples[j * (numItems + 3) + k]){
	    common++;
	    total++;
	  }
	  else{
	    total += 2;
	  }
	}
	
	if(total != 0){
	  cout << (double)common / double(total) << " " << i+1 << " " << j+1 << endl;
	}
	else{
	  cout << 0.0 << " " << i+1 << " " << j+1 << endl;
	}
      }
    }
    
   
    break;

  case CONFUSION:
  
    if(argc < 5){
      printf("convert: You need to give two confusion files and a mapping file.\n");
      printf("convert: CONFUSION <confusion1> <confusion2> <mapping file for 1>\n");
      exit(1);
    }

    ifile2.open(argv[3]);
    if(!ifile2.is_open()){
      printf("Could not open confusion file '%s'\n", argv[3]);
      ifile.close();
      exit(1);
    }

    ifile3.open(argv[4]);
    if(!ifile3.is_open()){
      printf("Could not open mapping file '%s'\n", argv[4]);
      ifile.close();
      ifile2.close();
      exit(1);
    }

    // First read the cluster number mapping
    while(!ifile3.eof()){
      ifile3 >> buffer;
      mapFrom = atoi(buffer);
      ifile3 >> buffer;
      mapTo = atoi(buffer);
      
      if(mapFrom == mapTo)
	continue;
  
      //cout << "convert: Mapping " << mapFrom << " to " << mapTo << endl;

      // Confusion files start counting at -1 (for "no cluster") and we need the
      // 0 as reserved value for "no mapping specified"
      confusionMap[mapFrom + 2] = mapTo + 2;

    }
    ifile3.close();


    numberOfTuples = 0;
    maxDim1 = maxDim2 = 0;

    while(!ifile.eof()){
      if(ifile2.eof()){
	printf("Warning: Confusion files have different length\n");
	break;
      }
      
      tokenizer.clear();
      clusters1.clear();
      ifile >> tokenizer;

      if(tokenizer.empty())
	break;

      while(!tokenizer.empty()){
        if((position = tokenizer.find_first_of(",")) >= 0){
	  clusterNumber = atoi(tokenizer.substr(0, position).c_str()) + 2;
	  tokenizer.erase(0, position + 1);
	}
	else{
	  clusterNumber = atoi(tokenizer.c_str()) + 2;
	  tokenizer.clear();
	}
	if(confusionMap[clusterNumber] != 0)
	  clusterNumber = confusionMap[clusterNumber];
	
	clusters1.push_back(clusterNumber);
	
	if(clusterNumber > maxDim1)
	  maxDim1 = clusterNumber;
      }

      tokenizer.clear();
      clusters2.clear();
      ifile2 >> tokenizer;

      while(!tokenizer.empty()){
        if((position = tokenizer.find_first_of(",")) >= 0){
	  clusterNumber = atoi(tokenizer.substr(0, position).c_str()) + 2;
	  tokenizer.erase(0, position + 1);
	}
	else{
	  clusterNumber = atoi(tokenizer.c_str()) + 2;
	  tokenizer.clear();
	}
	if(confusionMap[clusterNumber] != 0)
	  clusterNumber = confusionMap[clusterNumber];
	
	clusters2.push_back(clusterNumber);
	
	if(clusterNumber > maxDim2)
	  maxDim2 = clusterNumber;
      }
 
      numberOfTuples++;

      // See if there's a case where the actual class and the predicted class
      // match. If not so, take the first (actual, predicted) pair and add it to the
      // confusion matrix

      cluster1 = -1;
      cluster2 = -1;
      for(i = 0; i < clusters1.size(); i++){
	for(j = 0; j < clusters2.size(); j++){
	  if(clusters1[i] == clusters2[j]){
	    cluster1 = clusters1[i];
	    cluster2 = clusters2[j];
	    break;
	  }
	}
      }

      if(cluster1 == -1 && cluster2 == -1){
	cluster1 = clusters1[0];
	cluster2 = clusters2[0];
      }

      if(cluster1 >= MAXCLUSTERS || cluster2 >= MAXCLUSTERS){
	cout << "Warning: The maximal size of the confusion matrix is " << MAXCLUSTERS << "^2." << endl;
	continue;
      }

      confusionMatrix[cluster1][cluster2]++;
    }

    ifile2.close();

    if(numberOfTuples == 0){
      cout << "No tuples in the confusion files.\n" << endl;
      exit(1);
    }

    cout << "\\begin{table}" << endl;
    cout << "  \\begin{center}" << endl;
    cout << "    \\begin{tabular}";

    for(j = 1; j <= maxDim2+1; j++){
      if(j == 1){
	cout << "{c";
      }
      else if(j == maxDim2+1){
	cout << "|c}";
      }
      else{
	cout << "|c";
      }
    }
    cout << endl;
   
    for(j = 1; j<= maxDim2; j++){
      if(j == 1){
	cout << " & None ";
      }
      else{
	cout << " & $C_" << j-1 << "$";
      }
    }
    cout << " \\\\" << endl << "\\hline" << endl;


    for(i = 1; i <= maxDim1; i++){
      if(i == 1){
	cout << " None ";
      }
      else{
	cout << "$C_" << i-1 << "$ ";
      }
      for(j = 1; j <= maxDim2; j++){
	printf("& %.1f\\%% ", 100 * (double)confusionMatrix[i][j] / (double)numberOfTuples);
      }
      if(i < maxDim1){
	cout << "\\\\" << endl << "\\hline" << endl;
      }
      else{
	cout << endl;
      }
    }

    cout << "    \\end{tabular}" << endl;
    cout << "  \\end{center}" << endl;
    cout << "  \\caption{Confusion Matrix}" << endl;
    cout << "\\end{table}" << endl;
    
    

    break;
 
  default:
    break;
  }

  ifile.close();

  return 0;

}

