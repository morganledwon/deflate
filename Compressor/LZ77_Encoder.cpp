#include "LZ77_Encoder.h"
#include <utils/x_hls_utils.h>

t_hash_size hash_func(
		ap_uint<8> in_byte0,
		ap_uint<8> in_byte1,
		ap_uint<8> in_byte2,
		ap_uint<8> in_byte3,
		ap_uint<8> in_byte4
){
	t_hash_size hash;

	hash = ((t_hash_size)in_byte0 * 31) ^ in_byte1;
	hash.rrotate(4);
	hash = (hash * 3) ^ in_byte2;
	hash.rrotate(4);
	hash = (hash * 3) ^ in_byte3;
	hash.rrotate(4);
	hash = (hash * 3) ^ in_byte4;

	return hash;
}

void bank_access_func(
		t_hash_size         hash[WIN_SIZE],             //Input
		ap_uint<WIN_SIZE*8> substring_ints[WIN_SIZE],   //Input
		ap_uint<32>			data_pos,					//Input
		ap_uint<NUM_BANKS>  &bank_accessed_bits,		//Output
		ap_uint<WIN_SIZE>   &substring_access_bits,		//Output
		t_bank_values       associated_bank[WIN_SIZE],  //Output
		match_t 		    string_to_write[NUM_BANKS],	//Output
		t_bank_size 		string_address[NUM_BANKS]	//Output
){
#pragma HLS INLINE off
	t_num_banks desired_bank[WIN_SIZE];
#pragma HLS ARRAY_PARTITION variable=desired_bank complete dim=1
	t_bank_size bank_address[WIN_SIZE];
#pragma HLS ARRAY_PARTITION variable=bank_address complete dim=1
	t_win_size associated_substring[NUM_BANKS] = {0}; //Contains the substring number given to each bank
#pragma HLS ARRAY_PARTITION variable=associated_substring complete dim=1

	Bank_Access: for(int i = WIN_SIZE-1; i >= 0; i--){ //For each substring (starting from the bottom)
		desired_bank[i] = hash[i](HASH_BITS-1,HASH_BITS-NUM_BANKS_BITS); //The top 5 bits of the hash value indicate the desired bank to be accessed
		bank_address[i] = hash[i](BANK_SIZE_BITS-1,0);  //The bottom 9 bits are the bank address to access
		if(bank_accessed_bits[desired_bank[i]] == 0){ //If desired bank has not already been accessed
			bank_accessed_bits[desired_bank[i]] = 1; //Set bank as accessed
			substring_access_bits[i] = 1; //Record substring as granted access
			associated_bank[i] = desired_bank[i]; //Record bank accessed by substring
			associated_substring[desired_bank[i]] = i; //Record substring given to bank
		}
		else{
			associated_bank[i] = NUM_BANKS; //Associate with null bank
		}
	}

	//Pass 1 of 16 substrings to each of 32 banks (Not all banks will actually be accessed)
	String_to_Bank_MUX: for(int i = 0; i < NUM_BANKS; i++){//For each bank
		string_to_write[i].string = reg(substring_ints[associated_substring[i]]);
		string_to_write[i].position = data_pos + associated_substring[i];
		string_address[i] = bank_address[associated_substring[i]];
	}
}

void LZ77_Encoder(
		stream<in_strm>  &strm_in,
		stream<out_strm> &strm_out,
		int 			 input_size
){
#pragma HLS INTERFACE axis register both port=strm_in  //Input  AXI Stream
#pragma HLS INTERFACE axis register both port=strm_out //Output AXI Stream

	in_strm input_buff;
	ap_uint<8> curr_window[2*WIN_SIZE]; //Current window of data to be compressed.
#pragma HLS ARRAY_PARTITION variable=curr_window complete dim=1
	t_hash_size hash[WIN_SIZE]; //Hash values of each substring in curr_window.
#pragma HLS ARRAY_PARTITION variable=hash complete dim=1
	static match_t dictionary[NUM_BANKS][BANK_DEPTH][BANK_SIZE]; //Dictionary partitioned into banks with each bank having multiple depth levels
#pragma HLS RESOURCE variable=dictionary core=RAM_S2P_BRAM
#pragma HLS ARRAY_PARTITION variable=dictionary complete dim=1
#pragma HLS ARRAY_PARTITION variable=dictionary complete dim=2
	ap_uint<32> data_pos = 0; //Marker to keep track of input data position
	ap_uint<WIN_SIZE> match_length_bits[NUM_BANKS][BANK_DEPTH]; //Bit-string of matching bytes between substring and potential matches
#pragma HLS ARRAY_PARTITION variable=match_length_bits complete dim=0
	t_match_size match_length[NUM_BANKS][BANK_DEPTH]; //Integer value of match length between substring and potential matches
#pragma HLS ARRAY_PARTITION variable=match_length complete dim=0
	t_match_size bank_best_length[NUM_BANKS]; //The value of the best match length from each bank
#pragma HLS ARRAY_PARTITION variable=bank_best_length complete dim=1
	ap_int<32> bank_best_position[NUM_BANKS]; //The position of the best match from each bank
#pragma HLS ARRAY_PARTITION variable=bank_best_position complete dim=1
	t_bank_depth bank_best_match[NUM_BANKS]; //The depth number containing the best match from each bank
#pragma HLS ARRAY_PARTITION variable=bank_best_match complete dim=1
	t_match_size window_best_length[WIN_SIZE]; //The value of the best match length for each substring
#pragma HLS ARRAY_PARTITION variable=window_best_length complete dim=1
	ap_int<32> window_best_position[WIN_SIZE]; //The position of the best match for each substring
#pragma HLS ARRAY_PARTITION variable=window_best_position complete dim=1
	t_win_size last_match; //Position of last match in current window. Used to calculate FVP
	t_win_size first_valid_pos = 0; //Marker to keep track of first valid match position in current window
	t_match_size reach[WIN_SIZE]; //How far a match extends within the current window
#pragma HLS ARRAY_PARTITION variable=reach complete dim=1
	ap_uint<32> offsets[WIN_SIZE]; //Offset of each best match found
#pragma HLS ARRAY_PARTITION variable=offsets complete dim=1
	ap_uint<WIN_SIZE> valid_matches_bits; //Bit-string for identifying valid best matches after filtering
	ap_uint<WIN_SIZE> matched_literals_bits; //Bit-string for locating matched literals in current window
	out_array output_array[WIN_SIZE];
#pragma HLS ARRAY_PARTITION variable=output_array complete dim=1
	out_strm output_buff = {0};
	int num_iterations;
	t_match_size bytes_conflicting; //Number of bytes conflicting between two potential matches
	ap_uint<NUM_BANKS> bank_accessed_bits; //Bit-string for recording which dictionary banks have been accessed for writing
	ap_uint<WIN_SIZE> substring_access_bits; //Bit-string for recording which substrings have been granted access to a bank
	t_bank_values associated_bank[WIN_SIZE]; //Contains the bank number accessed by each window substring
#pragma HLS ARRAY_PARTITION variable=associated_bank complete dim=1
	match_t string_to_write[NUM_BANKS]; //String + position to be written to each bank
#pragma HLS ARRAY_PARTITION variable=string_to_write complete dim=1
	t_bank_size string_address[NUM_BANKS]; //Address to write string and position to
#pragma HLS ARRAY_PARTITION variable=string_address complete dim=1
	match_t potential_match[NUM_BANKS][BANK_DEPTH]; //Potential matches read from each bank and depth level
#pragma HLS ARRAY_PARTITION variable=potential_match complete dim=0
	ap_uint<WIN_SIZE*8> substring_ints[WIN_SIZE]; //Integers of each substring in current window
#pragma HLS ARRAY_PARTITION variable=substring_ints complete dim=1

	//Dictionary Initialization
	Dict_Init_Loop_A: for(int i = 0; i < BANK_SIZE; i++){ //For each bank entry
		Dict_Init_Loop_B: for(int j = 0; j < NUM_BANKS; j++){ //For each bank
#pragma HLS UNROLL
			Dict_Init_Loop_C: for(int k = 0; k < BANK_DEPTH; k++){ //For each depth level
#pragma HLS UNROLL
				dictionary[j][k][i].string = 0;
				dictionary[j][k][i].position = -32769;
			}
		}
	}

	num_iterations = (input_size-1)/WIN_SIZE + 1; //Calculate number of iterations (Rounded up)
	//In the first iteration, current window needs to be loaded before shifting order to fill it completely
	strm_in >> input_buff;
	First_Load: for(int i = 0; i < WIN_SIZE; i++){ //Load window with data from input buffer
#pragma HLS UNROLL
		curr_window[i + WIN_SIZE] = input_buff.data((i*8)+7,i*8); //Convert from int to array (Little Endian)
	}

	if(!input_buff.last){ //If first read wasn't last
		//Main Function Loop
		Main_Loop: for(int n = 0; n < (num_iterations-1); n++){ //Perform all iterations except last in loop
#pragma HLS PIPELINE
#pragma HLS DEPENDENCE variable=dictionary inter false

			Window_Shift: for(int i = 0; i < WIN_SIZE; i++){ //Shift second half of window into front half of window
				curr_window[i] = curr_window[i + WIN_SIZE];
			}

			strm_in >> input_buff;
			Load_Window: for(int i = 0; i < WIN_SIZE; i++){ //Load window with data from input buffer
				curr_window[i + WIN_SIZE] = input_buff.data((i*8)+7,i*8); //Convert from int to array (Little Endian)
			}

			Hash: for(int i = 0; i < WIN_SIZE; i++){	//Hash each substring of window
				hash[i] = hash_func(curr_window[i],curr_window[i+1],curr_window[i+2],curr_window[i+3],
						curr_window[i+4]); //Hash first four bytes of each substring
			}

			Pull_Substrings_A: for(int i = 0; i < WIN_SIZE; i++){ //For each substring
				Pull_Substrings_B: for(int j = 0; j < WIN_SIZE; j++){ //For each character in substring
					substring_ints[i]((j*8)+7,j*8) = curr_window[i+j]; //Pull substrings from current window
				}
			}

			bank_accessed_bits = 0; //Reset bank access flag bits
			substring_access_bits = 0; //Reset substring access flag bits
			//Allocate bank access to substrings
			bank_access_func(hash, substring_ints, data_pos, bank_accessed_bits, substring_access_bits, associated_bank, string_to_write, string_address);

			Read_Matches_A: for(int i = 0; i < NUM_BANKS; i++){//For each bank
				Read_Matches_B: for(int j = 0; j < BANK_DEPTH; j++){//For each depth level
					if(bank_accessed_bits[i] == 1){ //If accessed by substring
						potential_match[i][j].string = dictionary[i][j][string_address[i]].string; //Read potential match string
						potential_match[i][j].position = dictionary[i][j][string_address[i]].position; //Read potential match position
					}
				}
			}

			Write_Substrings_A: for(int i = 0; i < NUM_BANKS; i++){//For each bank
				if(bank_accessed_bits[i] == 1){ //If accessed by substring
					dictionary[i][0][string_address[i]].string = string_to_write[i].string; //Write substring to depth 0
					dictionary[i][0][string_address[i]].position = string_to_write[i].position; //Write position to depth 0
					Write_Substrings_B: for(int j = 1; j < BANK_DEPTH; j++){//For each depth level below depth 0
						dictionary[i][j][string_address[i]].string = potential_match[i][j-1].string; //Write substring from depth above
						dictionary[i][j][string_address[i]].position = potential_match[i][j-1].position; //Write position from depth above
					}
				}
			}

			Match_Compare_A: for(int i = 0; i < NUM_BANKS; i++){//For each bank
				Match_Compare_B: for(int j = 0; j < BANK_DEPTH; j++){//For each depth level
					Match_Compare_C: for(int k = 0; k < WIN_SIZE; k++){//For each character
						match_length_bits[i][j][k] = (string_to_write[i].string((k*8)+7,k*8) == potential_match[i][j].string((k*8)+7,k*8)) ? 1 : 0; //Compare strings
					}
				}

			}

			Match_Length_A: for(int i = 0; i < NUM_BANKS; i++){ //For each bank
				Match_Length_B: for(int j = 0; j < BANK_DEPTH; j++){ //For each depth level
					match_length[i][j] = __builtin_ctz((0b1,~match_length_bits[i][j])); //Calculate integer length
				}
			}

			Reset_Best: for(int i = 0; i < NUM_BANKS; i++){ //For each bank
				bank_best_length[i] = 0; //Reset best match lengths from last iteration
			}

			Best_Match_A: for(int i = 0; i < NUM_BANKS; i++){ //For each bank
				Best_Match_B: for(int j = 0; j < BANK_DEPTH; j++){ //For each depth level
					if(match_length[i][j] > bank_best_length[i]){ //Compare matches to find best length
						bank_best_length[i] = match_length[i][j];
						bank_best_match[i] = j;
					}
				}
			}

			Best_Match_MUX: for(int i = 0; i < NUM_BANKS; i++){ //For each bank
				bank_best_position[i] = reg(potential_match[i][bank_best_match[i]].position); //Pass position of best match
			}

			Bank_to_String_MUX: for(int i = 0; i < WIN_SIZE; i++){//For each window substring
				if(substring_access_bits[i] == 1){ //If substring was granted bank access
					window_best_length[i] = bank_best_length[associated_bank[i]]; //Obtain best match length and position from associated bank
					window_best_position[i] = bank_best_position[associated_bank[i]];
				}
				else{ //Otherwise, set match length to 0
					window_best_length[i] = 0;
					window_best_position[i] = 0;
				}
			}

			Calc_Valid: for(int i = 0; i < WIN_SIZE; i++){ //For each best match
				valid_matches_bits[i] = (window_best_length[i] == 0) ? 0 : 1; //If the match length is not 0, the match is valid
			}

			Calc_Offset: for(int i = 0; i < WIN_SIZE; i++){ //For each substring
				offsets[i] = data_pos + i - window_best_position[i];
				if(offsets[i] > MAX_DISTANCE){ //Filter matches that are too far back
					valid_matches_bits[i] = 0;
				}
			}

			Filter_A: for(int i = 0; i < WIN_SIZE; i++){ //For each best match
				if(window_best_length[i] < MIN_LENGTH) //Filter matches with length less than 3
					valid_matches_bits[i] = 0;
			}

			//Filter B
			valid_matches_bits &= ((ap_uint<WIN_SIZE>)0xFFFF << first_valid_pos); //Filter matches covered by previous iteration by clearing bits up to FVP

			last_match = (valid_matches_bits != 0) ? (31 - __builtin_clz(valid_matches_bits)) : 0; //Calculate last match. If no valid matches, set to 0

			//Set matched_literals based on first valid position before updating it
			matched_literals_bits = decoder[first_valid_pos](0,WIN_SIZE-1);

			Calc_Reach: for(int i = 0; i < WIN_SIZE; i++){ //For each best match
				reach[i] = i + window_best_length[i]; //Calculate reach of all matches
			}

			//Calculate first valid position of next iteration using the reach of the last match
			//If reach of last match is greater than 16, calculate FVP. Otherwise FVP is 0.
			first_valid_pos = (reach[last_match][MATCH_SIZE_BITS-1] == 1) ? reach[last_match](MATCH_SIZE_BITS-2,0) : 0;

			//Filter matches with same reach but lower length than others
			Filter_C1: for(int i = 0; i < WIN_SIZE; i++){ //For each match
				if(valid_matches_bits[i] != 0){ //If match is still valid
					Filter_C2: for(int j = i+1; j < WIN_SIZE; j++){ //Compare to all valid matches 'below' this match
						if(reach[i] == reach[j]){ //If matches have same reach, match below has lower length
							if(j == last_match){ //If last match is being filtered for a longer match
								valid_matches_bits(WIN_SIZE-1,i+1) = 0; //Filter all matches below match i
							}
							else{
								valid_matches_bits[j] = 0; //Filter match below
							}
						}
					}
				}
			}

			//Filter or trim matches that conflict with later matches (last match must be kept)
			Filter_D1: for(int i = WIN_SIZE-1; i >= 0; i--){ //For each match (starting from the bottom)
				if(valid_matches_bits[i] != 0){ //If match is still valid
					Filter_D2: for(int j = 0; j < i; j++){ //Compare to all valid matches 'above' this match
						if(reach[j] > i){ //If above matches conflict with this match
							bytes_conflicting = reach[j] - i; //Calculate number of conflicting bytes
							if(window_best_length[j] - bytes_conflicting >= MIN_LENGTH){ //If match j can safely be trimmed (without decreasing length below 3)
								window_best_length[j] -= bytes_conflicting; //Trim match j
							}
							else{
								valid_matches_bits[j] = 0; //Filter match j
							}
						}
					}
				}
			}

			//Preparing output sequence
			Prepare_OutputA: for(int i = 0; i < WIN_SIZE; i++){ //For each box in window
				output_array[i].user = (valid_matches_bits[i] != 0) ? 0b10 : 0b01;
				//If box contains a valid match, set user flag to match. Otherwise set to unmatched literal
			}

			Prepare_OutputB: for(int i = 0; i < WIN_SIZE; i++){ //For each box in window
				if(valid_matches_bits[i] != 0){ //If match is valid
					matched_literals_bits |= (((ap_uint<WIN_SIZE>)decoder[window_best_length[i]] >> 1) << (WIN_SIZE - window_best_length[i] - i));
				} //Turn match length into bit-string and OR all together to find matched literals
			}

			Prepare_OutputC: for(int i = 0; i < WIN_SIZE; i++){ //For each box in window (each bit in window_matches)
				if(matched_literals_bits[WIN_SIZE-1-i] == 1){ //If the box contains a matched literal
					output_array[i].user = 0b00;
				} //Set user flag to 0 for all matched literals
			}

			Prepare_OutputD: for(int i = 0; i < WIN_SIZE; i++){
				switch(output_array[i].user){ //Based on status of output box, fill data accordingly
				case 0b00 : //Matched Literal
					output_array[i].data = 0; //Clear box (Only useful for debugging)
					break;

				case 0b01 : //Unmatched Literal
					output_array[i].data = curr_window[i]; //Write literal
					break;

				case 0b10 : //Match
					assert(window_best_length[i] >= MIN_LENGTH); //Assertions for C debugging
					assert(window_best_length[i] <= WIN_SIZE);
					assert(offsets[i] >= 1);
					assert(offsets[i] <= MAX_DISTANCE);

					output_array[i].data(23, 15) = window_best_length[i]; //Write Length in upper 9 bits
					output_array[i].data(14,  0) = offsets[i] - 1; //Write Distance in lower 15 bits
					break;
				}
			}

			Prepare_OutputE: for(int i = 0; i < WIN_SIZE; i++){ //Convert from array to int (Little Endian)
				output_buff.data((i*24)+23,i*24) = output_array[i].data;
				output_buff.user( (i*2)+ 1, i*2) = output_array[i].user;
			}

			//Write to output stream
			strm_out << output_buff;

			data_pos += WIN_SIZE;	//Increment data marker
		} //For-num_iterations-loop
	}

	Last_Shift: for(int i = 0; i < WIN_SIZE; i++){ //Shift second half of window into front half of window
#pragma HLS UNROLL
		curr_window[i] = curr_window[i + WIN_SIZE];
	}

	//Calculate matched_literals one last time
	matched_literals_bits = decoder[first_valid_pos](0,WIN_SIZE-1);
	matched_literals_bits |= ~(((ap_uint<WIN_SIZE>)input_buff.keep(0,WIN_SIZE-1))); //OR with TKEEP to identify null bytes at end of input stream

	Last_OutputA: for(int i = 0; i < WIN_SIZE; i++){ //For each box in window (each bit in window_matches)
#pragma HLS UNROLL
		output_array[i].user = (matched_literals_bits[WIN_SIZE-1-i] == 1) ? 0b00 : 0b01;
	} //If the box contains a matched literal or null byte, set to 0b00

	Last_OutputB: for(int i = 0; i < WIN_SIZE; i++){
#pragma HLS UNROLL
		switch(output_array[i].user){ //Based on status of output box, fill data accordingly
		case 0b00 : //Matched Literal/Null Character
			output_array[i].data = 0; //Clear box (Only useful for debugging)
			break;

		case 0b01 : //Unmatched Literal
			output_array[i].data = curr_window[i]; //Write literal
			break;
		}
	}

	Last_OutputC: for(int i = 0; i < WIN_SIZE; i++){ //Convert from array to int (Little Endian)
#pragma HLS UNROLL
		output_buff.data((i*24)+23,i*24) = output_array[i].data;
		output_buff.user( (i*2)+ 1, i*2) = output_array[i].user;
	}
	output_buff.last = 1;
	strm_out << output_buff;
}
