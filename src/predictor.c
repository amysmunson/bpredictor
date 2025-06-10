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

// unsigned ghistory;
// unsigned mask;
int gsize;
int* global;

unsigned lhmask;
int lhsize;
unsigned* lhistories;
unsigned lpmask;
int lpsize;
int* lpredict;

int* choices;

// ghistoryBits; // Number of bits used for Global History
// lhistoryBits; // Number of bits used for Local History
// pcIndexBits; 

// Custom

//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

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
    case CUSTOM: {return;}
    default:
      return;
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
    case CUSTOM:
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
    case CUSTOM:
      break;
    default:
      break;
  }

  return;

}

void clean_predictor(){
  // Free all the data structures you created 
  if (bht){
    free(bht);
  }
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
}
