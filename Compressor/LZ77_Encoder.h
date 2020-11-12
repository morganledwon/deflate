#include <stdio.h>
#include <string.h>
#include <ap_int.h>
#include <hls_stream.h>
#include <assert.h>

using namespace hls;

#define WIN_SIZE     16	      			      //Compare window size. Sliding window is twice this
#define NUM_BANKS	 32					      //Number of dictionary banks
#define BANK_DEPTH   3		  			      //Depth of each dictionary bank
#define BANK_INDEXES (512*NUM_BANKS)//16384   //Total number of hash bank indexes
#define BANK_SIZE	 (BANK_INDEXES/NUM_BANKS) //Number of indexes in each bank
#define MIN_LENGTH   3	      			      //Minimum Deflate match length
#define MAX_DISTANCE 32768   			      //Maximum Deflate match distance

//Dynamic bit width definitions (Only work in C sim)
#ifndef __SYNTHESIS__
#define WIN_SIZE_BITS   (int)(log2(WIN_SIZE-1)+1)     //Number of bits required to store WIN_SIZE values
#define BANK_DEPTH_BITS (int)(log2(BANK_DEPTH-1)+1)   //Number of bits required to store BANK_DEPTH values
#define MATCH_SIZE_BITS (int)(log2(WIN_SIZE)+1)       //Number of bits required to store maximum match length (WIN_SIZE)
#define HASH_BITS       (int)(log2(BANK_INDEXES-1)+1) //Number of hash bits required to index BANK_INDEXES (Complete hash function)
#define NUM_BANKS_BITS  (int)(log2(NUM_BANKS-1)+1)    //Number of bits required to index NUM_BANKS values (Top bits of hash function)
#define BANK_SIZE_BITS  (int)(log2(BANK_SIZE-1)+1)    //Number of bits required to index BANK_SIZE values (Bottom bits of hash function)
#else
#define WIN_SIZE_BITS   4
#define BANK_DEPTH_BITS 2
#define MATCH_SIZE_BITS 5
#define HASH_BITS       14
#define NUM_BANKS_BITS  5
#define BANK_SIZE_BITS  9
#endif

typedef ap_uint<WIN_SIZE_BITS> t_win_size;
typedef ap_uint<BANK_DEPTH_BITS> t_bank_depth;
typedef ap_uint<MATCH_SIZE_BITS> t_match_size;
typedef ap_uint<HASH_BITS> t_hash_size;
typedef ap_uint<NUM_BANKS_BITS> t_num_banks;
typedef ap_uint<NUM_BANKS_BITS+1> t_bank_values; //1 extra bit to include the value NUM_BANKS
typedef ap_uint<BANK_SIZE_BITS> t_bank_size;

typedef struct{ //Input AXI-Stream structure
	ap_uint<WIN_SIZE*8> data; //Array of window characters (128 bits with 16 bytes)
	ap_uint<WIN_SIZE>   keep; //TKEEP Signals for each byte
	bool last;
} in_strm;

typedef struct{ //Output Array structure
	ap_uint<24> data; //3-byte box for one of the window characters from LZ77 encoder
	ap_uint<2> user;  //2-bit flag for identifying a box as a literal, length, distance, or matched literal
} out_array;

typedef struct{ //Output AXI-Stream structure
	ap_uint<3*WIN_SIZE*8> data; //3-byte 'boxes' for each of the window characters from LZ77 encoder
	ap_uint<2*WIN_SIZE> user;	//2-bit flags for identifying each box as a literal, length, distance, or matched literal
	bool last;					//AXI-Stream TLAST signal
} out_strm;

typedef struct{ //Match Structure
	ap_uint<WIN_SIZE*8> string;   //16-byte string, int form
	ap_int<32>			position; //Position of string in history
} match_t;

//16-bit decoder. Given an integer index n, returns a bit string containing n ones.
const ap_uint<16> decoder[17] = {
		/* 0*/ 0b0000000000000000,
		/* 1*/ 0b0000000000000001,
		/* 2*/ 0b0000000000000011,
		/* 3*/ 0b0000000000000111,
		/* 4*/ 0b0000000000001111,
		/* 5*/ 0b0000000000011111,
		/* 6*/ 0b0000000000111111,
		/* 7*/ 0b0000000001111111,
		/* 8*/ 0b0000000011111111,
		/* 9*/ 0b0000000111111111,
		/*10*/ 0b0000001111111111,
		/*11*/ 0b0000011111111111,
		/*12*/ 0b0000111111111111,
		/*13*/ 0b0001111111111111,
		/*14*/ 0b0011111111111111,
		/*15*/ 0b0111111111111111,
		/*16*/ 0b1111111111111111
};
