/*
William L Lopez
101899002

CS-341L fall 2023
Cache Simulator
*/

#include "cachelab.h"
#include <stdio.h>
#include <stdlib.h>

struct line {
  int validBit;
  long tag;
  int lastUsed;
};


struct memTrace {
  char operation;
  unsigned long  address;
  int size;
};

/*
A cache can be represented by a 2d array of the line structure. Where cacehe[i] represents the sets,
and cache[i][j] represends the lines in each set.
*/
int main(int argc, char *argv[])
{
  struct line **cache;
  struct memTrace *traces;
  int hit_count = 0; int miss_count = 0; int eviction_count = 0;
  int siBits = 0; int associ = 0; int blockBits = 0;
  int setCount = 0;
  char *traceFilePath; int verbose = 0; int helpFlag = 0;
  FILE *traceFile;
  
  for(int i = 0; i < argc; i++){
    if (argv[i][0] == '-'){
      switch (argv[i][1]){
      case 's':
	i++;
	sscanf(argv[i], "%d", &siBits);
	setCount = 1 << siBits;
	break;
      case 'E':
	i++;
	sscanf(argv[i], "%d", &associ);
	break;
      case 'b':
	i++;
	sscanf(argv[i], "%d", &blockBits);
	break;
      case 't':
	i++;
	traceFilePath = argv[i];
	break;
      case 'h':
	helpFlag = 1;
	break;
      case 'v':
	verbose = 1;
	break;
      }
    }
  }

  //Info from help flag
  if (helpFlag){
    printf("Options:\n");
    printf("  -h         Print this help message.\n");
    printf("  -v         Optional verbose flag.\n");
    printf("  -s <num>   Number of set index bits.\n");
    printf("  -E <num>   Number of lines per set.\n");
    printf("  -b <num>   Number of block offset bits.\n");
    printf("  -t <file>  Trace file.\n");
  }
    
  // Initializing the cache
  cache = malloc(sizeof(struct line*) * setCount);
  for (int i = 0; i < setCount; i++){
    cache[i] = malloc(sizeof(struct line) *  associ);
    for (int j = 0; j < associ; j++){
      cache[i][j].validBit = 0;
      cache[i][j].tag = 0xffffffff;
      cache[i][j].lastUsed = 0;
    }
  }

  // Reads the trace file and sets up the traces array.
  // First reads how many lines are in the tracefile.
  // Done to corrently allocate memory;
  int numTraces = 0;
  char c;
  traceFile = fopen(traceFilePath,"r");
  if (traceFile == NULL){
    printf("Tracefile not found.");
  }
  do{
    c = fgetc(traceFile);
    if (c == '\n'){
      numTraces++;
    }
  }
  while (c != EOF);
  fclose(traceFile);
  // Now that we know the number of traces we can allocate traces.
  traces = malloc(sizeof(struct memTrace) * numTraces);
  
  //Reads the contents of the file and puts values into the traces array.
  c = ' ';
  traceFile = fopen(traceFilePath,"r");
  int readTraceIndex = 0;
  int stri = 1;
  char strline[100];
  do{
    c = fgetc(traceFile);
    strline[stri] = c;
    stri++;
    if (c == '\n'){
      if (strline[0] != 'I'){
	struct memTrace newTrace;
	char tempop;
	char eatComma;
	long  tempad;
	int tempsize;
	
	sscanf(strline, " %c %lx%c%d", &tempop, &tempad, &eatComma, &tempsize);
	newTrace.operation = tempop;
	newTrace.address = tempad;
	newTrace.size = tempsize;
	
	traces[readTraceIndex] = newTrace;
	readTraceIndex++;
      }
      stri = 0;
    }
  } while (c != EOF);
  fclose(traceFile);
  
  // Old num traces is just the number of lines in the file.
  // We need to change it to the number of traces without
  // I operations.
  numTraces = readTraceIndex;
    
  // Start if simulation.
  for (int i = 0; i < numTraces; i++){
    if (verbose){
      printf("%c %lx,%d ", traces[i].operation, traces[i].address, traces[i].size);
    }
    int hitBool = 0;
    unsigned long tag = traces[i].address >> (siBits + blockBits);
    unsigned long set = traces[i].address >> blockBits;
    set = set & (~(-1 << siBits));
    
    // Search for a matching tag.
    for (int j = 0; j < associ; j++){
      if (cache[set][j].validBit == 1 && cache[set][j].tag == tag){
	// The cache hits. Twice if it's a modify operation.
	if (verbose){
	  printf("hit ");
	}
	hit_count++;
	if (traces[i].operation == 'M'){
	  if (verbose){
	    printf("hit");
	  }
	  hit_count++;
	}
	hitBool = 1;

	// Update the lastUsed for the line.
	cache[set][j].lastUsed = i;
	break;
      }
    };

    // If there was a hit we skip the rest of the code.
    if (hitBool){
      if (verbose){
	printf("\n");
      }
      continue;
    }
      
    // No matching tag was found, it missed.
    if (verbose){
      printf("miss ");
    }
    miss_count++;
    
    // Still hits of operation is M
    if(traces[i].operation == 'M'){
      if (verbose){
	printf("hit ");
      }
      hit_count++;
    }

    // Finds the least recently used line
    int lru = 0;
    for (int k = 0; k < associ; k++){
      if (cache[set][k].lastUsed < cache[set][lru].lastUsed){
	lru = k;
      }
    }

    // Checks if an eviction will happen.
    if (cache[set][lru].validBit == 1){
      eviction_count++;
      if (verbose){
	printf("eviction");
      }
    }

    if (verbose){
      printf("\n");
    }
    cache[set][lru].tag = tag;
    cache[set][lru].validBit = 1;
    cache[set][lru].lastUsed = i;
  }

  // Free the cache
  for (int i = 0; i < setCount; i++){
    free(cache[i]);
  }
  free(cache);
  free(traces);

  printSummary(hit_count, miss_count, eviction_count);
  return 0;
}
