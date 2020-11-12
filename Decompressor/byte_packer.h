#include <stdio.h>
#include <string.h>
#include <ap_int.h>
#include <hls_stream.h>

using namespace hls;

template<int D>
struct axi_strm{
	ap_uint<D> data;   //D bits of data
	ap_uint<D/8> keep; //TKEEP signals for each byte
	bool last;	       //TLAST AXI-Stream signal
};
//Stream Format:
// | 31 - Byte3 - 24 | 23 - Byte2 - 16 | 15 - Byte1 - 8 | 7 - Byte0 - 0 |
// |	  TKEEP3	 |		TKEEP2	   |	  TKEEP1	|	  TKEEP0    |

typedef axi_strm<32> io_strm;


