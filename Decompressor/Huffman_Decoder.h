#include <stdio.h>
#include <string.h>
#include <ap_int.h>
#include <hls_stream.h>
#include <assert.h>

using namespace hls;

#define MAX_LEN_CODES 286 //Max 286 Length/Literal code lengths
#define MIN_LEN_CODES 257 //Min 257 Length/Literal code lengths (256 Literals and 1 End_of_block code)
#define MAX_DIS_CODES 32  //Max 32 Distance code lengths
#define MAX_CL_CODES  19  //Max 19 Code Length code lengths
#define MAX_LEN_LENGTH 15 //Max length of Length/Literal code is 15 bits
#define MAX_DIS_LENGTH 15 //Max length of Distance code is 15 bits
#define MAX_CL_LENGTH  7  //Max length of Code Length code is 7 bits
#define STATIC_MAX_LEN_LENGTH 9 //Max length of Static Length/Literal code is 9 bits
#define STATIC_MIN_LEN_LENGTH 7 //Min length of Static Length/Literal code is 7 bits
#define STATIC_DIS_LENGTH 5 //Static Distance codes are fixed length 5-bit codes
#define MAX_LEN_EXTRA_BITS 5 //Max number of extra bits following a Length code
#define MAX_DIS_EXTRA_BITS 13 //Max number of extra bits following a Distance code

typedef struct{ //Input AXI-Stream structure
	ap_uint<32> data; //Four bytes of data
	ap_uint<4> keep;  //TKEEP Signals for each byte
	bool last;	   //TLAST AXI-Stream signal
} io_stream;

typedef struct{ //Output (Literal, Length/Distance Pair) AXI-Stream structure
	ap_uint<32> data; //Four bytes of data
	ap_uint<4> keep;  //TKEEP Signals for each byte
	bool user;	   //TUSER Signal to identify Literal(0) or Length-Distance pair(1)
	bool last;	   //TLAST AXI-Stream signal
} lld_stream;

typedef struct{ //Structure for Length symbol table
	ap_uint<3> bits; //Number of extra bits following code. Can be from 0 to 5 for lengths.
	ap_uint<8> base; //Base length value - 3. Lengths 3-258 are encoded in 0-255
} length_symbol_values;

typedef struct{ //Structure for Distance symbol table
	ap_uint<4> bits; //Number of extra bits following code. Can be from 0 to 13 for distances.
	ap_uint<15> base; //Base distance value from 0 to 24577
} distance_symbol_values;

typedef struct{ //Structure for looking up static codes in tables
	ap_uint<8> type; //End-of-block (0110 0000), Literal (0000 0000), or Length/Distance (0001 XXXX) (Where XXXX is number of extra bits)
	ap_uint<8> bits; //Number of bits in code
	ap_uint<16> base; //Base length/distance, or literal value
} static_symbol;
//The above structure was adapted from the 'code' structure used by zlib (defined in "inftrees.h")



