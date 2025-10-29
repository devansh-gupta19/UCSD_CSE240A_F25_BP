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
uint8_t *choice_bht;
uint16_t globalHistory;

// custom branch predictor data structures
uint8_t *base_bht_custom;
typedef struct {
    uint32_t tag;
    // Prediction Counter
    uint8_t ctr;
    // Useful Counter
    uint8_t useful;
} tage_table_entry;

typedef struct {
    tage_table_entry *tagTable;
    uint32_t tableSize;
    uint32_t historyBits;
    uint32_t numTagBits;
    uint32_t numEntries;
} tage_table;

tage_table tageTables[4] = {
    {.tableSize = 4096, .historyBits = 4, .numTagBits = 8},
    {.tableSize = 2048, .historyBits = 8, .numTagBits = 10},
    {.tableSize = 1024, .historyBits = 16, .numTagBits = 10},
    {.tableSize = 512, .historyBits = 32, .numTagBits = 12}
};
uint32_t baseTableEntries = 256;




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
  choice_bht = (uint8_t *)malloc(globalHistoryEntries * sizeof(uint8_t));
  int i = 0;

  for (i = 0; i < localHistoryEntries; i++)
  {
    localHistoryTable[i] = 0;
    bht_tournament[i] = WN3_3bit;
  }
  for (i = 0; i < globalHistoryEntries; i++)
  {
    bht_global[i] = WN;
    choice_bht[i] = WLocal;
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
  uint8_t choice = choice_bht[globalHistory];
  if (choice == SLocal || choice == WLocal)
  {
    return tournament_local_predict(pc);
  }
  else 
  {
    return tournament_global_predict(pc);
  }
}

void train_tournament_choice(uint32_t pc, uint8_t outcome, uint8_t local_pred, uint8_t global_pred)
{
  // Update choice predictor
  if (global_pred != local_pred)
  {
    switch (choice_bht[globalHistory])
    {
      // Update state of entry in bht based on outcome
      case SGlobal:
        choice_bht[globalHistory] = (global_pred == outcome && local_pred != outcome) ? SGlobal : WGlobal;
        break;
      case WGlobal:
        choice_bht[globalHistory] = (global_pred == outcome && local_pred != outcome) ? SGlobal : WLocal;
        break;
      case WLocal:
        choice_bht[globalHistory] = (global_pred == outcome && local_pred != outcome) ? WGlobal : SLocal;
        break;
      case SLocal:
        choice_bht[globalHistory] = (global_pred == outcome && local_pred != outcome) ? WLocal : SLocal;
        break;
      default:
        printf("Warning: Undefined state of entry in Choice BHT!\n");
        break;
    }
  }
}

void train_tournament_global(uint32_t pc, uint8_t outcome)
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
  uint8_t local_pred = tournament_local_predict(pc);
  uint8_t global_pred = tournament_global_predict(pc);

  train_tournament_choice(pc, outcome, local_pred, global_pred);
  train_tournament_global(pc, outcome);
  train_tournament_local(pc, outcome);
}

void cleanup_tournament()
{
  free(localHistoryTable);
  free(bht_tournament);
  free(bht_global);
}




// Custom Predictor functions
void init_custom()
{
  uint32_t i;
  base_bht_custom = (uint8_t *)malloc(baseTableEntries * sizeof(uint8_t));

  for (int idx = 0; idx < 4; idx++)
  {
    tageTables[idx].tagTable = (tage_table_entry *)malloc(tageTables[idx].tableSize * sizeof(tage_table_entry));
    for (i = 0; i < tageTables[idx].tableSize; i++)
    {
      tageTables[idx].tagTable[i].tag = 0xFFFFFFFF;
      tageTables[idx].tagTable[i].useful = U0;
      tageTables[idx].tagTable[i].ctr = WN;
    }
    tageTables[idx].numEntries = 0;
  }

  ghistory = 0;
    for (i = 0; i < baseTableEntries; i++)
  {
    base_bht_custom[i] = WN;
  }

}

uint8_t custom_base_predict(uint32_t pc)
{
  // get lower bits of pc
  uint32_t index = pc & (baseTableEntries - 1);
  switch (base_bht_custom[index])
  {
  case WN:
  case SN:
    return NOTTAKEN;
  case WT:
  case ST:
    return TAKEN;
  default:
    printf("Warning: Undefined state of entry in Base BHT!\n");
    return NOTTAKEN;
  }
}

uint32_t computeIndex(uint32_t pc, tage_table *table)
{
  uint32_t mask_out = (1 << table->historyBits) - 1;
  uint32_t out_len = (uint32_t)log2(table->tableSize);

  uint32_t folded_history = 0;
  for (int i = 0; i < table->historyBits; i += out_len)
  {
    folded_history ^= (ghistory >> i) & mask_out;
  }

  return (pc ^ folded_history) & mask_out;
}

uint32_t computeTag(uint32_t pc, tage_table *table)
{
  uint32_t mask_out = (1 << table->numTagBits) - 1;

  uint32_t folded_history = 0;
  for (int i = 0; i < table->historyBits; i += table->numTagBits)
  {
    folded_history ^= (ghistory >> i) & mask_out;
  }

  return (pc ^ folded_history) & mask_out;
}

uint8_t custom_tx_predict(uint32_t pc, tage_table *table)
{
  uint32_t tag = (computeIndex(pc, table) ^ pc) & ((1 << table->numTagBits) - 1);

  for (int i = 0; i < table->tableSize; i++)
  {
    if (table->tagTable[i].tag == tag)
    {
      if (table->tagTable[i].ctr == WN || table->tagTable[i].ctr == SN)
      {
        return NOTTAKEN;
      }
      else 
      {
        return TAKEN;
      }
    }
  }
  return NOTAPPLICABLE;
}


uint8_t custom_predict(uint32_t pc)
{
  uint8_t out = custom_base_predict(pc);
  uint8_t t1Out = custom_tx_predict(pc, &tageTables[0]);
  uint8_t t2Out = custom_tx_predict(pc, &tageTables[1]);
  uint8_t t3Out = custom_tx_predict(pc, &tageTables[2]);
  uint8_t t4Out = custom_tx_predict(pc, &tageTables[3]);
  if (t4Out != NOTAPPLICABLE)
  {
    return t4Out;
  }
  else if (t3Out != NOTAPPLICABLE)
  {
    return t3Out;
  }
  else if (t2Out != NOTAPPLICABLE)
  {
    return t2Out;
  }
  else if (t1Out != NOTAPPLICABLE)
  {
    return t1Out;
  }
  else 
  {
    return out;
  }
}

void train_custom_base(uint32_t pc, uint8_t outcome)
{
  uint32_t index = pc & (baseTableEntries - 1);
  switch (base_bht_custom[index])
  {
  case WN:
    base_bht_custom[index] = (outcome == TAKEN) ? WT : SN;
    break;
  case SN:
    base_bht_custom[index] = (outcome == TAKEN) ? WN : SN;
    break;
  case WT:
    base_bht_custom[index] = (outcome == TAKEN) ? ST : WN;
    break;
  case ST:
    base_bht_custom[index] = (outcome == TAKEN) ? ST : WT;
    break;
  default:
    base_bht_custom[index] = WN;
    printf("Warning: Undefined state of entry in BHT!\n");
    break;
  }

}

uint8_t addNewEntry(uint32_t pc, uint8_t outcome, tage_table *table)
{
  // get lower historyBits of pc
  uint32_t idx;
  uint32_t ghistory_lower_bits;
  uint32_t folded_history;
  uint32_t index;
  uint32_t tag;
  uint8_t tableFound = 0;

  ghistory_lower_bits = ghistory & ((1 << table->historyBits) - 1);
  folded_history = 0;
  for (int i = 0; i < table->historyBits; i += log2(table->tableSize))
  {
    folded_history ^= (ghistory_lower_bits >> i);
  }

  index = (pc ^ folded_history);
  tag = index & ((1 << table->numTagBits) - 1);

  // First try to add data in the last table
  for (idx = 0; idx < table->tableSize; idx++)
  {
    if (table->tagTable[idx].useful == U0)
    {
      table->tagTable[idx].useful++;

      // Allocate entry
      table->tagTable[idx].tag = tag;
      table->tagTable[idx].ctr = (outcome == TAKEN) ? WT : WN;
      tableFound = 1;
      break;
    }
  }
  if (!tableFound)
  {
    // First try to add data in the last table
    for (idx = 0; idx < table->tableSize; idx++)
    {
      if (table->tagTable[idx].useful == U1)
      {
        // Allocate entry
        table->tagTable[idx].tag = tag;
        table->tagTable[idx].ctr = (outcome == TAKEN) ? WT : WN;
        tableFound = 1;
        break;
      }
    }
  }

  return tableFound;
}

uint8_t deleteEntry(uint32_t pc, tage_table *table)
{
  // get lower historyBits of pc
  uint32_t idx;
  uint32_t ghistory_lower_bits;
  uint32_t folded_history;
  uint32_t index;
  uint32_t tag;
  uint8_t tableFound = 0;

  ghistory_lower_bits = ghistory & ((1 << table->historyBits) - 1);
  folded_history = 0;
  for (int i = 0; i < table->historyBits; i += log2(table->tableSize))
  {
    folded_history ^= (ghistory_lower_bits >> i);
  }

  index = (pc ^ folded_history);
  tag = index & ((1 << table->numTagBits) - 1);

  for (idx = 0; idx < table->tableSize; idx++)
  {
    if (table->tagTable[idx].tag == tag)
    {
      table->tagTable[idx].useful = U0;

      // Allocate entry
      table->tagTable[idx].tag = 0xFFFFFFFF;
      table->tagTable[idx].ctr = WN;
      tableFound = 1;
      break;
    }
  }
  return tableFound;
}

uint8_t train_custom_tx(uint32_t pc, uint8_t outcome, tage_table *table)
{
  uint32_t idx;
  uint8_t tagFound = 0;
  // get lower historyBits of pc
  uint32_t ghistory_lower_bits = ghistory & ((1 << table->historyBits) - 1);
  uint32_t folded_history = 0;
  for (int i = 0; i < table->historyBits; i += log2(table->tableSize))
  {
    folded_history ^= (ghistory_lower_bits >> i);
  }

  uint32_t index = (pc ^ folded_history);
  uint32_t tag = index & ((1 << table->numTagBits) - 1);

  for (idx = 0; idx < table->tableSize; idx++)
  {
    if (table->tagTable[idx].tag == tag)
    {
      tagFound = 1;
      // Update state of entry in bht based on outcome
      switch (table->tagTable[idx].ctr)
      {
      case WN:
        table->tagTable[idx].ctr = (outcome == TAKEN) ? WT : SN;
        break;
      case SN:
        table->tagTable[idx].ctr = (outcome == TAKEN) ? WN : SN;
        break;
      case WT:
        table->tagTable[idx].ctr = (outcome == TAKEN) ? ST : WN;
        break;
      case ST:
        table->tagTable[idx].ctr = (outcome == TAKEN) ? ST : WT;
        break;
      default:
        printf("Warning: Undefined state of entry in TX BHT!\n");
        break;
      }
      break;
    }
  }
  return tagFound;
}

void train_custom(uint32_t pc, uint8_t outcome)
{
  uint8_t tableFound;
  uint8_t t0Out = custom_tx_predict(pc, &tageTables[0]);
  uint8_t t1Out = custom_tx_predict(pc, &tageTables[1]);
  uint8_t t2Out = custom_tx_predict(pc, &tageTables[2]);
  uint8_t t3Out = custom_tx_predict(pc, &tageTables[3]);
  uint8_t baseOut = custom_base_predict(pc);

  //uint8_t t1Out = train_custom_tx(pc, outcome, &tageTables[0]);
  //uint8_t t2Out = train_custom_tx(pc, outcome, &tageTables[1]);
  //uint8_t t3Out = train_custom_tx(pc, outcome, &tageTables[2]);
  //uint8_t t4Out = train_custom_tx(pc, outcome, &tageTables[3]);


  if ((t0Out != outcome) &&
      (t1Out != outcome) &&
      (t2Out != outcome) &&
      (t3Out != outcome) &&
      (baseOut != outcome))
  {
    if (baseOut != NOTAPPLICABLE)
    {
      tableFound = addNewEntry(pc, outcome, &tageTables[0]);
    }
    // This branch data is not there in any table, add to first table
    if (t0Out != NOTAPPLICABLE)
    {
      tableFound = addNewEntry(pc, outcome, &tageTables[1]);
      deleteEntry(pc, &tageTables[0]);
    }
    else if (t1Out != NOTAPPLICABLE)
    {
      tableFound = addNewEntry(pc, outcome, &tageTables[2]);
      deleteEntry(pc, &tageTables[1]);
    }
    else if (t2Out != NOTAPPLICABLE)
    {
      tableFound = addNewEntry(pc, outcome, &tageTables[3]);
      deleteEntry(pc, &tageTables[2]);
    }
    else if (t3Out != NOTAPPLICABLE)
    {
      tableFound = addNewEntry(pc, outcome, &tageTables[3]);

    }
  }
  else 
  {
    if (t3Out == outcome)
    {
      train_custom_tx(pc, outcome, &tageTables[3]);
      deleteEntry(pc, &tageTables[2]);
      deleteEntry(pc, &tageTables[1]);
      deleteEntry(pc, &tageTables[0]);
    }
    else if (t2Out == outcome)
    {
      train_custom_tx(pc, outcome, &tageTables[2]);
      deleteEntry(pc, &tageTables[1]);
      deleteEntry(pc, &tageTables[0]);

    }
    else if (t1Out == outcome)
    {
      train_custom_tx(pc, outcome, &tageTables[1]);
      deleteEntry(pc, &tageTables[0]);
    }
    else if (t0Out == outcome)
    {
      train_custom_tx(pc, outcome, &tageTables[0]);
    }
    else if (baseOut == outcome)
    {
      train_custom_base(pc, outcome);
    }
  }
  ghistory = ((ghistory << 1) | outcome);
}

void cleanup_custom()
{
  free(base_bht_custom);
  free(tageTables[0].tagTable);
  free(tageTables[1].tagTable);
  free(tageTables[2].tagTable);
  free(tageTables[3].tagTable);
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
    init_custom();
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
    return custom_predict(pc);
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
    return train_custom(pc, outcome);
      return;
    default:
      break;
    }
  }
}
