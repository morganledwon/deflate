#include "byte_packer.h"

unsigned char align_bytes(
		io_strm &data_reg_in,
		io_strm &data_reg_out
){
#pragma HLS INLINE
	unsigned char bytes_needed;

	switch(data_reg_in.keep){
	case 0b0000 : //All bytes Null
		data_reg_out.data = data_reg_in.data; //Pass data through anyway
		data_reg_out.keep = 0b0000;
		bytes_needed = 4;
		break;
	case 0b0001 :
		data_reg_out.data = data_reg_in.data;
		data_reg_out.keep = 0b0001;
		bytes_needed = 3;
		break;
	case 0b0010 :
		data_reg_out.data = data_reg_in.data >> 8; //Shift all bytes right by 8 bits
		data_reg_out.keep = 0b0001;
		bytes_needed = 3;
		break;
	case 0b0011 :
		data_reg_out.data = data_reg_in.data;
		data_reg_out.keep = 0b0011;
		bytes_needed = 2;
		break;
	case 0b0100 :
		data_reg_out.data = data_reg_in.data >> 16; //Shift all bytes right by 16 bits
		data_reg_out.keep = 0b0001;
		bytes_needed = 3;
		break;
	case 0b0101 :
		data_reg_out.data(15,8) = data_reg_in.data(23,16); //Shift second byte right by 8 bits
		data_reg_out.data(7,0)  = data_reg_in.data(7,0);   //Copy bottom byte
		data_reg_out.keep = 0b0011;
		bytes_needed = 2;
		break;
	case 0b0110 :
		data_reg_out.data = data_reg_in.data >> 8; //Shift all bytes right by 8 bits
		data_reg_out.keep = 0b0011;
		bytes_needed = 2;
		break;
	case 0b0111 :
		data_reg_out.data = data_reg_in.data;
		data_reg_out.keep = 0b0111;
		bytes_needed = 1;
		break;
	case 0b1000 :
		data_reg_out.data = data_reg_in.data >> 24;
		data_reg_out.keep = 0b0001;
		bytes_needed = 3;
		break;
	case 0b1001 :
		data_reg_out.data(15,8) = data_reg_in.data(31,24); //Shift upper byte right by 16 bits
		data_reg_out.data(7,0)  = data_reg_in.data(7,0);   //Copy bottom byte
		data_reg_out.keep = 0b0011;
		bytes_needed = 2;
		break;
	case 0b1010 :
		data_reg_out.data(7,0)  = data_reg_in.data(15,8);  //Shift third byte right by 8 bits
		data_reg_out.data(15,8) = data_reg_in.data(31,24); //Shift upper byte right by 16 bits
		data_reg_out.keep = 0b0011;
		bytes_needed = 2;
		break;
	case 0b1011 :
		data_reg_out.data(23,16) = data_reg_in.data(31,24); //Shift upper byte right by 8 bits
		data_reg_out.data(15,0)  = data_reg_in.data(15,0);  //Copy bottom two bytes
		data_reg_out.keep = 0b0111;
		bytes_needed = 1;
		break;
	case 0b1100 :
		data_reg_out.data = data_reg_in.data >> 16; //Shift all bytes right by 16 bits
		data_reg_out.keep = 0b0011;
		bytes_needed = 2;
		break;
	case 0b1101 :
		data_reg_out.data(23,8) = data_reg_in.data(31,16); //Shift upper two bytes right by 8 bits
		data_reg_out.data(7,0)  = data_reg_in.data(7,0);   //Copy bottom byte
		data_reg_out.keep = 0b0111;
		bytes_needed = 1;
		break;
	case 0b1110 :
		data_reg_out.data = data_reg_in.data >> 8; //Shift all bytes right by 8 bits
		data_reg_out.keep = 0b0111;
		bytes_needed = 1;
		break;
	case 0b1111 :
		data_reg_out.data = data_reg_in.data;
		data_reg_out.keep = 0b1111;
		bytes_needed = 0;
		break;
	}
	data_reg_out.last = data_reg_in.last; //Pass TLAST signal

	return bytes_needed;
};

//byte_packer:
//Takes in 4 byte wide (little-endian) AXI-Stream and checks accompanying TKEEP signals for presence of null bytes.
//Will remove null bytes from stream and realign bytes into 4 byte wide continuous output stream.
//If TLAST is asserted, will output currently held bytes regardless of having 4 bytes or not.
void byte_packer(
		stream<io_strm> &in_strm,
		stream<io_strm> &out_strm
){
#pragma HLS PIPELINE enable_flush
#pragma HLS INTERFACE ap_ctrl_none port=return
#pragma HLS INTERFACE axis register both port=in_strm
#pragma HLS INTERFACE axis register both port=out_strm
	static io_strm in_buff = {0};   //Axi-Stream input buffer
	static io_strm data_regA = {0}; //First data register to be filled
	static io_strm data_regB = {0}; //Second data register to be filled, used to fill RegA
	static unsigned char bytes_neededA = 4; //Number of bytes needed to fill the data pack currently held in data_regA
	static unsigned char bytes_neededB = 4;
	static bool state = 0; //Current machine state, can be 1 or 0

	if(state == 0){ //Pass-through State: Continue streaming full data packs through until null bytes are encountered
		in_strm >> in_buff;
		bytes_neededA = align_bytes(in_buff, data_regA); //Align data pack and find bytes_needed
		if(bytes_neededA == 0){    //If no bytes needed, data already aligned
			out_strm << data_regA; //Stream out full data pack
		}
		else if(bytes_neededA == 4){   //If data pack is all null bytes
			if(data_regA.last){        //And TLAST is high
				out_strm << data_regA; //Stream out empty data pack with TLAST high
			} 						   //Otherwise, discard empty datapack
		}                          //If bytes_needed is 1, 2, or 3
		else if(data_regA.last){   //And TLAST is high
			out_strm << data_regA; //Stream out data pack whether it is full or not
		}
		else{ //Bytes needed to fill this data pack
			state = 1; //Change to fill state
		}
	}
	else{ //Fill State: Replace null bytes with true data bytes from the stream. Remain here as long as stream is discontinuous
		in_strm >> in_buff; //Stream in another data pack
		bytes_neededB = align_bytes(in_buff, data_regB);   //Align it and store in Register B
		switch(bytes_neededA){//Check if data is needed in RegA
		case 0 : //Reg A is already full of good data (Should never happen)
			out_strm << data_regA;				  //Stream out full data pack
			data_regA.data = data_regB.data >> 8; //Move RegB data to RegA
			data_regA.keep = data_regB.keep >> 1; //Move RegB TKEEP to RegA
			bytes_neededA = bytes_neededB;		  //Update number of bytes needed by RegA
			break;
		case 1 : //1 byte needed by RegA
			if(bytes_neededB == 3){ //If number of bytes in RegB is exactly 1
				data_regA.data(31,24) = data_regB.data(7,0); //Take 1 byte from it
				data_regA.keep = 0b1111;					 //Update RegA TKEEP signal
				data_regA.last = data_regB.last;			 //Pass TLAST signal
				out_strm << data_regA;						 //Stream out full data pack
				state = 0;									 //Return to pass-through state
			}
			else if(bytes_neededB <= 2){ //If number of bytes in RegB is 2 or more (more than needed)
				data_regA.data(31,24) = data_regB.data(7,0); //Take 1 byte from it
				data_regA.keep = 0b1111;					 //Update RegA TKEEP signal
				out_strm << data_regA;						 //Stream out full data pack
				data_regA.data = data_regB.data >> 8;		 //Shift RegB data and store in RegA
				data_regA.keep = data_regB.keep >> 1;		 //Shift RegB TKEEP signal and store in RegA
				bytes_neededA = bytes_neededB + 1;			 //Update number of bytes needed by RegA
			}
			else{ //RegB has no useable bytes.
				if(data_regB.last){ //If RegB is last
					data_regA.last = 1;
					out_strm << data_regA; //Flush out datapack in RegA
					state = 0;			   //Return to pass-through state
				} //Otherwise, read another data pack into RegB next iteration
			}
			break;
		case 2 : //2 bytes needed by RegA
			if(bytes_neededB == 2){ //If number of bytes in RegB is exactly 2
				data_regA.data(31,16) = data_regB.data(15,0); //Take 2 bytes
				data_regA.keep = 0b1111;
				data_regA.last = data_regB.last;
				out_strm << data_regA;
				state = 0;	//Return to pass-through state
			}
			else if(bytes_neededB == 3){ //If only 1 byte in RegB (less than needed)
				data_regA.data(23,16) = data_regB.data(7,0); //Take 1 byte
				data_regA.keep = 0b0111; //Update TKEEP (in case TLAST is 1)
				if(data_regB.last){ //If TLAST is high
					data_regA.last = 1;
					out_strm << data_regA; //Write out regA
					state = 0;			   //Return to pass-through state
				}
				else{ //Otherwise
					bytes_neededA = 1; //Update number of bytes needed by RegA
				}
			}
			else if(bytes_neededB <= 1){ //If number of bytes in RegB is 3 or more (more than needed)
				data_regA.data(31,16) = data_regB.data(15,0); //Take 2 bytes
				data_regA.keep = 0b1111;					  //Update RegA TKEEP signal
				out_strm << data_regA;						  //Stream out full data pack
				data_regA.data = data_regB.data >> 16;		  //Shift RegB data and store in RegA
				data_regA.keep = data_regB.keep >> 2;		  //Shift RegB TKEEP signal and store in RegA
				bytes_neededA = bytes_neededB + 2;			  //Update number of bytes needed by RegA
			}
			else{ //RegB has no useable bytes
				if(data_regB.last){ //If RegB is last
					data_regA.last = 1;
					out_strm << data_regA; //Flush out datapack in RegA
					state = 0;			   //Return to pass-through state
				} //Otherwise, read another data pack into RegB next iteration
			}
			break;
		case 3 : //3 bytes needed by RegA
			if(bytes_neededB == 1){ //If number of bytes in RegB is exactly 3
				data_regA.data(31,8) = data_regB.data(23,0); //Take 3 bytes
				data_regA.keep = 0b1111;
				data_regA.last = data_regB.last;
				out_strm << data_regA;
				state = 0;	//Return to pass-through state
			}
			else if(bytes_neededB == 2){ //If only 2 bytes in RegB (less than needed)
				data_regA.data(23,8) = data_regB.data(15,0); //Take 2 bytes
				data_regA.keep = 0b0111; //Update TKEEP (in case TLAST is 1)
				if(data_regB.last){
					data_regA.last = 1;
					out_strm << data_regA; //Write out regA
					state = 0;			   //Return to pass-through state
				}
				else{
					bytes_neededA = 1;
				}
			}
			else if(bytes_neededB == 3){ //If only 1 byte in RegB (less than needed)
				data_regA.data(15,8) = data_regB.data(7,0); //Take 1 byte
				data_regA.keep = 0b0011; //Update TKEEP (in case TLAST is 1)
				if(data_regB.last){
					data_regA.last = 1;
					out_strm << data_regA; //Write out regA
					state = 0;			   //Return to pass-through state
				}
				else{
					bytes_neededA = 2;
				}
			}
			else if(bytes_neededB == 0){ //If number of bytes in RegB is 4 (more than needed)
				data_regA.data(31,8) = data_regB.data(23,0); //Take 3 bytes
				data_regA.keep = 0b1111;					 //Update RegA TKEEP signal
				out_strm << data_regA;						 //Stream out full data pack
				data_regA.data = data_regB.data >> 24;		  //Shift RegB data and store in RegA
				data_regA.keep = data_regB.keep >> 3;		  //Shift RegB TKEEP signal and store in RegA
				bytes_neededA = bytes_neededB + 3;			  //Update number of bytes needed by RegA
			}
			else{ //RegB has no useable bytes
				if(data_regB.last){ //If RegB is last
					data_regA.last = 1;
					out_strm << data_regA; //Flush out datapack in RegA
					state = 0;			   //Return to pass-through state
				} //Otherwise, read another data pack into RegB next iteration
			}
			break;
		case 4 : //RegA is empty (Should never happen)
			data_regA.data = data_regB.data >> 8; //Move RegB data to RegA
			data_regA.keep = data_regB.keep >> 1; //Move RegB TKEEP to RegA
			bytes_neededA = bytes_neededB;		  //Update number of bytes needed by RegA
			break;
		}
	}
}
