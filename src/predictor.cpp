//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include <math.h>
#include "predictor.h"

//
// TODO:Student Information
//
const char *studentName = "Devansh Gupta";
const char *studentID = "A69041198";
const char *email = "d6gupta@ucsd.edu";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = {"Static", "Gshare",
                         "Tournament", "Custom"};

// define number of bits required for indexing the BHT here.
int ghistoryBits = 15; // Number of bits used for Global History
int bpType;            // Branch Prediction Type
int verbose;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

//
// TODO: Add your own Branch Predictor data structures here
//
// gshare
uint8_t *bht_gshare;
uint64_t ghistory;

// tournament
// Local Histrory Table of 1024 entries of 10 bits each
uint16_t *localHistoryTable;
uint8_t *bht_tournament;
uint8_t *bht_global;
uint16_t globalHistory;

//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//

// gshare functions
void init_gshare()
{
  int bht_entries = 1 << ghistoryBits;
  bht_gshare = (uint8_t *)malloc(bht_entries * sizeof(uint8_t));
  int i = 0;
  for (i = 0; i < bht_entries; i++)
  {
    bht_gshare[i] = WN;
  }
  ghistory = 0;
}

uint8_t gshare_predict(uint32_t pc)
{
  // get lower ghistoryBits of pc
  uint32_t bht_entries = 1 << ghistoryBits;
  uint32_t pc_lower_bits = pc & (bht_entries - 1);
  uint32_t ghistory_lower_bits = ghistory & (bht_entries - 1);
  uint32_t index = pc_lower_bits ^ ghistory_lower_bits;
  switch (bht_gshare[index])
  {
  case WN:
    return NOTTAKEN;
  case SN:
    return NOTTAKEN;
  case WT:
    return TAKEN;
  case ST:
    return TAKEN;
  default:
    printf("Warning: Undefined state of entry in GSHARE BHT!\n");
    return NOTTAKEN;
  }
}

void train_gshare(uint32_t pc, uint8_t outcome)
{
  // get lower ghistoryBits of pc
  uint32_t bht_entries = 1 << ghistoryBits;
  uint32_t pc_lower_bits = pc & (bht_entries - 1);
  uint32_t ghistory_lower_bits = ghistory & (bht_entries - 1);
  uint32_t index = pc_lower_bits ^ ghistory_lower_bits;

  // Update state of entry in bht based on outcome
  switch (bht_gshare[index])
  {
  case WN:
    bht_gshare[index] = (outcome == TAKEN) ? WT : SN;
    break;
  case SN:
    bht_gshare[index] = (outcome == TAKEN) ? WN : SN;
    break;
  case WT:
    bht_gshare[index] = (outcome == TAKEN) ? ST : WN;
    break;
  case ST:
    bht_gshare[index] = (outcome == TAKEN) ? ST : WT;
    break;
  default:
    printf("Warning: Undefined state of entry in GSHARE BHT!\n");
    break;
  }

  // Update history register
  ghistory = ((ghistory << 1) | outcome);
}

void cleanup_gshare()
{
  free(bht_gshare);
}

// Tournament predictor functions

void init_tournament()
{
  uint32_t localHistoryEntries = 1024; // 10 bits for local history
  uint32_t globalHistoryEntries = 4096; // 12 bits for global history
  localHistoryTable = (uint16_t *)malloc(localHistoryEntries * sizeof(uint16_t));
  bht_tournament = (uint8_t *)malloc(localHistoryEntries * sizeof(uint8_t));
  bht_global = (uint8_t *)malloc(globalHistoryEntries * sizeof(uint8_t));
  int i = 0;

  for (i = 0; i < localHistoryEntries; i++)
  {
    localHistoryTable[i] = 0;
    bht_tournament[i] = WN3_3bit;
  }
  for (i = 0; i < globalHistoryEntries; i++)
  {
    bht_global[i] = WN;
  }
  globalHistory = 0;
}

uint8_t tournament_global_predict(uint32_t pc)
{
  switch (bht_global[globalHistory])
  {
  case WN:
    return NOTTAKEN;
  case SN:
    return NOTTAKEN;
  case WT:
    return TAKEN;
  case ST:
    return TAKEN;
  default:
    printf("Warning: Undefined state of entry in Global BHT!\n");
    return NOTTAKEN;
  }
}

uint8_t tournament_local_predict(uint32_t pc)
{
  // get lower localHistoryBits of pc
  uint32_t bht_entries = 1024;
  uint32_t index = pc & (bht_entries - 1);

  switch (bht_tournament[localHistoryTable[index]])
  {
  case SN_3bit:
  case WN1_3bit:
  case WN2_3bit:
  case WN3_3bit:
    return NOTTAKEN;
  case WT1_3bit:
  case WT2_3bit:
  case WT3_3bit:
  case ST_3bit:
    return TAKEN;
  default:
    printf("Warning: Undefined state of entry in GSHARE BHT!\n");
    return NOTTAKEN;
  }
  return NOTTAKEN;
}

uint8_t tournament_predict(uint32_t pc)
{

}

uint8_t train_tournament_global(uint32_t pc, uint8_t outcome)
{
  // Update state of entry in bht based on outcome
  switch (bht_global[globalHistory])
  {
  case WN:
    bht_global[globalHistory] = (outcome == TAKEN) ? WT : SN;
    break;
  case SN:
    bht_global[globalHistory] = (outcome == TAKEN) ? WN : SN;
    break;
  case WT:
    bht_global[globalHistory] = (outcome == TAKEN) ? ST : WN;
    break;
  case ST:
    bht_global[globalHistory] = (outcome == TAKEN) ? ST : WT;
    break;
  default:
    printf("Warning: Undefined state of entry in Global BHT!\n");
    break;
  }

  // Update history register
  globalHistory = ((globalHistory << 1) | outcome) & 0xFFF; // keep only 12 bits
}

void train_tournament_local(uint32_t pc, uint8_t outcome)
{
    // get lower localHistoryBits of pc
  uint32_t bht_entries = 1024;
  uint32_t index = pc & (bht_entries - 1);

  // Update state of entry in bht based on outcome
  switch (bht_tournament[localHistoryTable[index]])
  {
    // SN_3bit, WN1_3bit, WN2_3bit, WN3_3bit, WT1_3bit, WT2_3bit, WT3_3bit, ST_3bit
  case SN_3bit:
    bht_tournament[localHistoryTable[index]] = (outcome == TAKEN) ? WN1_3bit : SN_3bit;
    break;
  case WN1_3bit:
    bht_tournament[localHistoryTable[index]] = (outcome == TAKEN) ? WN2_3bit : SN_3bit;
    break;
  case WN2_3bit:
    bht_tournament[localHistoryTable[index]] = (outcome == TAKEN) ? WN3_3bit : WN1_3bit;
    break;
  case WN3_3bit:
    bht_tournament[localHistoryTable[index]] = (outcome == TAKEN) ? WT1_3bit : WN2_3bit;
    break;
  case WT1_3bit:
    bht_tournament[localHistoryTable[index]] = (outcome == TAKEN) ? WT2_3bit : WN3_3bit;
    break;
  case WT2_3bit:
    bht_tournament[localHistoryTable[index]] = (outcome == TAKEN) ? WT3_3bit : WT1_3bit;
    break;
  case WT3_3bit:
    bht_tournament[localHistoryTable[index]] = (outcome == TAKEN) ? ST_3bit : WT2_3bit;
    break;
  case ST_3bit:
    bht_tournament[localHistoryTable[index]] = (outcome == TAKEN) ? ST_3bit : WT3_3bit;
    break;
  default:
    printf("Warning: Undefined state of entry in localHistoryTable BHT!\n");
    break;
  }

  // Update history register
  localHistoryTable[index] = ((localHistoryTable[index] << 1) | outcome) & 0x3FF; // keep only 10 bits
}

void train_tournament(uint32_t pc, uint8_t outcome)
{
  train_tournament_global(pc, outcome);
  train_tournament_local(pc, outcome);
}

void cleanup_tournament()
{
  free(localHistoryTable);
  free(bht_tournament);
  free(bht_global);
}

void init_predictor()
{
  switch (bpType)
  {
  case STATIC:
    break;
  case GSHARE:
    init_gshare();
    break;
  case TOURNAMENT:
    init_tournament();
    break;
  case CUSTOM:
    break;
  default:
    break;
  }
}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint32_t make_prediction(uint32_t pc, uint32_t target, uint32_t direct)
{

  // Make a prediction based on the bpType
  switch (bpType)
  {
  case STATIC:
    return TAKEN;
  case GSHARE:
    return gshare_predict(pc);
  case TOURNAMENT:
    return tournament_predict(pc);
  case CUSTOM:
    return NOTTAKEN;
  default:
    break;
  }

  // If there is not a compatable bpType then return NOTTAKEN
  return NOTTAKEN;
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//

void train_predictor(uint32_t pc, uint32_t target, uint32_t outcome, uint32_t condition, uint32_t call, uint32_t ret, uint32_t direct)
{
  if (condition)
  {
    switch (bpType)
    {
    case STATIC:
      return;
    case GSHARE:
      return train_gshare(pc, outcome);
    case TOURNAMENT:
      return train_tournament(pc, outcome);
    case CUSTOM:
      return;
    default:
      break;
    }
  }
}
