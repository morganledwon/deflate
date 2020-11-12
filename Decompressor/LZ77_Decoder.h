
#include <stdio.h>
#include <string.h>
#include <ap_int.h>
#include <hls_stream.h>
#include <assert.h>

using namespace hls;

typedef struct{ //Input AXI-Stream structure
	ap_uint<32> data; //Four bytes of data
	ap_uint<4> keep;  //TKEEP Signals for each byte
	bool user;	   //TUSER Signal to identify Literal(0) or Length-Distance pair(1)
	bool last;	   //TLAST AXI-Stream signal
} lld_stream;

typedef struct{ //Output AXI-Stream structure
	ap_uint<32> data; //Four bytes of data
	ap_uint<4> keep;  //TKEEP Signals for each byte
	bool last;	   //TLAST AXI-Stream signal
} io_stream;
