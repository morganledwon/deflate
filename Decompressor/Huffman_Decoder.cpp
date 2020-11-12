#include "Huffman_Decoder.h"
#include "symbol_tables.cpp"
#include "static_decoding_tables.cpp"

//Function for filling accumulator with bits from input stream. Input variable "num" is the number of bits to obtain.
//If accumulator already contains the number of needed bits, no more are read.
void fetch_bits(
		int num,				    //Input
		stream<io_stream> &in_strm, //Input
		io_stream &in_buff,		    //In/Out
		ap_uint<8> &bit_count,		    //In/Out
		ap_uint<64> &acc			    //In/Out
){
#pragma HLS INLINE
	//Inlining function automatically optimizes function usage to read one byte or two depending on input num
	if((bit_count < num) & !in_buff.last){ //If bit_count is still less than num and TLAST hasn't been asserted, read another byte
		in_strm >> in_buff;	//Read 1 byte from input stream into buffer
		acc |= (ap_uint<64>)in_buff.data << bit_count;  //Append data to the left of current bits in acc. Assuming little endian data
		bit_count += 32; //Update number of bits in acc
	}
}

//Reset code length code length counts. Takes 1 cycle
void reset_CL_codelength_count(ap_uint<8> CL_codelength_count[MAX_CL_LENGTH+1]){
#pragma HLS INLINE off
	for(int i = 0; i < (MAX_CL_LENGTH+1); i++){
#pragma HLS UNROLL
		CL_codelength_count[i] = 0;
	}
}

//Reset code length code lengths. Takes 1 cycle
void reset_CL_codelengths(ap_uint<3> CL_codelengths[MAX_CL_CODES]){
#pragma HLS INLINE off
	for(int i = 0; i < MAX_CL_CODES; i++){
#pragma HLS UNROLL
		CL_codelengths[i] = 0;
	}
}

//Reset length and distance code length counts. Takes 1 cycle
void reset_LD_codelength_count(ap_uint<9> length_codelength_count[MAX_LEN_LENGTH+1], ap_uint<9> distance_codelength_count[MAX_DIS_LENGTH+1]){
#pragma HLS INLINE off
	for(int i = 0; i < (MAX_LEN_LENGTH+1); i++){
#pragma HLS UNROLL
		length_codelength_count[i] = 0;
		distance_codelength_count[i] = 0;
	}
}

//Reset length code lengths. Takes 29 cycles
void reset_length_codelengths(ap_uint<4> length_codelengths[MAX_LEN_CODES]){
#pragma HLS INLINE off
	for(int i = MIN_LEN_CODES; i < MAX_LEN_CODES; i++){ //Only last 30 indexes need to be cleared, all others will always be filled
		//#pragma HLS UNROLL
		length_codelengths[i] = 0;
	}
}

//Reset distance code lengths. Takes 32 cycles
void reset_distance_codelengths(ap_uint<4> distance_codelengths[MAX_DIS_CODES]){
#pragma HLS INLINE off
	for(int i = 0; i < MAX_DIS_CODES; i++){
		//#pragma HLS UNROLL
		distance_codelengths[i] = 0;
	}
}

//Build length code table. Takes 286 cycles
void build_length_table(
		ap_uint<4> length_codelengths[MAX_LEN_CODES],    //Input
		ap_uint<9> length_code_table[MAX_LEN_CODES],     //Output
		ap_uint<9> next_length_address[MAX_LEN_LENGTH+1] //Input
){
#pragma HLS INLINE off
	ap_uint<4> length_codelength; //Code length of encoded Length/Literal
	L_Table_LoopC: for(int i = 0; i < MAX_LEN_CODES; i++){ //Build Length/Literal Table
#pragma HLS PIPELINE
		length_codelength = length_codelengths[i]; //Read Length/Literal code length
		if(length_codelength != 0){ //If the length of a code is not 0
			length_code_table[next_length_address[length_codelength]] = i; //At coded index, write symbol
			next_length_address[length_codelength]++;
		}
	}
}

//Build distance code table. Takes 32 cycles
void build_distance_table(
		ap_uint<4> distance_codelengths[MAX_DIS_CODES],
		ap_uint<8> distance_code_table[MAX_DIS_CODES],
		ap_uint<8> next_distance_address[MAX_DIS_LENGTH+1]
){
#pragma HLS INLINE off
	ap_uint<4> distance_codelength; //Code length of encoded Distance
	D_Table_LoopC: for(int i = 0; i < MAX_DIS_CODES; i++){ //Build Distance Table
#pragma HLS PIPELINE
		distance_codelength = distance_codelengths[i]; //Read Distance code length
		if(distance_codelength != 0){ //If the length of a code is not 0
			distance_code_table[next_distance_address[distance_codelength]] = i; //At coded index, write symbol
			next_distance_address[distance_codelength]++;
		}
	}
}

void decode_static_block(
		stream<io_stream> &in_strm,  //Input
		io_stream &in_buff,		     //In/Out
		ap_uint<8> &bit_count,		     //In/Out
		ap_uint<64> &acc,			     //In/Out
		bool &end_of_block,		     //In/Out
		stream<lld_stream> &out_strm //Output
){
	ap_uint<9> static_code_bits; //9 bits containing a static length/distance code, actual code length may be 7 to 9 bits long
	static_symbol length_symbol; //Decoded Length/Literal Huffman symbol
	static_symbol distance_symbol; //Decoded Distance Huffman symbol
	ap_uint<9> match_length; //Match length variable
	ap_uint<16> match_distance; //Match distance variable
	ap_uint<5> extra_length; //Integer value of extra length
	ap_uint<13> extra_distance; //Integer value of extra distance
	lld_stream out_buff; //Buffer for writing to output stream

	Static_Block_Loop: while(!end_of_block){ //Continue processing a block until end-of-block code found
		fetch_bits(32, in_strm, in_buff, bit_count, acc);
		static_code_bits = acc(8,0); //Take in 9 bits from ACC
		length_symbol = static_length_table[static_code_bits]; //Decode in static literal/length table
		acc >>= length_symbol.bits;	  //Shift out bits from acc
		bit_count -= length_symbol.bits; //Update bit count
		switch (length_symbol.type(7, 4)){ //Check upper four bits of type
		case 0 : //Literal
			out_buff.data = length_symbol.base; //Write Literal to output
			out_buff.keep = 0b0001; //Update TKEEP bits to indicate Literal
			out_buff.user = 0;
			out_buff.last = 0; //Assign TLAST
			out_strm << out_buff; //Write to output stream
			break;
		case 1: // Length/Distance
			extra_length = acc & ((1 << length_symbol.type(3,0)) - 1 ); //Fetch extra length bits from acc
			acc >>= length_symbol.type(3,0); //Discard extra length bits from acc
			bit_count -= length_symbol.type(3,0); //Update bit count accordingly
			match_length = length_symbol.base + extra_length; //Add base and extra to get length. Can be 3-258 (9 bits)

			distance_symbol = static_distance_table[acc(4,0)]; //Decode in static distance table
			acc >>= STATIC_DIS_LENGTH; //Shift out bits from acc
			bit_count -= STATIC_DIS_LENGTH; //Update bit count
			extra_distance = acc & ((1 << distance_symbol.type(3,0)) - 1 ); //Fetch extra distance bits from acc
			acc >>= distance_symbol.type(3,0); //Discard extra distance bits from acc
			bit_count -= distance_symbol.type(3,0); //Update bit count accordingly
			match_distance = distance_symbol.base + extra_distance; //Add base and extra to get distance. Can be 1-32768 (16 bits)

			out_buff.data(31,23) = match_length; //Write 9-bit match length to output
			out_buff.data(15, 0) = match_distance; //Write 16-bit match distance to output
			out_buff.keep = 0b1111; //Update TKEEP bits to indicate length-distance pair
			out_buff.user = 1;
			out_buff.last = 0; //Assign TLAST
			out_strm << out_buff; //Write to output stream
			break;
		case 6: //End-of-Block
			end_of_block = true; //Update EOB flag, exit Block loop and return to Main Loop
			break;
		}
	}
}

void decode_dynamic_block(
		stream<io_stream> &in_strm,   //Input
		io_stream &in_buff,		      //In/Out
		ap_uint<8> &bit_count,		      //In/Out
		ap_uint<64> &acc,			      //In/Out
		bool &end_of_block,		      //In/Out
		stream<lld_stream> &out_strm, //Output
		ap_uint<9> length_codelength_count[MAX_LEN_LENGTH+1],   //Input
		ap_uint<9> distance_codelength_count[MAX_DIS_LENGTH+1], //Input
		ap_uint<15> base_length_values[MAX_LEN_LENGTH+1],		 //Input
		ap_uint<15> base_distance_values[MAX_DIS_LENGTH+1],	 //Input
		ap_uint<9> base_length_address[MAX_LEN_LENGTH+1],		 //Input
		ap_uint<8> base_distance_address[MAX_DIS_LENGTH+1],	 //Input
		ap_uint<9> length_code_table[MAX_LEN_CODES],		     //Input
		ap_uint<8> distance_code_table[MAX_DIS_CODES]		     //Input
){
	ap_uint<15> code_bits; //15 bits containing a length/distance code, actual code length may be 0 to 15 bits long
	ap_uint<16> code_comparison; //Bit array for comparing code_bits to base_length_values
	ap_uint<4> code_length; //Deciphered length of currently held length code
	ap_uint<9> code_address; //Register for calculating length code address
	ap_uint<9> length_symbol; //Decoded Length/Literal Huffman symbol
	ap_uint<5> distance_symbol; //Decoded Distance Huffman symbol
	length_symbol_values length_symbol_extras;
	distance_symbol_values distance_symbol_extras;
	ap_uint<9> match_length; //Match length variable
	ap_uint<16> match_distance; //Match distance variable
	ap_uint<5> extra_length; //Integer value of extra length
	ap_uint<13> extra_distance; //Integer value of extra distance
	lld_stream out_buff; //Buffer for writing to output stream

	Dynamic_Block_Loop: while(!end_of_block){ //Continue processing a block until end-of-block code found
		fetch_bits(MAX_LEN_LENGTH, in_strm, in_buff, bit_count, acc); //Fetch 15 bits for Length/Literal code
		code_bits = acc(0,14); //Take in 15 bits from ACC and reverse them
		Length_Code_Compare_Loop: for(int i = 1; i <= MAX_LEN_LENGTH; i++){ //Compare bits to every length base_value to find code length of bits
#pragma HLS UNROLL
			code_comparison[i] = (length_codelength_count[i] != 0) ? (code_bits >= (base_length_values[i] << (MAX_LEN_LENGTH - i))) : 0;
		} //Comparisons with lengths that aren't used are disabled (They would always be 1 since base value would be 0)
		code_length = MAX_LEN_LENGTH - (__builtin_clz(code_comparison) - 16); //Code length is bit position of highest passing comparison. CLZ adds 16 bits
		code_bits >>= (MAX_LEN_LENGTH - code_length); //Shift bits out so only codelength remain
		code_address = base_length_address[code_length] + (code_bits - base_length_values[code_length]); //Address = base + offset
		length_symbol = length_code_table[code_address]; //Decode length code in lookup table
		acc >>= code_length;	  //Shift out bits from acc
		bit_count -= code_length; //Update bit count
		if(length_symbol < 256){ //If decoded Length/Literal symbol is from 0 to 255, it is a Literal
			out_buff.data = length_symbol; //Write Literal to output
			out_buff.keep = 0b0001; //Update TKEEP bits to indicate Literal
			out_buff.user = 0;
			out_buff.last = 0; //Assign TLAST
			out_strm << out_buff; //Write to output stream
		}
		else if(length_symbol == 256){ //End-of-block symbol
			end_of_block = true; //Update EOB flag, exit Block loop and return to Main Loop
		}
		else{ //If length_symbol > 256, It is a Length
			length_symbol_extras = length_symbol_table[length_symbol-MIN_LEN_CODES]; //Lookup base length and extra bits in table
			fetch_bits(MAX_LEN_EXTRA_BITS+MAX_DIS_LENGTH, in_strm, in_buff, bit_count, acc); //Fetch 20 bits for Length code extra bits and distance
			extra_length = acc & ((1 << length_symbol_extras.bits) - 1 ); //Fetch extra bits from acc
			acc >>= length_symbol_extras.bits; //Discard extra bits from acc
			bit_count -= length_symbol_extras.bits; //Update bit count accordingly
			match_length = length_symbol_extras.base + extra_length + 3; //Add base and extra to get length. Base Lengths 3-258 are encoded in 0-255 (8 bits) so we add 3

			code_bits = acc(0,14); //Take in 15 bits from ACC and reverse them
			Distance_Code_Compare_Loop: for(int i = 1; i <= MAX_DIS_LENGTH; i++){ //Compare bits to every distance base_value to find code length of bits
#pragma HLS UNROLL
				code_comparison[i] = (distance_codelength_count[i] != 0) ? (code_bits >= (base_distance_values[i] << (MAX_DIS_LENGTH - i))) : 0;
			} //Comparisons with lengths that aren't used are disabled (They would always be 1 since base value would be 0)
			code_length = MAX_DIS_LENGTH - (__builtin_clz(code_comparison) - 16); //Code length is bit position of highest passing comparison. CLZ adds 16 bits
			code_bits >>= (MAX_DIS_LENGTH - code_length); //Shift bits out so only code length remain
			code_address = base_distance_address[code_length] + (code_bits - base_distance_values[code_length]); //Address = base + offset
			distance_symbol = distance_code_table[code_address]; //Decode distance code in lookup table
			acc >>= code_length;	  //Shift out bits from acc
			bit_count -= code_length; //Update bit count
			distance_symbol_extras = distance_symbol_table[distance_symbol]; //Lookup base distance and extra bits in table
			fetch_bits(MAX_DIS_EXTRA_BITS, in_strm, in_buff, bit_count, acc); //Fetch 13 bits for Distance code extra bits
			extra_distance = acc & ((1 << distance_symbol_extras.bits) - 1 ); //Fetch extra bits from acc
			acc >>= distance_symbol_extras.bits; //Discard extra bits from acc
			bit_count -= distance_symbol_extras.bits; //Update bit count accordingly
			match_distance = distance_symbol_extras.base + extra_distance; //Add base and extra to get distance. Can be 1-32768 (16 bits)

			out_buff.data(31,23) = match_length; //Write 9-bit match length to output
			out_buff.data(15, 0) = match_distance; //Write 16-bit match distance to output
			out_buff.keep = 0b1111; //Update TKEEP bits to indicate length-distance pair
			out_buff.user = 1;
			out_buff.last = 0; //Assign TLAST
			out_strm << out_buff; //Write to output stream
		}
	}
}

void build_dynamic_tables(
		stream<io_stream> &in_strm, //Input
		io_stream &in_buff,		    //In/Out
		ap_uint<8> &bit_count,		    //In/Out
		ap_uint<64> &acc,			    //In/Out
		ap_uint<4> length_codelengths[MAX_LEN_CODES],			 //Input
		ap_uint<4> distance_codelengths[MAX_DIS_CODES],		 //Input
		ap_uint<9> length_codelength_count[MAX_LEN_LENGTH+1],   //Output
		ap_uint<9> distance_codelength_count[MAX_DIS_LENGTH+1], //Output
		ap_uint<15> base_length_values[MAX_LEN_LENGTH+1],		 //Output
		ap_uint<15> base_distance_values[MAX_DIS_LENGTH+1],	 //Output
		ap_uint<9> base_length_address[MAX_LEN_LENGTH+1],		 //Output
		ap_uint<8> base_distance_address[MAX_DIS_LENGTH+1],	 //Output
		ap_uint<9> length_code_table[MAX_LEN_CODES],		     //Output
		ap_uint<8> distance_code_table[MAX_DIS_CODES]		     //Output
){
	ap_uint<9> num_length_codelengths; //Number of Length/Literal code lengths in sequence, from 257 to 286
	ap_uint<8> num_distance_codelengths; //Number of Distance code lengths in sequence, from 1 to 32
	ap_uint<8> num_codelength_codelengths; //Number of Code_Length code lengths in sequence, from 4 to 19
	ap_uint<3> CL_codelength; //Code length of encoded Code Length. CLCLs can be from 0 to 7 bits long
	ap_uint<3> CL_codelengths[MAX_CL_CODES]; //Array for storing CL code lengths
#pragma HLS ARRAY_PARTITION variable=CL_codelengths complete dim=1
	ap_uint<8> CL_codelength_count[MAX_CL_LENGTH+1]; //Array for counting number of times each CLCL occurs
#pragma HLS ARRAY_PARTITION variable=CL_codelength_count complete dim=1
	ap_uint<7> base_CL_values[MAX_CL_LENGTH+1]; //Calculated base value for each CLCL
#pragma HLS ARRAY_PARTITION variable=base_CL_values complete dim=1
	ap_uint<5> base_CL_address[MAX_CL_LENGTH+1]; //Base address for each CLCL base value
#pragma HLS ARRAY_PARTITION variable=base_CL_address complete dim=1
	ap_uint<5> next_CL_address[MAX_CL_LENGTH+1]; //Contains the next address for each CLC symbol
#pragma HLS ARRAY_PARTITION variable=next_CL_address complete dim=1
	ap_uint<5> codelength_address; //Register for calculating code_length code address
	ap_uint<5> CL_code_table[MAX_CL_CODES]; //Table containing symbols for 19 CL Codes. Codes are indexed using base_CL_addresses
	ap_uint<9> codelengths_received; //For counting number of code lengths read from sequence
	ap_uint<7> codelength_bits; //7 bits containing a length/distance code length code, actual code length may be 0 to 7 bits long
	ap_uint<8> CL_comparison; //Bit array for comparing codelength_bits to base_CL_values
	ap_uint<3> codelength_length; //Deciphered length of currently held code length code
	ap_uint<5> CL_symbol; //Decoded Code Length symbol (Can be 0 to 18)
	ap_uint<9> length_symbol_counter; //Index counter for current Length/Literal symbol in sequence
	ap_uint<5> distance_symbol_counter; //Index counter for current Distance symbol in sequence
	ap_uint<9> next_length_address[MAX_LEN_LENGTH+1]; //Contains the next address for each Length/Literal symbol
#pragma HLS ARRAY_PARTITION variable=next_length_address complete dim=1
	ap_uint<8> next_distance_address[MAX_DIS_LENGTH+1]; //Contains the next address for each Distance symbol
#pragma HLS ARRAY_PARTITION variable=next_distance_address complete dim=1
	ap_uint<5> previous_CL; //Register for storing found_CL for next iteration
	ap_uint<8> repeat_value; //Used by code length symbols 16, 17, and 18 for storing number of repetitions to perform

	reset_CL_codelength_count(CL_codelength_count);
	reset_CL_codelengths(CL_codelengths);
	reset_LD_codelength_count(length_codelength_count, distance_codelength_count);

	//Read code length codes and build code length table
	fetch_bits(14, in_strm, in_buff, bit_count, acc);
	num_length_codelengths = acc(4,0) + 257; //There will always be at least 257 Length/Literal codes for the 256 Literals and the EOB code
	num_distance_codelengths = acc(9,5) + 1; //There will always be at least 1 distance code
	num_codelength_codelengths = acc(13,10) + 4; //There will always be at least 4 code length code lengths (for symbols 16, 17, 18, and 0)
	assert(num_length_codelengths < 287); //Max 286 Length/Literal code lengths
	assert(num_distance_codelengths < 33); //Max 32 Distance code lengths
	assert(num_codelength_codelengths < 20); //Max 19 Code Length code lengths
	acc >>= 14;		  //Shift out bits from acc
	bit_count -= 14;  //Update bit count

	//Build code_length table
	//1: Count number of codes for each length
	CL_Table_LoopA: for(int i = 0; i < num_codelength_codelengths; i++){
#pragma HLS PIPELINE
		fetch_bits(3, in_strm, in_buff, bit_count, acc);
		CL_codelength = acc(2,0); //Read 3-bit code length
		CL_codelengths[permuted_order[i]] = CL_codelength; //Store code length in array in permuted order
		CL_codelength_count[CL_codelength]++; //Take count of number of code lengths
		acc >>= 3;		  //Shift out bits from acc
		bit_count -= 3;	  //Update bit count
	}
	//2: Calculate base value for each code length
	CL_codelength_count[0] = 0; //Set back to 0 before calculating base values (Codes with length of 0 are unused)
	base_CL_values[0] = 0;
	base_CL_address[0] = 0;
	CL_Table_LoopB: for(int i = 1; i <= MAX_CL_LENGTH; i++){ //For each possible code length
#pragma HLS PIPELINE
		base_CL_values[i] = (base_CL_values[i-1] + CL_codelength_count[i-1]) << 1; //Base value for a code length is based on number of previous code lengths
		base_CL_address[i] = base_CL_address[i-1] + CL_codelength_count[i-1]; //Base address is "number of code lengths" away from previous base address
		next_CL_address[i] = base_CL_address[i]; //Initialize "next address" to base address for this length
	}
	//3: Assign consecutive values (addresses) to each code for all code lengths
	CL_Table_LoopC: for(int i = 0; i < MAX_CL_CODES; i++){ //For all 19 code length symbols
#pragma HLS PIPELINE
		CL_codelength = CL_codelengths[i]; //Read CL code length
		if(CL_codelength != 0){ //If the length of a code is not 0
			CL_code_table[next_CL_address[CL_codelength]] = i;
			next_CL_address[CL_codelength]++;
		}
	}

	//Read encoded code length sequence and decode it using that table
	codelengths_received = 0; //Reset code lengths received
	length_symbol_counter = 0; //Reset Length/Literal array index pointer
	distance_symbol_counter = 0; //Reset Distance array index pointer
	LD_Table_LoopA: while(codelengths_received < (num_length_codelengths+num_distance_codelengths)){ //Retrieve all code lengths in sequence
		//Sequence can be from 258 to 318 code lengths long
		fetch_bits(7, in_strm, in_buff, bit_count, acc);
		codelength_bits = acc(0,6); //Take in 7 bits from ACC and reverse them
		CL_Comparison_Loop: for(int i = 1; i <= MAX_CL_LENGTH; i++){ //Compare bits to every length base_value to find code length of bits
#pragma HLS UNROLL
			CL_comparison[i] = (CL_codelength_count[i] != 0) ? (codelength_bits >= (base_CL_values[i] << (MAX_CL_LENGTH - i))) : 0;
		} //Comparisons with lengths that aren't used are disabled (They would always be 1 since base value would be 0)
		codelength_length = MAX_CL_LENGTH - (__builtin_clz(CL_comparison) - 24); //Code length is bit position of highest passing comparison. CLZ adds 24 bits
		codelength_bits >>= (MAX_CL_LENGTH - codelength_length); //Shift bits out so only codelength remain
		codelength_address = base_CL_address[codelength_length] + (codelength_bits - base_CL_values[codelength_length]);
		CL_symbol = CL_code_table[codelength_address]; //Decode code length in lookup table
		acc >>= codelength_length;		 //Shift out bits from acc
		bit_count -= codelength_length; //Update bit count
		if(CL_symbol <= 15){ //If decoded code length symbol is from 0 to 15
			if(codelengths_received < num_length_codelengths){ //If Length code length
				length_codelengths[length_symbol_counter] = CL_symbol; //Current symbol in sequence has code length of found_CL
				length_codelength_count[CL_symbol]++; //Count code length
				length_symbol_counter++; //Increment symbol pointer
			}
			else{ //If Distance code length
				distance_codelengths[distance_symbol_counter] = CL_symbol; //Current symbol in sequence has code length of found_CL
				distance_codelength_count[CL_symbol]++; //Count code length
				distance_symbol_counter++;
			}
			codelengths_received++; //Increment code length counter
			previous_CL = CL_symbol; //Save code length for next iteration
		}
		else{
			switch(CL_symbol){
			case 16 : //Symbol 16: Copy previous code length 3 to 6 times
				fetch_bits(2, in_strm, in_buff, bit_count, acc); //Fetch 2 extra bits containing repeat value
				repeat_value = acc(1,0) + 3;
				acc >>= 2;		 //Shift out bits from acc
				bit_count -= 2; //Update bit count
				Copy_Previous_CL_Loop: for(int i = 0; i < repeat_value; i++){ //For 3 to 6 times
					//#pragma HLS PIPELINE
					if(codelengths_received < num_length_codelengths){ //If Length code length
						length_codelengths[length_symbol_counter] = previous_CL; //Current symbol in sequence has code length of previous_CL
						length_codelength_count[previous_CL]++; //Count code length
						length_symbol_counter++; //Increment symbol pointer
					}
					else{ //If Distance code length
						distance_codelengths[distance_symbol_counter] = previous_CL; //Current symbol in sequence has code length of previous_CL
						distance_codelength_count[previous_CL]++; //Count code length
						distance_symbol_counter++;
					}
					codelengths_received++; //Increment code length counter
				}
				break;
			case 17 : //Symbol 17: Repeat code length of zero 3 to 10 times
				fetch_bits(3, in_strm, in_buff, bit_count, acc); //Fetch 3 extra bits containing repeat value
				repeat_value = acc(2,0) + 3;
				acc >>= 3;		 //Shift out bits from acc
				bit_count -= 3; //Update bit count
				Repeat_Zero_CL_LoopA: for(int i = 0; i < repeat_value; i++){ //For 3 to 10 times
					//#pragma HLS PIPELINE
					if(codelengths_received < num_length_codelengths){ //If Length code length
						length_codelengths[length_symbol_counter] = 0; //Current symbol in sequence has code length of 0
						length_symbol_counter++; //Increment symbol pointer
					}
					else{ //If Distance code length
						distance_codelengths[distance_symbol_counter] = 0; //Current symbol in sequence has code length of 0
						distance_symbol_counter++; //Increment symbol pointer
					}
					codelengths_received++; //Increment code length counter
				}
				break;
			case 18 : //Symbol 18: Repeat code length of zero 11 to 138 times
				fetch_bits(7, in_strm, in_buff, bit_count, acc); //Fetch 7 extra bits containing repeat value
				repeat_value = acc(6,0) + 11;
				acc >>= 7;		 //Shift out bits from acc
				bit_count -= 7; //Update bit count
				Repeat_Zero_CL_LoopB: for(int i = 0; i < repeat_value; i++){ //For 11 to 138 times
					//#pragma HLS PIPELINE
					if(codelengths_received < num_length_codelengths){ //If Length code length
						length_codelengths[length_symbol_counter] = 0; //Current symbol in sequence has code length of 0
						length_symbol_counter++; //Increment symbol pointer
					}
					else{ //If Distance code length
						distance_codelengths[distance_symbol_counter] = 0; //Current symbol in sequence has code length of 0
						distance_symbol_counter++; //Increment symbol pointer
					}
					codelengths_received++; //Increment code length counter
				}
				break;
			default :
				//Shouldn't occur, misread codes should be mostly 0s, could be other values from previous iterations if tables arent cleared.
				break;
			}
		}
	}

	//When all length and distance code lengths are received
	length_codelength_count[0] = 0; //Set back to 0 before calculating base values (Codes with length of 0 are unused)
	distance_codelength_count[0] = 0;
	base_length_values[0] = 0;
	base_length_address[0] = 0;
	base_distance_values[0] = 0;
	base_distance_address[0] = 0;
	//Calculate Base Values and Base Addresses
	LD_Table_LoopB: for(int i = 1; i <= MAX_LEN_LENGTH; i++){ //For each possible code length
#pragma HLS PIPELINE
		base_length_values[i] = (base_length_values[i-1] + length_codelength_count[i-1]) << 1;
		base_length_address[i] = base_length_address[i-1] + length_codelength_count[i-1];
		next_length_address[i] = base_length_address[i];
		base_distance_values[i] = (base_distance_values[i-1] + distance_codelength_count[i-1]) << 1;
		base_distance_address[i] = base_distance_address[i-1] + distance_codelength_count[i-1];
		next_distance_address[i] = base_distance_address[i];
	}

	//Build code tables from code lengths
	//Separated as functions to allow both to execute simultaneously
	build_length_table(length_codelengths, length_code_table, next_length_address);
	build_distance_table(distance_codelengths, distance_code_table, next_distance_address);
}

ap_uint<2> full_huffman_decoder(
		stream<io_stream> &in_strm,
		stream<lld_stream> &out_strm
){
#pragma HLS INTERFACE ap_ctrl_hs register port=return
#pragma HLS INTERFACE axis off port=out_strm
#pragma HLS INTERFACE axis off port=in_strm
	ap_uint<64> acc = 0; //Accumulator for storing up to 32 bits at a time
	ap_uint<8> bit_count = 0; //Number of bits currently in the acc
	bool block_final = false; //Flag for BFINAL block header
	ap_uint<2> block_type; //Flag for BTYPE block header
	io_stream in_buff = {0}; //Buffer for reading input stream. It is passed to functions so they can check TLAST before reading from stream.
	bool end_of_block; //End-of-block flag
	ap_uint<16> block_length;
	ap_uint<16> block_Nlength;
	static ap_uint<4> length_codelengths[MAX_LEN_CODES] = {0}; //Array of code lengths for each Length/Literal symbol
	static ap_uint<4> distance_codelengths[MAX_DIS_CODES] = {0}; //Array of code lengths for each Distance symbol
	ap_uint<9> length_codelength_count[MAX_LEN_LENGTH+1]; //Array for counting number of times each code length occurs
#pragma HLS ARRAY_PARTITION variable=length_codelength_count complete dim=1
	ap_uint<9> distance_codelength_count[MAX_DIS_LENGTH+1]; //Array for counting number of times each code length occurs
#pragma HLS ARRAY_PARTITION variable=distance_codelength_count complete dim=1
	ap_uint<15> base_length_values[MAX_LEN_LENGTH+1]; //Calculated base value for each Length/Literal code length
#pragma HLS ARRAY_PARTITION variable=base_length_values complete dim=1
	ap_uint<15> base_distance_values[MAX_DIS_LENGTH+1]; //Calculated base value for each Distance code length
#pragma HLS ARRAY_PARTITION variable=base_distance_values complete dim=1
	ap_uint<9> base_length_address[MAX_LEN_LENGTH+1]; //Base address for each Length/Literal code length base value
#pragma HLS ARRAY_PARTITION variable=base_length_address complete dim=1
	ap_uint<8> base_distance_address[MAX_DIS_LENGTH+1]; //Base address for each Distance code length base value
#pragma HLS ARRAY_PARTITION variable=base_distance_address complete dim=1
	ap_uint<9> length_code_table[MAX_LEN_CODES]; //For looking up Length/Literal symbol of a code.
	ap_uint<8> distance_code_table[MAX_DIS_CODES]; //For looking up Distance symbol of a code.
	lld_stream out_buff = {0}; //Buffer for writing to output stream

	Main_Loop: while(!block_final){ //Continue processing Deflate blocks until final block is finished
		end_of_block = false;	   	  //Reset end-of-block flag
		fetch_bits(3, in_strm, in_buff, bit_count, acc);
		block_final = acc[0];   //Retrieve BFINAL bit
		block_type = acc(2, 1); //Retrieve BTYPE bits
		acc >>= 3;		  //Shift out bits from acc
		bit_count -= 3;	  //Update bit count

		switch(block_type){
		case 0b00 : //Stored Block
			acc >>= (bit_count % 8);      //Discard 0 to 7 remaining bits to next byte boundary in acc
			bit_count -= (bit_count % 8); //Update bit count

			fetch_bits(32, in_strm, in_buff, bit_count, acc);
			block_length(15,8)  = acc(7,0); //Read upper byte of LEN
			block_length(7,0)   = acc(15,8); //Read lower byte of LEN
			block_Nlength(15,8) = acc(23,16); //Read upper byte of NLEN
			block_Nlength(7,0)  = acc(31,24); //Read lower byte of NLEN
			if(block_length != ~block_Nlength){ //Check if LEN is 1s compliment of NLEN
				return 1;
			}
			else{
				acc >>= 32;		  //Shift out bits from acc
				bit_count -= 32;	  //Update bit count
				Stream_Stored_Block_Loop: for(int i = 0; i < block_length-4; i+=4){ //Pass LEN number of bytes through
#pragma HLS PIPELINE
					in_strm >> in_buff;
					out_buff.data = in_buff.data;
					out_buff.keep = 0b1111;
					out_buff.user = 0;
					out_buff.last = 0;
					out_strm << out_buff;
				}
				fetch_bits(32, in_strm, in_buff, bit_count, acc);

				switch(block_length % 4){ //Write remaining amount of bytes on last iteration (1-4)
				case 0 : //4 remaining bytes
					out_buff.data = acc(31,0);
					out_buff.keep = 0b1111;
					acc >>= 32;		 //Shift out bits from acc
					bit_count -= 32; //Update bit count
					break;
				case 1 : //1 remaining byte
					out_buff.data = acc(7,0);
					out_buff.keep = 0b0001;
					acc >>= 8;		//Shift out bits from acc
					bit_count -= 8;	//Update bit count
					break;
				case 2 : //2 remaining bytes
					out_buff.data = acc(15,0);
					out_buff.keep = 0b0011;
					acc >>= 16;		 //Shift out bits from acc
					bit_count -= 16; //Update bit count
					break;
				case 3 : //3 remaining bytes
					out_buff.data = acc(23,0);
					out_buff.keep = 0b0111;
					acc >>= 24;		 //Shift out bits from acc
					bit_count -= 24; //Update bit count
					break;
				}
				out_buff.user = 0;
				out_buff.last = 0; //Assign TLAST
				out_strm << out_buff;
			}
			break;

		case 0b01 : //Static Compressed Block
			decode_static_block(in_strm, in_buff, bit_count, acc, end_of_block, out_strm);
			break;

		case 0b10 : //Dynamic Compressed Block
			build_dynamic_tables( in_strm, in_buff, bit_count, acc,
					length_codelengths,		 distance_codelengths,
					length_codelength_count, distance_codelength_count,
					base_length_values,  	 base_distance_values,
					base_length_address, 	 base_distance_address,
					length_code_table,   	 distance_code_table);
			decode_dynamic_block( in_strm, in_buff, bit_count, acc, end_of_block, out_strm,
					length_codelength_count, distance_codelength_count,
					base_length_values,  	 base_distance_values,
					base_length_address, 	 base_distance_address,
					length_code_table,   	 distance_code_table
			);
			reset_length_codelengths(length_codelengths); //Reset code length arrays while dynamic decoding takes place
			reset_distance_codelengths(distance_codelengths);
			break;

		case 0b11 : //Reserved (Error)
			return 2;
			break;
		}
	} //After last block has been processed
	out_buff.keep = 0; //Lower TKEEP signal
	out_buff.last = 1; //Raise TLAST signal
	out_strm << out_buff;
	return 0;
}
