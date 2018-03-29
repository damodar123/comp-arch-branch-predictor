#include "predictor.h"
#include <cmath>


#define PHT_CTR_MAX  3
//chuan: for tournament predictor
#define TOURNAMENT_CTR_MAX 3
#define PHT_CTR_INIT 2

#define HIST_LEN   16
#define TOUR_LEN   16
#define BHT_BIT_SIZE 11
#define BHT_HIST_LENGTH 16
#define PHT_LOCAL_CTR_INIT 2
#define PHT_LOCAL_CTR_MAX  3
#define UINT16      unsigned short int


/////////////// STORAGE BUDGET JUSTIFICATION ////////////////
// Total storage budget: 52KB + 32 bits

// Total PHT counters for Global predictor: 2^16
// Total PHT size for global predictor = 2^16 * 2 bits/counter = 2^17 bits = 16KB
// GHR size for global predictor: 32 bits

// Total PHT counters for local predictor: 2// Total PHT size for local predictor = 2^16 * 2 bits/counter = 2^17 bits = 16KB
// Total BHT size for local predictor = 2^11 * 16 bits/counter = 2^15 bits = 4KB
// Total Size for local predictor = 16KB + 4KB = 20KB

// Total Tournament counters is: 2^16
// Total Tournament counter's size = 2^16 * 2 bits/counter = 2^17 bits = 16KB
/////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

//float weights[32] = {1.0};
//float bias = 1.0;
//float thresold = 76;
//float sum = 0.0;
int pcbits=256;
int inputsize=32;
float thresold=76;
float perceptrons[256][33];
 
float sum=0.0;
PREDICTOR::PREDICTOR(void)
{
  ghr              = 0;
//  float perceptrons[pcbits][inputsize];
  for(int i=0;i<pcbits;i++)
  {
     for(int j=0;j<inputsize;j++)
     {

         perceptrons[i][j]=1.0;
     }


   }

}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

bool   PREDICTOR::GetPrediction(UINT32 PC){
	UINT32 ghr_bit;
    sum = 0.0;
    UINT32 pCC   = PC % (251);
//    printf("%d",pCC);
	for(int i=0; i<inputsize; i++)
	{
		ghr_bit = ((ghr << (31-i)) >> 31);
        //printf("%d\n",ghr_bit);
		sum += perceptrons[pCC][i]* (ghr_bit ? 1:-1);
	}
	sum += perceptrons[pCC][32];

	if(sum > 0)
	{
        return TAKEN;
    }
	else
	{
        return NOT_TAKEN;
	}
}


/////////////////////////////////////////////////////////////

void  PREDICTOR::UpdatePredictor(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget)
{
	UINT32 ghr_bit;	
     UINT32 pCC   = PC % (251);
//    printf("%d",pCC);
	if( abs(sum) <= thresold || (resolveDir != predDir))
	{
		for(int i=0; i<32; i++)
		{
			ghr_bit = ((ghr << (31-i)) >> 31);
			perceptrons[pCC][i] += (resolveDir ? 1 : -1) * (ghr_bit ? 1: -1);
		}
        perceptrons[pCC][32]+=(resolveDir?1:-1);
	}
	
  	ghr = (ghr << 1);

	if(resolveDir == TAKEN){
    	ghr++;
  	}

}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

void    PREDICTOR::TrackOtherInst(UINT32 PC, OpType opType, UINT32 branchTarget){

  // This function is called for instructions which are not
  // conditional branches, just in case someone decides to design
  // a predictor that uses information from such instructions.
  // We expect most contestants to leave this function untouched.

  return;
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
