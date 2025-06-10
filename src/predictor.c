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

unsigned ghistory;
unsigned mask;
int bhtBits;
int* bht;

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
      ghistory = 0;

      // If ghistory bits is 2, you start with 001 for 1
      // Shift left by 2 bits  to get 100
      // Subtract 1 from this value to get 011, will only ever use as many bits as are 1s in the mask/allocated by ghistoryBits
      mask = (1 << ghistoryBits) - 1;

      bhtBits = 1 << ghistoryBits;

      bht = calloc(bhtBits, sizeof(int));
      for (int i = 0; i < bhtBits; i++) {
        bht[i] = WN;
      }

      return;
    }
    case TOURNAMENT: {return;}
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
    case TOURNAMENT:
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
      unsigned newGhistory = ((ghistory << 1) | outcome);
      // Cut it down to the mask size (chopping off the leftmost bit)
      ghistory = newGhistory & mask;

      return;
    }
    case TOURNAMENT:
    case CUSTOM:
    default:
      return;
  }
}

void clean_predictor(){
  // Free all the data structures you created 
  free(bht);
}
