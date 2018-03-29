#include "predictor.h"
#include <cmath>

#define PHT_CTR_MAX  3
//chuan: for tournament predictor
#define TOURNAMENT_CTR_MAX 3
#define PHT_CTR_INIT 2

#define HIST_LEN   16
#define PERCEPTORS_LEN   8
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

// Total PHT counters for local predictor: 2^16
// Total PHT size for local predictor = 2^16 * 2 bits/counter = 2^17 bits = 16KB
// Total BHT size for local predictor = 2^11 * 16 bits/counter = 2^15 bits = 4KB
// Total Size for local predictor = 16KB + 4KB = 20KB

// Total Tournament counters is: 2^16
// Total Tournament counter's size = 2^16 * 2 bits/counter = 2^17 bits = 16KB
/////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

int pcbits=61;
int inputsize=32;
float thresold=100;
float perceptrons[61][33];
float sum=0.0;

PREDICTOR::PREDICTOR(void){

  historyLength    = HIST_LEN;
  ghr              = 0;
  numPhtEntries    = (1<< HIST_LEN);

  pht = new UINT32[numPhtEntries];


  for(UINT32 ii=0; ii< numPhtEntries; ii++){
    pht[ii]=PHT_CTR_INIT;
  }

  //Initialization for local branch predictor
  bht_history_length = BHT_HIST_LENGTH;
  bht_bit_size = BHT_BIT_SIZE;
  numBhtEntries    = (1<< bht_bit_size);
  bht = new UINT16[numBhtEntries];
  for(UINT32 kk=0; kk< numBhtEntries; kk++){
    bht[kk]=0;
  }

  numPhtLocalEntries = (1<<bht_history_length);
  pht_local = new UINT32[numPhtLocalEntries];
  for(UINT32 ll=0; ll< numPhtLocalEntries; ll++){
    pht_local[ll]=PHT_LOCAL_CTR_INIT;
  }

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
	bool ret;
	UINT32 ghr_bit;
    sum = 0.0;

  //Add for tournament predictor: when 00, 01, use global predictor; when 10, 11, use local predictor
    UINT32 pCC   = PC % (61);

	for(int i=0; i<inputsize; i++)
	{
		ghr_bit = ((ghr << (31-i)) >> 31);
		sum += perceptrons[pCC][i]* (ghr_bit ? 1:-1);
	}
	sum += perceptrons[pCC][32];

	if(sum > 0)
	{
        //use global predictor
      ret =  GetGlobalPrediction(PC);
  } else {
      //use local predictor
      ret = GetLocalPrediction(PC);
  }
	return ret;
}


//for global predictor
bool   PREDICTOR::GetGlobalPrediction(UINT32 PC){
    UINT32 phtIndex   = (PC^ghr) % (numPhtEntries);
    UINT32 phtCounter = pht[phtIndex];
    if(phtCounter > PHT_CTR_MAX/2){
        return TAKEN;
    }else{
        return NOT_TAKEN;
    }
}

//for local predictor
bool   PREDICTOR::GetLocalPrediction(UINT32 PC){
    UINT32 bhtIndex   = (PC >> (32-bht_bit_size));
    UINT16 bht_result = bht[bhtIndex];
    UINT32 pht_local_index = (PC^(UINT32)(bht_result))% (numPhtLocalEntries);

    if(pht_local[pht_local_index] > PHT_LOCAL_CTR_MAX/2){
        return TAKEN;
    }else{
        return NOT_TAKEN;
    }
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

void  PREDICTOR::UpdatePredictor(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget){

  UINT32 ghr_bit;	

  UINT32 phtIndex   = (PC^ghr) % (numPhtEntries);
  UINT32 phtCounter = pht[phtIndex];

  // update the PHT for global predictor
  if(resolveDir == TAKEN){
    pht[phtIndex] = SatIncrement(phtCounter, PHT_CTR_MAX);
  }else{
    pht[phtIndex] = SatDecrement(phtCounter);
  }

  // update the GHR for global predictor
  ghr = (ghr << 1);

  if(resolveDir == TAKEN){
    ghr++;
  }

  // update the tournament counter
  bool global_pred_result = GetGlobalPrediction(PC);
  bool local_pred_result = GetLocalPrediction(PC);
    UINT32 pCC   = PC % (61);

	//if( abs(sum) <= thresold || (resolveDir != predDir)) original
	//if( (abs(sum) <= thresold) && (abs(sum) >= -thresold) Should We consider the thresold
	//if((resolveDir != predDir))
	if ((global_pred_result != local_pred_result) || (abs(sum) <= thresold))
	{
		for(int i=0; i<32; i++)
		{
			ghr_bit = ((ghr << (31-i)) >> 31);
			perceptrons[pCC][i] += (resolveDir==global_pred_result ? 1 : -1) * (ghr_bit ? 1: -1);
		}
        perceptrons[pCC][32]+=(resolveDir?1:-1);
	}

  //update the BHT and PHT for local branch predictor
  //update the PHT_LOCAL
  UINT32 bhtIndex   = (PC >> (32-bht_bit_size));
  UINT16 bht_result = bht[bhtIndex];
  UINT32 pht_local_index = (PC^(UINT32)(bht_result))% (numPhtLocalEntries);
  UINT32 pht_local_counter = pht_local[pht_local_index];
  if(resolveDir == TAKEN){
    pht_local[pht_local_index] = SatIncrement(pht_local_counter, PHT_LOCAL_CTR_MAX);
  }else{
    pht_local[pht_local_index] = SatDecrement(pht_local_counter);
  }

  //update the bht for local predictor
  bht[bhtIndex] = (bht[bhtIndex] << 1);
  if(resolveDir == TAKEN){
    bht[bhtIndex]++;
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
