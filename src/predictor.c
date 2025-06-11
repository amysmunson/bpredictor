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
const char *studentName = "NAME";
const char *studentID   = "PID";
const char *email       = "EMAIL";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = { "Static", "Gshare",
                          "Tournament", "Custom" };

int ghistoryBits; // Number of bits used for Global History
int lhistoryBits; // Number of bits used for Local History
int pcIndexBits;  // Number of bits used for PC index
int bpType;       // Branch Prediction Type
int verbose;
int customType;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

//
//TODO: Add your own Branch Predictor data structures here
//
// GShare
unsigned ghistory;
unsigned mask;
int bhtBits;
int* bht;

// Tournament
int* choices;

// Commenting things that already exist to keep track of what I'm using for each predictor
// unsigned ghistory; (Already declared for gshare)
// unsigned mask; (Already declared for gshare)
int gsize;
int* global;

unsigned lhmask;
int lhsize;
unsigned* lhistories;
unsigned lpmask;
int lpsize;
int* lpredict;

int* choices;

// Custom (Perceptron version -- Tournament-based predictor will use tournament data structures)
// unsigned ghistory; (Already declared for gshare)
// unsigned mask; (Already declared for gshare)
int pmask;
int psize;
int threshold;
int** perceptrons;



//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Init functions for my custom predictors

void init_custom_tournament(){
  // Copied from tournament set up with set-in-stone bit numbers
  ghistoryBits = 25;
  lhistoryBits = 25;
  pcIndexBits = 25;

  ghistory = 0;

  mask = (1 << ghistoryBits) - 1;
  lhmask = (1 << pcIndexBits) - 1;
  lpmask = (1 << lhistoryBits) - 1;

  gsize = 1 << ghistoryBits;
  lhsize = 1 << pcIndexBits;
  lpsize = 1 << lhistoryBits;

  global = calloc(gsize, sizeof(int));
  for (int i = 0; i < gsize; i++) {
    global[i] = WN;
  }

  lhistories = calloc(lhsize, sizeof(unsigned));
  for (int i = 0; i < lhsize; i++) {
    lhistories[i] = 0;
  }

  lpredict = calloc(lpsize, sizeof(int));
  for (int i = 0; i < lpsize; i++) {
    lpredict[i] = WN;
  }

  // Weak local default
  choices = calloc(gsize, sizeof(int));
  for (int i = 0; i < gsize; i++) {
    choices[i] = 2;
  }
}

void init_custom_perceptron(){
  // Setting these in stone
  ghistoryBits = 28;
  pcIndexBits = 9;

  // Basing this value off the Jimenez paper
  threshold = (1.93 * ghistoryBits + 14);

  ghistory = 0;

  mask = (1 << ghistoryBits) - 1;
  pmask = (1 << pcIndexBits) - 1;
  
  psize = 1 << pcIndexBits;

  // Table of perceptron arrays that are bias + weights indexed by pc
  perceptrons = calloc(psize, sizeof(int*));
  for (int i = 0; i < psize; i++) {
    perceptrons[i] = calloc(ghistoryBits + 1, sizeof(int));
  }

  return;
}

// Initialize the predictor
//
void
init_predictor()
{
  switch (bpType) {
    case GSHARE: {
      // Needs to start as 0
      ghistory = 0;

      // If ghistory bits is 2, you start with 001 for 1
      // Shift left by 2 bits  to get 100
      // Subtract 1 from this value to get 011, will only ever use as many bits as are 1s in the mask/allocated by ghistoryBits
      mask = (1 << ghistoryBits) - 1;

      // 2^bits is the size of the table
      bhtBits = 1 << ghistoryBits;

      // Allocate the branch history table and init everything to weak not taken
      bht = calloc(bhtBits, sizeof(int));
      for (int i = 0; i < bhtBits; i++) {
        bht[i] = WN;
      }

      return;
    }
    case TOURNAMENT: {
      ghistory = 0;

      mask = (1 << ghistoryBits) - 1;
      lhmask = (1 << pcIndexBits) - 1;
      lpmask = (1 << lhistoryBits) - 1;

      gsize = 1 << ghistoryBits;
      lhsize = 1 << pcIndexBits;
      lpsize = 1 << lhistoryBits;

      global = calloc(gsize, sizeof(int));
      for (int i = 0; i < gsize; i++) {
        global[i] = WN;
      }

      lhistories = calloc(lhsize, sizeof(unsigned));
      for (int i = 0; i < lhsize; i++) {
        lhistories[i] = 0;
      }

      lpredict = calloc(lpsize, sizeof(int));
      for (int i = 0; i < lpsize; i++) {
        lpredict[i] = WN;
      }

      // Which do you default to? Weak for sure, but local or global? 1-2
      choices = calloc(gsize, sizeof(int));
      for (int i = 0; i < gsize; i++) {
        choices[i] = 2;
      }
      return;
    }
    case CUSTOM: {
      // Initialize differently depending on the different custom function
      if (customType == 0){
        init_custom_tournament();
      }
      else if (customType == 1){
        init_custom_perceptron();
      }
      return;
    }
    default:
      return;
  }
}

// Prediction Functions for the two Custom Predictors

// Modified tournament custom predictor
int custom_tournament(uint32_t pc){
  // Important: Using the PC as the choice index instead of the globalhistory
  int choiceIndex = pc & mask;
  // Find the selector choice for global/local 
  int choice = choices[choiceIndex];

  int prediction = 0;
  // Select Global Predictor
  if (choice >= 2){
    // Just grab the prediciton from the global table
    int ghistoryIndex = ghistory & mask;
    prediction = global[ghistoryIndex];
    if (prediction == WN || prediction == SN){
      return NOTTAKEN;
    }
    else {
      return TAKEN;
    }
  }
  // Select Local Predictor
  else {
    // Find the history in the table of local histories given PC
    int lhIndex = pc & lhmask;
    unsigned lhistory = lhistories[lhIndex];
    // Find the prediction for this history
    int lpIndex = lhistory & lpmask;
    prediction = lpredict[lpIndex];
    if (prediction == WN || prediction == SN){
      return NOTTAKEN;
    }
    else {
      return TAKEN;
    }
  }

}

// Perceptron predictor 
int custom_perceptron(uint32_t pc){
  // Index in w/ pc
  int pcIndex = pc & pmask;
  // array with bias, weights
  int* perceptron = perceptrons[pcIndex];

  int sum = 0;
  // Sum the (global history bits * their weight)
  for (int i = 0; i < ghistoryBits; i++) {
    // Shift the history so this bit is the last one
    int shiftedGhistory = ghistory >> i;
    // Grab only this bit
    int currBit = shiftedGhistory & 1;
    // If the bit says not taken, convert it to -1 for the perceptron calculation
    int bitVal = 1;
    if (currBit == 0){
      bitVal = -1;
    }

    // weight index is +1 because bias is index 0
    int weight = perceptron[i + 1];

    int currVal = weight * bitVal;
    sum = sum + currVal;
  }

  // Add the bias
  int bias = perceptron[0];
  sum = sum + bias;

  // If the sum is not negative, take the branch
  if (sum < 0){
    return NOTTAKEN;
  }
  else{
    return TAKEN;
  }

}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint8_t
make_prediction(uint32_t pc)
{
  // Make a prediction based on the bpType
  switch (bpType) {
    case STATIC:
      return TAKEN;
    case GSHARE: {
      // XOR for the bht index
      int xorval = pc ^ ghistory;
      // Don't grab an index that's out of bounds and then segfault
      int predindex = xorval & mask;
      // Get the prediction, return the corresponding value
      int prediction = bht[predindex];
      if (prediction == WN || prediction == SN){
        return NOTTAKEN;
      }
      else{
        return TAKEN;
      }
    }
    case TOURNAMENT: {
      // Find the selector choice for global/local 
      int ghistoryIndex = ghistory & mask;
      int choice = choices[ghistoryIndex];
      int prediction = 0;
      // Select Global Predictor
      if (choice >= 2){
        // Just grab the prediciton from the global table
        prediction = global[ghistoryIndex];
        if (prediction == WN || prediction == SN){
          return NOTTAKEN;
        }
        else {
          return TAKEN;
        }
      }
      // Select Local Predictor
      else {
        // Find the history in the table of local histories given PC
        int lhIndex = pc & lhmask;
        unsigned lhistory = lhistories[lhIndex];
        // Find the prediction for this history
        int lpIndex = lhistory & lpmask;
        prediction = lpredict[lpIndex];
        if (prediction == WN || prediction == SN){
          return NOTTAKEN;
        }
        else {
          return TAKEN;
        }
      }
    }
    case CUSTOM: {
      // Predict differently depending on the different custom function, make sure to use the right one
      int prediction = TAKEN;
      if (customType == 0){
        prediction = custom_tournament(pc);
      }
      else if (customType == 1){
        prediction = custom_perceptron(pc);
      }
      return prediction;
    }
    default:
      break;
  }

  // If there is not a compatable bpType then return NOTTAKEN
  return NOTTAKEN;
}

// Training Functions for the Two Custom Predictors

// Custom tournament-based training
void train_custom_tournament(uint32_t pc, uint8_t outcome){
  // Important: Using the PC as the choice index instead of the globalhistory
  int choiceIndex = pc & mask;
  int choice = choices[choiceIndex];

  // Select Global Predictor
  int ghistoryIndex = ghistory & mask;
  int gprediction = global[ghistoryIndex];
  
  // Select Local Predictor
  int lprediction;
  // Find the history in the table given PC
  int lhIndex = pc & lhmask;
  unsigned lhistory = lhistories[lhIndex];
  // Find the prediction for this history
  int lpIndex = lhistory & lpmask;
  lprediction = lpredict[lpIndex];

  // Simplify to whether they decided to take the branch
  int gout = 0;
  if (gprediction == WN || gprediction == SN){
    gout = NOTTAKEN;
  }
  else{
    gout = TAKEN;
  }
  int lout = 0;
  if (lprediction == WN || lprediction == SN){
    lout = NOTTAKEN;
  }
  else{
    lout = TAKEN;
  }

  // Update the global and local histories with this new outcome
  if(outcome != TAKEN){
    if (gprediction != SN){
      global[ghistoryIndex] = gprediction - 1;
    }
    if (lprediction != SN){
      lpredict[lpIndex] = lprediction - 1;
    }
  }
  else{
    if (gprediction != ST){
      global[ghistoryIndex] = gprediction + 1;
    }
    if (lprediction != ST){
      lpredict[lpIndex] = lprediction + 1;
    }
  }

  // If the two had different predictions, update which one you choose based on who was right
  if (gout != lout){
    // Favor global chances
    if(gout == outcome){
      if (choice < 3){
        choice++;
        choices[choiceIndex] = choice;
      }
    } 
    // Favor local chances
    else {
      if (choice > 0){
        choice--;
        choices[choiceIndex] = choice;
      }
    }
  }

  // Update the local and global history
  // Shift it left 1 (opening up a new 0 on the right), fill that spot with the actual outcome
  unsigned newGhistory = (ghistory << 1) | outcome;
  // Cut it down to the mask size (chopping off the leftmost bit)
  ghistory = newGhistory & mask;
  // Do the same for the local history
  unsigned newLhistory = (lhistory << 1) | outcome;
  lhistories[lhIndex] = newLhistory & lpmask;

  return;
}

// Custom perceptron predictor
void train_custom_perceptron(uint32_t pc, uint8_t outcome){
  // Index in w/ pc
  int pcIndex = pc & pmask;
  // array with bias, weights
  int* perceptron = perceptrons[pcIndex];

  int sum = 0;
  // Sum the (global history bits * their weight)
  for (int i = 0; i < ghistoryBits; i++) {
    // Shift the history so this bit is the last one
    int shiftedGhistory = ghistory >> i;
    // Grab only this bit
    int currBit = shiftedGhistory & 1;
    // If the bit says not taken, convert it to -1 for the perceptron calculation
    int bitVal = 1;
    if (currBit == 0){
      bitVal = -1;
    }

    // weight index is +1 because bias is index 0
    int weight = perceptron[i + 1];

    int currVal = weight * bitVal;
    sum = sum + currVal;
  }

  // Add the bias
  int bias = perceptron[0];
  sum = sum + bias;

  int prediction = TAKEN;
  if (sum < 0){
    prediction = NOTTAKEN;
  }
  else{
    prediction = TAKEN;
  }

  // If the outcome is not taken, convert it to -1 for the perceptron calculations
  int outcomeVal = 1;
  if (outcome == NOTTAKEN){
    outcomeVal = -1;
  }

  if (abs(sum) <=  threshold){
    // Update the bias with your actual outcome
    if (outcomeVal == -1){
      // Stay within your bounds
      if (bias > -128){
        perceptron[0]--;
      }
    }
    else{
      // Stay within your bounds
      if (bias < 127){
        perceptron[0]++;
      }
    }

    // Update the weight for every bit on if it matches the outcome
    for (int i = 0; i < ghistoryBits; i++) {
      // Shift the history so this bit is the last one
      int shiftedGhistory = ghistory >> i;
      // Grab only this bit
      int currBit = shiftedGhistory & 1;
      // If the bit says not taken, convert it to -1 for the perceptron
      int bitVal = 1;
      if (currBit == 0){
        bitVal = -1;
      }

      // Should come out to 1 if they match and -1 if they don't
      int newWeight = outcomeVal * bitVal;
      // Don't forget the bias
      int currWeight = perceptron[i + 1];

      // Update the weight within the bounds
      if (newWeight < 0){
        if (currWeight > -128){
          perceptron[i + 1] = currWeight + newWeight;
        }
      }
      else{
        if (currWeight < 127){
          perceptron[i + 1] = currWeight + newWeight;
        }

      }
    }
  }
  else if (prediction != outcome) {
    // Update the bias with your actual outcome
    if (outcomeVal == -1){
      // Stay within your bounds
      if (bias > -128){
        // Increase the bias
        perceptron[0]++;
      }
    }
    else{
      // Stay within your bounds
      if (bias < 127){
        // Decrease the bias
        perceptron[0]--;
      }
    }

    // Update the weight for every bit on if it matches the outcome
    for (int i = 0; i < ghistoryBits; i++) {
      // Shift the history so this bit is the last one
      int shiftedGhistory = ghistory >> i;
      // Grab only this bit
      int currBit = shiftedGhistory & 1;
      // If the bit says not taken, convert it to -1 for the perceptron
      int bitVal = 1;
      if (currBit == 0){
        bitVal = -1;
      }

      // Should come out to 1 if they match and -1 if they don't
      int newWeight = outcomeVal * bitVal;
      // Don't forget the bias
      int currWeight = perceptron[i + 1];

      // Update the weight within the bounds
      if (newWeight < 0){
        if (currWeight > -128){
          perceptron[i + 1] = currWeight + newWeight;
        }
      }
      else{
        if (currWeight < 127){
          perceptron[i + 1] = currWeight + newWeight;
        }

      }
    }
  }

  unsigned newGhistory = (ghistory << 1) | outcome;
  // Cut it down to the mask size
  ghistory = newGhistory & mask;

  return;
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//
void
train_predictor(uint32_t pc, uint8_t outcome)
{
  switch (bpType) {
    case GSHARE: {
      // Same as before, grab the xor value and make sure it fits inside the mask/bht table
      int xorval = pc ^ ghistory;
      int predindex = xorval & mask;
      // Grab the original prediction
      int prediction = bht[predindex];
      
      // Update the prediction for this value of the table, making sure not to go out of bounds as to accepted values
      if (outcome == TAKEN){
        if (prediction != ST){
          prediction++;
        }
      }
      else{
        if (prediction != SN){
          prediction--;
        }
      }

      // Store the new prediction for this index
      bht[predindex] = prediction;

      // Update the global history
      // Shift it left 1 (opening up a new 0 on the right), fill that spot with the actual outcome
      unsigned newGhistory = (ghistory << 1) | outcome;
      // Cut it down to the mask size (chopping off the leftmost bit)
      ghistory = newGhistory & mask;

      break;
    }
    case TOURNAMENT: {
      int ghistoryIndex = ghistory & mask;
      int choice = choices[ghistoryIndex];

      // Select Global Predictor
      int gprediction = global[ghistoryIndex];
      
      // Select Local Predictor
      int lprediction;
      // Find the history in the table given PC
      int lhIndex = pc & lhmask;
      unsigned lhistory = lhistories[lhIndex];
      // Find the prediction for this history
      int lpIndex = lhistory & lpmask;
      lprediction = lpredict[lpIndex];

      // Simplify to whether they decided to take the branch
      int gout = 0;
      if (gprediction == WN || gprediction == SN){
        gout = NOTTAKEN;
      }
      else{
        gout = TAKEN;
      }
      int lout = 0;
      if (lprediction == WN || lprediction == SN){
        lout = NOTTAKEN;
      }
      else{
        lout = TAKEN;
      }

      // Update the global and local histories with this new outcome
      if(outcome != TAKEN){
        if (gprediction != SN){
          global[ghistoryIndex] = gprediction - 1;
        }
        if (lprediction != SN){
          lpredict[lpIndex] = lprediction - 1;
        }
      }
      else{
        if (gprediction != ST){
          global[ghistoryIndex] = gprediction + 1;
        }
        if (lprediction != ST){
          lpredict[lpIndex] = lprediction + 1;
        }
      }

      // If the two had different predictions, update which one you choose based on who was right
      if (gout != lout){
        // Favor global chances
        if(gout == outcome){
          if (choice < 3){
            choice++;
            choices[ghistoryIndex] = choice;
          }
        } 
        // Favor local chances
        else {
          if (choice > 0){
            choice--;
            choices[ghistoryIndex] = choice;
          }
        }
      }

      // Update the local and global history
      // Shift it left 1 (opening up a new 0 on the right), fill that spot with the actual outcome
      unsigned newGhistory = (ghistory << 1) | outcome;
      // Cut it down to the mask size (chopping off the leftmost bit)
      ghistory = newGhistory & mask;
      // Do the same for the local history
      unsigned newLhistory = (lhistory << 1) | outcome;
      lhistories[lhIndex] = newLhistory & lpmask;

      break;
        
    }
    case CUSTOM: {
      // Train differently depending on the different custom function, make sure to use the right one
      int prediction = TAKEN;
      if (customType == 0){
        train_custom_tournament(pc, outcome);
      }
      else if (customType == 1){
        train_custom_perceptron(pc, outcome);
      }
      break;
    }
    default:
      break;
  }

  return;

}

void clean_predictor(){
  // Free all the data structures you created 

  // GShare
  if (bht){
    free(bht);
  }

  // Tournament and Custom Tournament
  if (global){
    free(global);
  }
  if (choices){
    free(choices);
  }
  if (lhistories){
    free(lhistories);
  }
  if (lpredict){
    free(lpredict);
  }

  // Custom Perceptron
  // Table of perceptron arrays that are bias + weights
  if (perceptrons){
    for (int i = 0; i < psize; i++) {
      free(perceptrons[i]);
    }
    free(perceptrons);
  }

  return;
}
