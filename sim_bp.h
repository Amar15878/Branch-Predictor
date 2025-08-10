#ifndef SIM_BP_H
#define SIM_BP_H

// enum to represent predictor types
typedef enum {
    BIMODAL,
    GSHARE
} predictor_type;

// branch predictor parameters
typedef struct {
    unsigned long int K;
    unsigned long int M1;
    unsigned long int M2;
    unsigned long int N;
    char* bp_name;  
} bp_params;

// branch predictor structure for bimodal and gshare
typedef struct {
    predictor_type type;      // BIMODAL or GSHARE
    unsigned int num_entries; // Number of entries in the prediction table
    int* prediction_table;    // Pointer to the prediction table
    unsigned int history_register; // GHR for gshare predictor
    unsigned int history_bits;     // Number of bits in GHR (for gshare)
} branch_predictor;

#endif 
