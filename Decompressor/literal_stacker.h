#include <stdio.h>
#include <string.h>
#include <ap_int.h>
#include <hls_stream.h>
#include <assert.h>

using namespace hls;

//Stream Format:
//TDATA: | 31 - Byte3 - 24 | 23 - Byte2 - 16 | 15 - Byte1 - 8 | 7 - Byte0 - 0 |
//TKEEP: |	    TKEEP3     |	  TKEEP2	 |	    TKEEP1	  |	    TKEEP0    |
typedef struct{ //Literal, Length/Distance stream structure
	ap_uint<32> data; //Four bytes of data
	ap_uint<4> keep;  //TKEEP Signals for each byte
	bool user;	   //TUSER Signal to identify Literal(0) or Length-Distance pair(1)
	bool last;	   //TLAST AXI-Stream signal
} lld_stream;

