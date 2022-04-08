#include <stdio.h> 
#include <stdlib.h> 
#include <limits.h> 
#include <omp.h>
#include "papi.h" 

//Structure of a bucket with a dynamic allocated array to save memory. 
typedef struct {
    int *array;
    size_t used;
    size_t size;
}Bucket;

//Function that initializes an array 
void initBucket(Bucket *a,size_t initialSize){
    a->array=malloc(initialSize * sizeof(int));
    a->used = 0;
    a->size = initialSize;
}

//Function that inserts an element in a bucket, resizing the bucket if necessary 
void insertBucket(Bucket *a, int element) {
  if (a->used == a->size) {
    a->size *= 2;
    a->array = realloc(a->array, a->size * sizeof(int));
  }
  a->array[a->used++] = element;
}

//Function that frees a bucket 
void freeBucket(Bucket *a) {
  free(a->array);
  a->array = NULL;
  a->used = a->size = 0;
}

//Function that compare two integers. 
static int intCompare(const void *p1, const void *p2){
    int int_a = * ( (int*) p1 );
    int int_b = * ( (int*) p2 );
    if ( int_a == int_b ) return 0;
    else if ( int_a < int_b ) return -1;
    else return 1;
}

//Function that sorts a bucket using qsort 
void sortBucket (Bucket *a){
    qsort(a->array,a->used,sizeof(int), intCompare);
}

//Function that applies the Bucket Sort. 
int bucketSort(int* array,int* arrayOut,int tamArray,int nBuckets,int nThreads){
    int i,j;
    //Array with a bucket in each position.
    Bucket buckets [nBuckets];
    //Init space for every bucket.
    int tamInicial = tamArray/nBuckets;
    //Initializing every bucket int the array
    for(i=0;i<nBuckets;i++){
        initBucket(&(buckets[i]),tamInicial);
        //Print para debbug
        //int t = omp_get_thread_num();
        //printf("Thread n=%d Initializing bucket n=%d\n",t,i);
    }
    //Calculating the max and min element of the array
    int max = array[0];
    int min = array[0];
    for(i=0;i<tamArray;i++){
        if (array[i]>max) max=array[i];
        if (array[i]<min) min=array[i];
    }
    //Debbug:
    //printf("Max:%d Min:%d\n",max,min);
    //Check if range gives overflow:
    if(max > INT_MAX + min){
        printf("Overflow detected - range too high");
        return -1;
    }
    //Insert elements in the buckets
    int range = (max-min)/ (nBuckets - 1);
    if(range==0){
	printf("Too Many Buckets");
        return -1;
    }
    int elem;
    int indiceBucket;
    for(i=0;i<tamArray;i++){
        elem = array[i];
        indiceBucket = (elem-min)/range;
        insertBucket(&(buckets[indiceBucket]),elem);
    }
    //Sort each bucket
    #pragma omp parallel for num_threads(nThreads) schedule(dynamic,1)
    for(i=0;i<nBuckets;i++){
        sortBucket(&(buckets[i]));
        //debbug
        //int t = omp_get_thread_num();
        //printf("Thread n=%d Sorting bucket number n=%d\n",t,i);
    }
/*
    //Save in array how many elements all the buckets before i have,so next step can be parallellized
    int arrayUsed [nBuckets + 1];
    arrayUsed[0]=0;
    for(i=1;i<nBuckets + 1;i++){
        arrayUsed[i]=arrayUsed[i - 1] + ((&(buckets[i - 1]))->used);
        printf("%d ",arrayUsed[i]);
    }
    //Put the elements in each bucket to the array
    int add=0;
    for(i=0;i<nBuckets;i++){
        add=0;
        for(j=arrayUsed[i];j<arrayUsed[i + 1];j++){
            elem = ((&(buckets[i]))->array)[add++];
            array[j]=elem;
        }
    }
*/
    int index=0;
    for(i=0;i<nBuckets;i++){
	for(j=0;j<((&(buckets[i]))->used);j++){
		arrayOut[index++]=((&(buckets[i]))->array)[j];
		}
    }

    //Free each bucket
    for(i=0;i<nBuckets;i++){
        freeBucket(&(buckets[i]));
        //debbug
        //int t = omp_get_thread_num();
        //printf("\nThread n=%d Freeing bucket n=%d",t,i);
    }
    return 0;
}

int gerarArrayTeste(int *array,int tamArray,int min,int max){
    int range=max-min;
    int i;
    for(i=0;i<tamArray;i++){
        array[i]=(rand()%(range))+min;
    }
   return 0;
}

int printArray(int *array,int tamArray){
    int i;
    printf("\n[");
    for(i=0;i<tamArray-1;i++){
        printf("%d,",array[i]);
    }
    printf("%d]\n",array[tamArray-1]);
    return 0;
}

#define NUM_EVENTS 4
int Events[NUM_EVENTS]={PAPI_TOT_CYC,PAPI_TOT_INS,PAPI_L1_DCM,PAPI_L2_DCM}; 
long long values[NUM_EVENTS],min_values[NUM_EVENTS];
#define NUM_RUNS 5

int main(int argc,char *argv[]){
    int nThreads=atoi(argv[1]); 		//Number of Threads 
    int tamArray=atoi(argv[2]);			//Size of the array we want to sort
    int nBuckets=atoi(argv[3]);			//Size of buckets
    int * array = malloc(tamArray*sizeof(int)); //Allocate space to the array
    int * arrayOut=malloc(tamArray*sizeof(int));    

    printf("Using nThreads=%d,tamArray=%d,nBuckets=%d\n",nThreads,tamArray,nBuckets);

    long long start_usec,end_usec,elapsed_usec,min_usec=0L;
    int num_hwcntrs=0;
    
    int i,run;
    //Setting up Papi
    PAPI_library_init (PAPI_VER_CURRENT);
    if ((num_hwcntrs=PAPI_num_counters())<=PAPI_OK){
	fprintf(stderr,"PAPI error getting number of available hardware counters!\n");
	return 0;
    }
    if(num_hwcntrs>=NUM_EVENTS){
	num_hwcntrs=NUM_EVENTS;
    }else{
	fprintf(stderr,"Error: there aren't enough counters to monitor %d events!\n",NUM_EVENTS);
	return 0;
    }

    gerarArrayTeste(array,tamArray,-10000,10000);

    //Running N simulations
    for(run=0;run<NUM_RUNS;run++){
	fprintf (stdout,"\nrun=%d - Computing",run);
	start_usec = PAPI_get_real_usec();
	//Starting Papi Counters
        if(PAPI_start_counters(Events,num_hwcntrs)!=PAPI_OK){
	  fprintf(stderr,"PAPI Error starting counters!\n");
	  return 0;
	}
	//Sort the array using the algorithm
	bucketSort(array,arrayOut,tamArray,nBuckets,nThreads);
        //Stopping Papi counters
	if(PAPI_stop_counters(values,NUM_EVENTS)!=PAPI_OK){
	  fprintf(stderr,"PAPI Error stopping counters!\n");
	  return 0;
	}
	end_usec=PAPI_get_real_usec();
	fprintf(stdout,"Done");
	elapsed_usec= end_usec - start_usec;
        //Updating values if time is better
	if ((run==0)||(elapsed_usec<min_usec)){
	 min_usec=elapsed_usec;
	 for(int i=0;i<NUM_EVENTS;i++) min_values[i]=values[i];
	}
    }//End of For

    printf("\nWall clock time:%lld usecs\n",min_usec);
    for(i=0;i<NUM_EVENTS;i++){
	char EventCodeStr[PAPI_MAX_STR_LEN];
	if (PAPI_event_code_to_name(Events[i],EventCodeStr)==PAPI_OK){
	  fprintf(stdout,"%s=%lld\n",EventCodeStr,min_values[i]);
	}else{
	  fprintf(stdout,"PAPI Unknown event=%lld\n",min_values[i]);
	}
    }
    if((Events[0]==PAPI_TOT_CYC) && (Events[1]==PAPI_TOT_INS)){
	float CPI = ((float)min_values[0] / (float)min_values[1]);
	fprintf(stdout,"CPI=%.2f\n",CPI);
    }
    //Print array for debbug
    //printArray(array,tamArray);
    return 0;
}
