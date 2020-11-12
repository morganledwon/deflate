#include "literal_stacker.h"

void literal_stacker(
		stream<lld_stream> &in_strm,
		stream<lld_stream> &out_strm
){
#pragma HLS INTERFACE ap_ctrl_none port=return
#pragma HLS PIPELINE enable_flush
#pragma HLS INTERFACE axis off port=out_strm //For individual core synthesis
#pragma HLS INTERFACE axis off port=in_strm
	static lld_stream in_buff;
	static lld_stream data_regA; //Data register to be filled with literals
	static ap_uint<2> state = 0;

	if(state == 0){ //Length-Distance State: Continue streaming LD pairs through until Literals are encountered
		in_strm >> in_buff;
		if(in_buff.last == 1){ //If last pack (No data)
			out_strm << in_buff; //Stream out data pack with TLAST high
		}
		else{
			if(in_buff.user == 1){ //If data is Length-Distance pair
				out_strm << in_buff; //Stream out LD pair
			}
			else{ //If data is composed of literals
				if(in_buff.keep == 0b1111){ //If no literals needed
					out_strm << in_buff; //Stream out full data pack
				}
				else{ //Literals needed to fill this data pack
					data_regA = in_buff; //Store in RegA
					state = 1; //Change to literal state
				}
			}
		}
	}
	else if(state == 1){ //Literal State: Stack literals with more literals from the stream. Remain here until LD pair read
		in_strm >> in_buff; //Stream in another data pack
		if(in_buff.last == 1){ //If last pack (No data)
			out_strm << data_regA; //Flush out datapack in RegA
			state = 2; //Go to flush state
		}
		else{
			if(in_buff.user == 1){ //If data is Length-Distance pair
				out_strm << data_regA; //Flush out datapack in RegA
				state = 2; //Go to flush state
			}
			else{ //If data is composed of literals
				if(data_regA.keep == 0b1111){ //If no literals needed (Should never happen)
					out_strm << data_regA; //Stream out full data pack
					data_regA = in_buff; //Move new data to regA
				}
				else if(data_regA.keep == 0b0111){ //1 literal needed by RegA
					if(in_buff.keep == 0b0001){ //If number of literals in in_buff is exactly 1
						data_regA.data(31,24) = in_buff.data(7,0); //Take 1 literal from it
						data_regA.keep = 0b1111;				   //Update RegA TKEEP signal
						out_strm << data_regA;					   //Stream out full data pack
						state = 0;								   //Return to LD state
					}
					else if(in_buff.keep == 0b0000){ //If data pack is empty (Should never happen)
						//Do nothing
					}
					else{ //If number of literals is from 2 to 4 (more than needed)
						data_regA.data(31,24) = in_buff.data(7,0); //Take 1 literal from it
						data_regA.keep = 0b1111;				   //Update RegA TKEEP signal
						out_strm << data_regA;					   //Stream out full data pack
						data_regA.data = in_buff.data >> 8;		 //Shift in_buff data and store in RegA
						data_regA.keep = in_buff.keep >> 1;		 //Shift in_buff TKEEP signal and store in RegA
					}
				}
				else if(data_regA.keep == 0b0011){ //2 literals needed by RegA
					if(in_buff.keep == 0b0011){ //If number of literals in in_buff is exactly 2
						data_regA.data(31,16) = in_buff.data(15,0); //Take 2 literals
						data_regA.keep = 0b1111; //Update RegA TKEEP signal
						out_strm << data_regA; //Stream out full data pack
						state = 0;	//Return to LD state
					}
					else if(in_buff.keep == 0b0001){ //If only 1 literal in in_buff (less than needed)
						data_regA.data(23,16) = in_buff.data(7,0); //Take 1 literal
						data_regA.keep = 0b0111; //Update TKEEP
					}
					else if(in_buff.keep == 0b0000){ //If data pack is empty (Should never happen)
						//Do nothing
					}
					else{ //If number of literals is 3 or 4 (more than needed)
						data_regA.data(31,16) = in_buff.data(15,0); //Take 2 literals
						data_regA.keep = 0b1111; //Update RegA TKEEP signal
						out_strm << data_regA; //Stream out full data pack
						data_regA.data = in_buff.data >> 16; //Shift in_buff data and store in RegA
						data_regA.keep = in_buff.keep >> 2;	 //Shift in_buff TKEEP signal and store in RegA
					}
				}
				else if(data_regA.keep == 0b0001){ //3 literals needed by RegA
					if(in_buff.keep == 0b0111){ //If number of literals in in_buff is exactly 3
						data_regA.data(31,8) = in_buff.data(23,0); //Take 3 literals
						data_regA.keep = 0b1111;
						out_strm << data_regA;
						state = 0;	//Return to LD state
					}
					else if(in_buff.keep == 0b0011){ //If only 2 literals in in_buff (less than needed)
						data_regA.data(23,8) = in_buff.data(15,0); //Take 2 literals
						data_regA.keep = 0b0111; //Update TKEEP
					}
					else if(in_buff.keep == 0b0001){ //If only 1 literal in in_buff (less than needed)
						data_regA.data(15,8) = in_buff.data(7,0); //Take 1 literal
						data_regA.keep = 0b0011; //Update TKEEP
					}
					else if(in_buff.keep == 0b0000){ //If data pack is empty (Should never happen)
						//Do nothing
					}
					else{ //If number of literals is 4 (more than needed)
						data_regA.data(31,8) = in_buff.data(23,0); //Take 3 literals
						data_regA.keep = 0b1111; //Update RegA TKEEP signal
						out_strm << data_regA; //Stream out full data pack
						data_regA.data = in_buff.data >> 24; //Shift in_buff data and store in RegA
						data_regA.keep = in_buff.keep >> 3;	//Shift in_buff TKEEP signal and store in RegA
					}
				}
				else if(data_regA.keep == 0b0000){ //RegA is empty (Should never happen)
					data_regA = in_buff; //Move new data to regA
				}
			}
		}
	}
	else if(state == 2){ //Flush state (Needed to space out writes to output stream)
		out_strm << in_buff; //Stream out LD pair
		state = 0; //Return to LD state
	}
}

