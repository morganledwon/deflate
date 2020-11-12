#include <stdio.h>
#include <string.h>
#include <ap_int.h>
#include <hls_stream.h>
#include <assert.h>

using namespace hls;

#define WINDOW_SIZE   16	//Number of boxes in input window
#define WINDOW_BITS   WINDOW_SIZE*8
#define ENC_WIN_BITS  (WINDOW_SIZE-1)*9 + 26 //Bit width of encoded window. Longest possible enc_win is 15 9-bit literals and 1 26-bit LD pair = 161 bits.
#define OUT_STRM_BITS 2*WINDOW_BITS //Output stream bit width, must be power of 2 and larger than encoded window width
#define OUT_WIN_BITS  2*OUT_STRM_BITS //Bit width of output window packer, must be double that of output stream width

typedef ap_uint<ENC_WIN_BITS> t_enc_win;

typedef struct{ //Input AXI-Stream structure
	ap_uint<3*WINDOW_BITS> data;  //3-byte DATA 'boxes' for each of the window characters from LZ77 encoder
	ap_uint<2*WINDOW_SIZE> user;  //2-bit TUSER flags for identifying each box as a literal, length, distance, or matched literal
	bool last;					  //TLAST signal
} in_stream;

typedef struct{ //Output AXI-Stream structure
	ap_uint<OUT_STRM_BITS>   data; //Compressed data output
	ap_uint<OUT_STRM_BITS/8> keep; //TKEEP signal for each byte of data
	bool last;					   //TLAST signal
} out_stream;

typedef struct{ //Distance code table structure
	ap_uint<5>  code;
	ap_uint<15> base;
	ap_uint<4>  bits;
} distance_code;

//32-bit decoder. Given an integer index n, returns a bit string containing n ones.
const ap_uint<32> decoder32[33] = {
		/* 0*/ 0b00000000000000000000000000000000,
		/* 1*/ 0b00000000000000000000000000000001,
		/* 2*/ 0b00000000000000000000000000000011,
		/* 3*/ 0b00000000000000000000000000000111,
		/* 4*/ 0b00000000000000000000000000001111,
		/* 5*/ 0b00000000000000000000000000011111,
		/* 6*/ 0b00000000000000000000000000111111,
		/* 7*/ 0b00000000000000000000000001111111,
		/* 8*/ 0b00000000000000000000000011111111,
		/* 9*/ 0b00000000000000000000000111111111,
		/*10*/ 0b00000000000000000000001111111111,
		/*11*/ 0b00000000000000000000011111111111,
		/*12*/ 0b00000000000000000000111111111111,
		/*13*/ 0b00000000000000000001111111111111,
		/*14*/ 0b00000000000000000011111111111111,
		/*15*/ 0b00000000000000000111111111111111,
		/*16*/ 0b00000000000000001111111111111111,
		/*17*/ 0b00000000000000011111111111111111,
		/*18*/ 0b00000000000000111111111111111111,
		/*19*/ 0b00000000000001111111111111111111,
		/*20*/ 0b00000000000011111111111111111111,
		/*21*/ 0b00000000000111111111111111111111,
		/*22*/ 0b00000000001111111111111111111111,
		/*23*/ 0b00000000011111111111111111111111,
		/*24*/ 0b00000000111111111111111111111111,
		/*25*/ 0b00000001111111111111111111111111,
		/*26*/ 0b00000011111111111111111111111111,
		/*27*/ 0b00000111111111111111111111111111,
		/*28*/ 0b00001111111111111111111111111111,
		/*29*/ 0b00011111111111111111111111111111,
		/*30*/ 0b00111111111111111111111111111111,
		/*31*/ 0b01111111111111111111111111111111,
		/*32*/ 0b11111111111111111111111111111111
};
