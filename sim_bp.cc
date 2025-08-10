#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <stdlib.h>
#include <string.h>
#include "sim_bp.h"

int main(int argc, char* argv[])
{
    FILE* FP;               // File handler
    char* trace_file;       // Variable that holds trace file name;
    bp_params params;       // look at sim_bp.h header file for the the definition of struct bp_params
    char outcome;           // Variable holds branch outcome
    unsigned long int addr; // Variable holds the address read from input file

    
    if (!(argc == 4 || argc == 5 || argc == 7))
    {
        printf("Error: Wrong number of inputs:%d\n", argc - 1);
        exit(EXIT_FAILURE);
    }
    
    params.bp_name = argv[1];

    // strtoul() converts char* to unsigned long. It is included in <stdlib.h>
    if (strcmp(params.bp_name, "bimodal") == 0)              // Bimodal
    {
        if (argc != 4)
        {
            printf("Error: %s wrong number of inputs:%d\n", params.bp_name, argc - 1);
            exit(EXIT_FAILURE);
        }
        params.M2 = strtoul(argv[2], NULL, 10);
        trace_file = argv[3];
        printf("COMMAND\n%s %s %u %s\n", argv[0], params.bp_name, params.M2, trace_file);
    }
	else if (strcmp(params.bp_name, "gshare") == 0)		  // Gshare
    {
        if (argc != 5)
        {
            printf("Error: %s wrong number of inputs:%d\n", params.bp_name, argc - 1);
            exit(EXIT_FAILURE);
        }
        params.M1 = strtoul(argv[2], NULL, 10);
        params.N = strtoul(argv[3], NULL, 10);
        trace_file = argv[4];
        printf("COMMAND\n%s %s %u %u %s\n", argv[0], params.bp_name, params.M1, params.N, trace_file);
    }
    else if (strcmp(params.bp_name, "hybrid") == 0)
    {
        if (argc != 7)
        {
            printf("Error: %s wrong number of inputs:%d\n", params.bp_name, argc - 1);
            exit(EXIT_FAILURE);
        }
        params.K = strtoul(argv[2], NULL, 10);
        params.M1 = strtoul(argv[3], NULL, 10);
        params.N = strtoul(argv[4], NULL, 10);
        params.M2 = strtoul(argv[5], NULL, 10);
        trace_file = argv[6];
        printf("COMMAND\n%s %s %u %u %u %u %s\n", argv[0], params.bp_name, params.K, params.M1, params.N, params.M2, trace_file);
    }
    else
    {
        printf("Error: Wrong branch predictor name:%s\n", params.bp_name);
        exit(EXIT_FAILURE);
    }

    // Open trace_file in read mode
    FP = fopen(trace_file, "r");
    if (FP == NULL)
    {
        // Throw error and exit if fopen() failed
        printf("Error: Unable to open file %s\n", trace_file);
        exit(EXIT_FAILURE);
    }

    // Initialization
    int total_predictions = 0;
    int mispredictions = 0;

    // hybrid predictor
    int* chooser_table = nullptr;
    branch_predictor bimodal; // object for bimodal
    branch_predictor gshare;  // object for gshare 
    branch_predictor bp;      // object for bimodal & gshare when not using hybrid

	// Predictor initializations based on the predictor type (bimodal, gshare, or hybrid)
    if (strcmp(params.bp_name, "bimodal") == 0)
    {
        // Initialize bimodal predictor
        bp.type = BIMODAL;
        bp.num_entries = 1 << params.M2;
        bp.prediction_table = new int[bp.num_entries];

        // Initialize prediction table to 2 ("weakly taken")
        for (unsigned int i = 0; i < bp.num_entries; i++)
        {
            bp.prediction_table[i] = 2;
        }
    }
    else if (strcmp(params.bp_name, "gshare") == 0)
    {
        // Initialize gshare predictor
        bp.type = GSHARE;
        bp.num_entries = 1 << params.M1;
        bp.prediction_table = new int[bp.num_entries];
        bp.history_register = 0;
        bp.history_bits = params.N;

        // Initialize prediction table to 2 ("weakly taken")
        for (unsigned int i = 0; i < bp.num_entries; i++)
        {
            bp.prediction_table[i] = 2;
        }
    }
    else if (strcmp(params.bp_name, "hybrid") == 0)
    {
        // Initialize the chooser table
        unsigned int chooser_size = 1 << params.K;
        chooser_table = new int[chooser_size];
        for (unsigned int i = 0; i < chooser_size; i++)
        {
            chooser_table[i] = 1; // Initialize to 1 -> weak low
        }

        // Initialize the bimodal predictor
        bimodal.type = BIMODAL;
        bimodal.num_entries = 1 << params.M2;
        bimodal.prediction_table = new int[bimodal.num_entries];
        for (unsigned int i = 0; i < bimodal.num_entries; i++)
        {
            bimodal.prediction_table[i] = 2; // Initialize to "weakly taken"
        }

        // Initialize the gshare predictor
        gshare.type = GSHARE;
        gshare.num_entries = 1 << params.M1;
        gshare.prediction_table = new int[gshare.num_entries];
        gshare.history_register = 0;
        gshare.history_bits = params.N;
        for (unsigned int i = 0; i < gshare.num_entries; i++)
        {
            gshare.prediction_table[i] = 2; // Initialize to "weakly taken"
        }
    }

    // main logic
    char str[2];
    while (fscanf(FP, "%lx %s", &addr, str) != EOF){
        
        outcome = str[0];
        //if (outcome == 't')
        //    printf("%lx %s\n", addr, "t");           // Print and test if file is read correctly
        //else if (outcome == 'n')
        //    printf("%lx %s\n", addr, "n");          // Print and test if file is read correctly
        
        total_predictions++;

        // remove lowest two bits of PC - since they are always zero
        unsigned int pc = addr >> 2;

        if (strcmp(params.bp_name, "bimodal") == 0)
        {
            // Bimodal predictor
            unsigned int m = params.M2;
            unsigned int index = pc & ((1 << m) - 1); // lower m bits

            // Make prediction
            int counter = bp.prediction_table[index];
            char prediction = (counter >= 2) ? 't' : 'n';

            // Check for prediction
            if (prediction != outcome){
                mispredictions++;
            }

            // Update counter
            if (outcome == 't'){
                if (bp.prediction_table[index] < 3)
                    bp.prediction_table[index]++;
            }
            else{
                if (bp.prediction_table[index] > 0)
					bp.prediction_table[index]--;
            }
        }
        else if (strcmp(params.bp_name, "gshare") == 0)
        {
            // Gshare predictor
            unsigned int m = params.M1;
            unsigned int n = params.N;

            unsigned int pc_index_bits = pc & ((1 << m) - 1);

            // Extract upper bits
            unsigned int pc_upper_n_bits = (pc_index_bits >> (m - n)) & ((1 << n) - 1);

            unsigned int xor_result = pc_upper_n_bits ^ (bp.history_register & ((1 << n) - 1));

            // Combine result with (m - n) bits to get index 
            unsigned int index = (xor_result << (m - n)) | (pc_index_bits & ((1 << (m - n)) - 1));

            // Make prediction
            int counter = bp.prediction_table[index];
            char prediction = (counter >= 2) ? 't' : 'n';

            // Check if prediction is correct
            if (prediction != outcome){
                mispredictions++;
            }

            // Update the counter
            if (outcome == 't'){
                if (bp.prediction_table[index] < 3)
                    bp.prediction_table[index]++;
            }
            else{
                if (bp.prediction_table[index] > 0)
                    bp.prediction_table[index]--;
            }

            // Update the history register: shift right and insert outcome at MSB
            bp.history_register = ((outcome == 't' ? 1 : 0) << (n - 1)) | (bp.history_register >> 1);
        }
        else if (strcmp(params.bp_name, "hybrid") == 0){
            
			// gshare predictor for hybrid
            unsigned int m1 = params.M1;
            unsigned int n = params.N;

            unsigned int gshare_pc_index_bits = pc & ((1 << m1) - 1);

            // Extract upper bits
            unsigned int gshare_pc_upper_n_bits = (gshare_pc_index_bits >> (m1 - n)) & ((1 << n) - 1);

            // XOR with GHR
            unsigned int gshare_xor_result = gshare_pc_upper_n_bits ^ (gshare.history_register & ((1 << n) - 1));

            // Combine result with (m1 - n) to get index
            unsigned int gshare_index = (gshare_xor_result << (m1 - n)) | (gshare_pc_index_bits & ((1 << (m1 - n)) - 1));

            int gshare_counter = gshare.prediction_table[gshare_index];
            char gshare_prediction = (gshare_counter >= 2) ? 't' : 'n';

			// bimodal predictor for hybrid
            unsigned int m2 = params.M2;
            unsigned int bimodal_index = pc & ((1 << m2) - 1);

            int bimodal_counter = bimodal.prediction_table[bimodal_index];
            char bimodal_prediction = (bimodal_counter >= 2) ? 't' : 'n';

			// chooser predictor
            unsigned int chooser_index = pc & ((1 << params.K) - 1);

			// Get prediction from chooser table
            int chooser_counter = chooser_table[chooser_index];
            char final_prediction;
            bool used_gshare = false;
            if (chooser_counter >= 2){
                final_prediction = gshare_prediction;
                used_gshare = true;
            }
            else{
                final_prediction = bimodal_prediction;
            }

            // Check if prediction is correct
            if (final_prediction != outcome){
                mispredictions++;
            }

			                  // Update the predictors
            if (used_gshare){
                if (outcome == 't'){
                    if (gshare.prediction_table[gshare_index] < 3)
                        gshare.prediction_table[gshare_index]++;
                }
                else{
                    if (gshare.prediction_table[gshare_index] > 0)
                        gshare.prediction_table[gshare_index]--;
                }
            }
            else{           // Update bimodal predictor
                if (outcome == 't')
                {
                    if (bimodal.prediction_table[bimodal_index] < 3)
                        bimodal.prediction_table[bimodal_index]++;
                }
                else
                {
                    if (bimodal.prediction_table[bimodal_index] > 0)
                        bimodal.prediction_table[bimodal_index]--;
                }
            }

			// update history register for gshare
            gshare.history_register = ((outcome == 't' ? 1 : 0) << (n - 1)) | (gshare.history_register >> 1);

			// update chooser table
            bool gshare_correct = (gshare_prediction == outcome);
            bool bimodal_correct = (bimodal_prediction == outcome);

            if (gshare_correct != bimodal_correct){
                if (gshare_correct){
                    if (chooser_table[chooser_index] < 3)
                        chooser_table[chooser_index]++;
                }
                else{
                    if (chooser_table[chooser_index] > 0)
                        chooser_table[chooser_index]--;
                }
            }
        }
    }

    //misprediction rate
    double misprediction_rate = ((double)mispredictions / total_predictions) * 100.0;

	std::cout << "OUTPUT" << std::endl;
	std::cout << " number of predictions:    " << total_predictions << std::endl;
	std::cout << " number of mispredictions: " << mispredictions << std::endl;
    std::cout << " misprediction rate:       " << std::fixed << std::setprecision(2) << misprediction_rate << "%" << std::endl;

    // Output the final contents based on predictor type
    if (strcmp(params.bp_name, "bimodal") == 0){
		std::cout << "FINAL BIMODAL CONTENTS" << std::endl;
        for (unsigned int i = 0; i < bp.num_entries; i++){
			std::cout << " " << i << "\t" << bp.prediction_table[i] << std::endl;
        }
    }
    else if (strcmp(params.bp_name, "gshare") == 0){
		std::cout << "FINAL GSHARE CONTENTS" << std::endl;
        for (unsigned int i = 0; i < bp.num_entries; i++){
			std::cout << " " << i << "\t" << bp.prediction_table[i] << std::endl;
        }
    }
    else if (strcmp(params.bp_name, "hybrid") == 0){
        std::cout << "FINAL CHOOSER CONTENTS" << std::endl;
        for (unsigned int i = 0; i < (1 << params.K); i++){
            std::cout << " " << i << "\t" << chooser_table[i] << std::endl;
        }

		std::cout << "FINAL GSHARE CONTENTS" << std::endl;
        for (unsigned int i = 0; i < gshare.num_entries; i++){
			std::cout << " " << i << "\t" << gshare.prediction_table[i] << std::endl;
        }

        std::cout<<"FINAL BIMODAL CONTENTS"<<std::endl;
        for (unsigned int i = 0; i < bimodal.num_entries; i++){
            std::cout << " " << i << "\t" << bimodal.prediction_table[i] << std::endl;
        }
    }
    fclose(FP);
}
