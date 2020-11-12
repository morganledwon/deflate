#include "LZ77_Decoder.h"

#define BUFF_SIZE 32768

void read_cb(
		ap_uint<3> n, //Number of bytes to read (1-4)
		ap_uint<2> read_config, //Read configuration
		ap_uint<15> address0,
		ap_uint<15> address1,
		ap_uint<15> address2,
		ap_uint<15> address3,
		ap_uint<8> &literal0, //Output Literals
		ap_uint<8> &literal1,
		ap_uint<8> &literal2,
		ap_uint<8> &literal3,
		ap_uint<8> (&circ_buff0)[BUFF_SIZE/4],
		ap_uint<8> (&circ_buff1)[BUFF_SIZE/4],
		ap_uint<8> (&circ_buff2)[BUFF_SIZE/4],
		ap_uint<8> (&circ_buff3)[BUFF_SIZE/4]
){
#pragma HLS INLINE
	switch(n){
	case 4 :{
		switch(read_config){
		case 0 :
			literal0 = circ_buff0[address0]; //Read 4 literals from buffer
			literal1 = circ_buff1[address1];
			literal2 = circ_buff2[address2];
			literal3 = circ_buff3[address3];
			break;
		case 1 :
			literal0 = circ_buff1[address0];
			literal1 = circ_buff2[address1];
			literal2 = circ_buff3[address2];
			literal3 = circ_buff0[address3];
			break;
		case 2 :
			literal0 = circ_buff2[address0];
			literal1 = circ_buff3[address1];
			literal2 = circ_buff0[address2];
			literal3 = circ_buff1[address3];
			break;
		case 3 :
			literal0 = circ_buff3[address0];
			literal1 = circ_buff0[address1];
			literal2 = circ_buff1[address2];
			literal3 = circ_buff2[address3];
			break;
		}
		break;}
	case 3 :{
		switch(read_config){
		case 0 :
			literal0 = circ_buff0[address0]; //Read 3 literals from buffer
			literal1 = circ_buff1[address1];
			literal2 = circ_buff2[address2];
			break;
		case 1 :
			literal0 = circ_buff1[address0];
			literal1 = circ_buff2[address1];
			literal2 = circ_buff3[address2];
			break;
		case 2 :
			literal0 = circ_buff2[address0];
			literal1 = circ_buff3[address1];
			literal2 = circ_buff0[address2];
			break;
		case 3 :
			literal0 = circ_buff3[address0];
			literal1 = circ_buff0[address1];
			literal2 = circ_buff1[address2];
			break;
		}
		break;}
	case 2 :{
		switch(read_config){
		case 0 :
			literal0 = circ_buff0[address0];
			literal1 = circ_buff1[address1];
			break;
		case 1 :
			literal0 = circ_buff1[address0];
			literal1 = circ_buff2[address1];
			break;
		case 2 :
			literal0 = circ_buff2[address0];
			literal1 = circ_buff3[address1];
			break;
		case 3 :
			literal0 = circ_buff3[address0];
			literal1 = circ_buff0[address1];
			break;
		}
		break;}
	case 1 :{
		switch(read_config){
		case 0 : literal0 = circ_buff0[address0]; break;
		case 1 : literal0 = circ_buff1[address0]; break;
		case 2 : literal0 = circ_buff2[address0]; break;
		case 3 : literal0 = circ_buff3[address0]; break;
		}
		break;}
	}
}

void write_cb(
		ap_uint<3> n, //Number of bytes to write (1-4)
		ap_uint<2> write_config, //Write configuration
		ap_uint<15> address0,
		ap_uint<15> address1,
		ap_uint<15> address2,
		ap_uint<15> address3,
		ap_uint<8> literal0, //Input Literals
		ap_uint<8> literal1,
		ap_uint<8> literal2,
		ap_uint<8> literal3,
		ap_uint<8> (&circ_buff0)[BUFF_SIZE/4],
		ap_uint<8> (&circ_buff1)[BUFF_SIZE/4],
		ap_uint<8> (&circ_buff2)[BUFF_SIZE/4],
		ap_uint<8> (&circ_buff3)[BUFF_SIZE/4]
){
#pragma HLS INLINE
	switch(n){
	case 4 :{
		switch(write_config){
		case 0 :
			circ_buff0[address0] = literal0; //Print 4 literals to buffer head
			circ_buff1[address1] = literal1;
			circ_buff2[address2] = literal2;
			circ_buff3[address3] = literal3;
			break;
		case 1 :
			circ_buff1[address0] = literal0;
			circ_buff2[address1] = literal1;
			circ_buff3[address2] = literal2;
			circ_buff0[address3] = literal3;
			break;
		case 2 :
			circ_buff2[address0] = literal0;
			circ_buff3[address1] = literal1;
			circ_buff0[address2] = literal2;
			circ_buff1[address3] = literal3;
			break;
		case 3 :
			circ_buff3[address0] = literal0;
			circ_buff0[address1] = literal1;
			circ_buff1[address2] = literal2;
			circ_buff2[address3] = literal3;
			break;
		}
		break;}
	case 3 :{
		switch(write_config){
		case 0 :
			circ_buff0[address0] = literal0;
			circ_buff1[address1] = literal1;
			circ_buff2[address2] = literal2;
			break;
		case 1 :
			circ_buff1[address0] = literal0;
			circ_buff2[address1] = literal1;
			circ_buff3[address2] = literal2;
			break;
		case 2 :
			circ_buff2[address0] = literal0;
			circ_buff3[address1] = literal1;
			circ_buff0[address2] = literal2;
			break;
		case 3 :
			circ_buff3[address0] = literal0;
			circ_buff0[address1] = literal1;
			circ_buff1[address2] = literal2;
			break;
		}
		break;}
	case 2 :{
		switch(write_config){
		case 0 :
			circ_buff0[address0] = literal0;
			circ_buff1[address1] = literal1;
			break;
		case 1 :
			circ_buff1[address0] = literal0;
			circ_buff2[address1] = literal1;
			break;
		case 2 :
			circ_buff2[address0] = literal0;
			circ_buff3[address1] = literal1;
			break;
		case 3 :
			circ_buff3[address0] = literal0;
			circ_buff0[address1] = literal1;
			break;
		}
		break;}
	case 1 :{
		switch(write_config){
		case 0 : circ_buff0[address0] = literal0; break;
		case 1 : circ_buff1[address0] = literal0; break;
		case 2 : circ_buff2[address0] = literal0; break;
		case 3 : circ_buff3[address0] = literal0; break;
		}
		break;}
	}
}

//LZ77 Decoder: Takes in LZ77 commands and outputs decompressed data (4-byte version)
void LZ77_Decoder(
		stream<lld_stream> &in_strm,
		stream<io_stream> &out_strm
){
#pragma HLS INTERFACE ap_ctrl_hs port=return
#pragma HLS INTERFACE axis off port=in_strm //Registers off
#pragma HLS INTERFACE axis off port=out_strm
	lld_stream in_buff;
	lld_stream literal_buffer;
	io_stream out_buff = {0};
	ap_uint<3> state = 0;
	ap_uint<9> length;
	ap_uint<16> distance;
	ap_uint<8> circ_buff0[BUFF_SIZE/4]; //32 kiB cicular buffer split into 4 cyclical partitions
	ap_uint<8> circ_buff1[BUFF_SIZE/4];
	ap_uint<8> circ_buff2[BUFF_SIZE/4];
	ap_uint<8> circ_buff3[BUFF_SIZE/4];
	ap_uint<2> read_config; //Points to the circ_buff partition that literal0 will be read from
	ap_uint<2> write_config = 0; //Points to the circ_buff partition that literal0 will be written to
	ap_uint<8> copy_array[3][4];
#pragma HLS ARRAY_PARTITION variable=copy_array complete dim=0
	ap_uint<2> copy_pointer;
	ap_uint<15> write_addr0 = 0;
	ap_uint<15> write_addr1 = 1;
	ap_uint<15> write_addr2 = 2;
	ap_uint<15> write_addr3 = 3;
	ap_uint<15> read_addr0;
	ap_uint<15> read_addr1;
	ap_uint<15> read_addr2;
	ap_uint<15> read_addr3;
	ap_uint<15> first_read_addr;
	ap_uint<9> length_remaining; //Length_remaining is the total number of bytes to be written
	ap_uint<9> distance_remaining; //Distance_remaining is the number of bytes we can write before the sliding window must be reset
	ap_uint<8> literal0;
	ap_uint<8> literal1;
	ap_uint<8> literal2;
	ap_uint<8> literal3;

	Main_Loop: while(1){
#pragma HLS PIPELINE
#pragma HLS DEPENDENCE variable=circ_buff0 inter false
#pragma HLS DEPENDENCE variable=circ_buff1 inter false
#pragma HLS DEPENDENCE variable=circ_buff2 inter false
#pragma HLS DEPENDENCE variable=circ_buff3 inter false
		if(state == 0){ //No Literal Held State
			in_buff = in_strm.read();
			if(in_buff.last){
				break;
			}
			else{
				if(in_buff.user == 0){ //If data is a literal
					literal_buffer = in_buff;
					state = 1;
				}
				else{ //Length/Distance pair
					length = in_buff.data(31,23);
					distance = in_buff.data(15,0);
					assert(length >= 3); //DEFLATE min allowed length is 3
					assert(length <= 258); //DEFLATE max allowed length is 258
					assert(distance >= 1); //DEFLATE min allowed distance is 1
					assert(distance <= 32768); //DEFLATE max allowed distance is 32768
					read_addr0 = write_addr0 - distance;
					read_addr1 = read_addr0 + 1;
					read_addr2 = read_addr0 + 2;
					read_addr3 = read_addr0 + 3;
					read_config = read_addr0 & 3; //Modulo the read_address to find which buffer partition is read first
					first_read_addr = read_addr0;
					length_remaining = length;
					if(distance <= 4){ //If Distance is 1, 2, 3, or 4
						state = 3;
						distance_remaining = length; //Distance remaining equals length, no wrap-around
					}
					else if(distance <= 7){ //If Distance is 5, 6, or 7
						state = 2;
						distance_remaining = distance; //Distance remaining before wrap-around is distance
					}
					else{ //If Distance is 8 or greater
						state = 2;
						distance_remaining = length;
					}
				}
			}
		}
		else if(state == 1){ //Have_Literal State
			switch(literal_buffer.keep){
			case 0b0001 :
				write_cb(1, write_config, write_addr0/4, 0, 0, 0, literal_buffer.data(7,0), 0, 0, 0, circ_buff0,circ_buff1,circ_buff2,circ_buff3);
				write_config++;
				write_addr0++; write_addr1++; write_addr2++; write_addr3++;
				break;
			case 0b0011 :
				write_cb(2,write_config,write_addr0/4,write_addr1/4,0,0,literal_buffer.data(7,0),literal_buffer.data(15,8),
						0,0,circ_buff0,circ_buff1,circ_buff2,circ_buff3);
				write_config += 2;
				write_addr0 += 2; write_addr1 += 2; write_addr2 += 2; write_addr3 += 2;
				break;
			case 0b0111 :
				write_cb(3,write_config,write_addr0/4,write_addr1/4,write_addr2/4,0,literal_buffer.data(7,0),literal_buffer.data(15,8),
						literal_buffer.data(23,16),0,circ_buff0,circ_buff1,circ_buff2,circ_buff3);
				write_config += 3;
				write_addr0 += 3; write_addr1 += 3; write_addr2 += 3; write_addr3 += 3;
				break;
			case 0b1111 :
				write_cb(4,write_config,write_addr0/4,write_addr1/4,write_addr2/4,write_addr3/4,literal_buffer.data(7,0),literal_buffer.data(15,8),
						literal_buffer.data(23,16),literal_buffer.data(31,24),circ_buff0,circ_buff1,circ_buff2,circ_buff3);
				write_addr0 += 4; write_addr1 += 4; write_addr2 += 4; write_addr3 += 4;
				break;
			}
			out_buff.data = literal_buffer.data;
			out_buff.keep = literal_buffer.keep;
			out_strm << out_buff;
			in_buff = in_strm.read();
			if(in_buff.last){
				break;
			}
			else{
				if(in_buff.user == 0){ //If data is a literal
					literal_buffer = in_buff;
				}
				else{ //Length/Distance pair
					length = in_buff.data(31,23);
					distance = in_buff.data(15,0);
					assert(length >= 3); //DEFLATE min allowed length is 3
					assert(length <= 258); //DEFLATE max allowed length is 258
					assert(distance >= 1); //DEFLATE min allowed distance is 1
					assert(distance <= 32768); //DEFLATE max allowed distance is 32768
					read_addr0 = write_addr0 - distance;
					read_addr1 = read_addr0 + 1;
					read_addr2 = read_addr0 + 2;
					read_addr3 = read_addr0 + 3;
					read_config = read_addr0 & 3; //Modulo the read_address to find which buffer partition is read first
					first_read_addr = read_addr0;
					length_remaining = length;
					if(distance <= 4){ //If Distance is 1, 2, 3, or 4
						state = 3;
						distance_remaining = length; //Distance remaining equals length, no wrap-around
					}
					else if(distance <= 7){ //If Distance is 5, 6, or 7
						state = 2;
						distance_remaining = distance; //Distance remaining before wrap-around is distance
					}
					else{ //If Distance is 8 or greater
						state = 2;
						distance_remaining = length;
					}
				}
			}
		}
		else if(state == 2){ //Regular Length-Distance State
			if(length_remaining >= 4){
				if(distance_remaining >= 4){
					read_cb(4,read_config,read_addr0/4,read_addr1/4,read_addr2/4,read_addr3/4,literal0,literal1,literal2,literal3,circ_buff0,circ_buff1,circ_buff2,circ_buff3);
					if(distance != 32768){
						write_cb(4,write_config,write_addr0/4,write_addr1/4,write_addr2/4,write_addr3/4,literal0,literal1,literal2,literal3,circ_buff0,circ_buff1,circ_buff2,circ_buff3);
					}
					out_buff.data(7,0)   = literal0; //Write 4 literals to output buffer
					out_buff.data(15,8)  = literal1;
					out_buff.data(23,16) = literal2;
					out_buff.data(31,24) = literal3;
					write_addr0 += 4; write_addr1 += 4; write_addr2 += 4; write_addr3 += 4;
					out_buff.keep = 0b1111;
					if(length_remaining == 4){ //If only 4 bytes were left
						state = 0; //Return to Literal state
					}
					else{
						length_remaining -= 4;
						if(distance_remaining == 4){
							distance_remaining = distance; //Reset sliding window
							read_addr0 = first_read_addr; //Shift addresses back to start
							read_addr1 = first_read_addr + 1;
							read_addr2 = first_read_addr + 2;
							read_addr3 = first_read_addr + 3;
						}
						else{
							read_addr0 += 4; read_addr1 += 4; read_addr2 += 4; read_addr3 += 4;
							distance_remaining -= 4;
						}
					}
				}
				else if(distance_remaining == 3){
					read_cb(3,read_config,read_addr0/4,read_addr1/4,read_addr2/4,0,literal0,literal1,literal2,literal3,circ_buff0,circ_buff1,circ_buff2,circ_buff3);
					if(distance != 32768){
						write_cb(3,write_config,write_addr0/4,write_addr1/4,write_addr2/4,0,literal0,literal1,literal2,0,circ_buff0,circ_buff1,circ_buff2,circ_buff3);
					}
					write_config += 3;
					write_addr0 += 3; write_addr1 += 3; write_addr2 += 3; write_addr3 += 3;
					out_buff.data(7,0)   = literal0; //Write 3 literals to output buffer
					out_buff.data(15,8)  = literal1;
					out_buff.data(23,16) = literal2;
					out_buff.keep = 0b0111;
					length_remaining -= 3;
					distance_remaining = distance; //Reset sliding window
					read_addr0 = first_read_addr; // Shift addresses back to start
					read_addr1 = first_read_addr + 1;
					read_addr2 = first_read_addr + 2;
					read_addr3 = first_read_addr + 3;
				}
				else if(distance_remaining == 2){
					read_cb(2,read_config,read_addr0/4,read_addr1/4,0,0,literal0,literal1,literal2,literal3,circ_buff0,circ_buff1,circ_buff2,circ_buff3);
					if(distance != 32768){
						write_cb(2,write_config,write_addr0/4,write_addr1/4,0,0,literal0,literal1,0,0,circ_buff0,circ_buff1,circ_buff2,circ_buff3);
					}
					write_config += 2;
					write_addr0 += 2; write_addr1 += 2; write_addr2 += 2; write_addr3 += 2;
					out_buff.data(7,0)   = literal0; //Write 2 literals to output buffer
					out_buff.data(15,8)  = literal1;
					out_buff.keep = 0b0011;
					length_remaining -= 2;
					distance_remaining = distance; //Reset sliding window
					read_addr0 = first_read_addr; // Shift addresses back to start
					read_addr1 = first_read_addr + 1;
					read_addr2 = first_read_addr + 2;
					read_addr3 = first_read_addr + 3;
				}
				else if(distance_remaining == 1){
					read_cb(1,read_config,read_addr0/4,0,0,0,literal0,literal1,literal2,literal3,circ_buff0,circ_buff1,circ_buff2,circ_buff3);
					if(distance != 32768){
						write_cb(1,write_config,write_addr0/4,0,0,0,literal0,0,0,0,circ_buff0,circ_buff1,circ_buff2,circ_buff3);
					}
					write_config++;
					write_addr0++; write_addr1++; write_addr2++; write_addr3++;
					out_buff.data(7,0)   = literal0; //Read literal from buffer
					out_buff.keep = 0b0001;
					length_remaining -= 1;
					distance_remaining = distance; //Reset sliding window
					read_addr0 = first_read_addr; // Shift addresses back to start
					read_addr1 = first_read_addr + 1;
					read_addr2 = first_read_addr + 2;
					read_addr3 = first_read_addr + 3;
				}
			}
			else if(length_remaining == 3){
				if(distance_remaining >= 3){
					read_cb(3,read_config,read_addr0/4,read_addr1/4,read_addr2/4,0,literal0,literal1,literal2,literal3,circ_buff0,circ_buff1,circ_buff2,circ_buff3);
					if(distance != 32768){
						write_cb(3,write_config,write_addr0/4,write_addr1/4,write_addr2/4,0,literal0,literal1,literal2,0,circ_buff0,circ_buff1,circ_buff2,circ_buff3);
					}
					out_buff.data(7,0)   = literal0; //Write 3 literals to output buffer
					out_buff.data(15,8)  = literal1;
					out_buff.data(23,16) = literal2;
					write_config += 3;
					write_addr0 += 3; write_addr1 += 3; write_addr2 += 3; write_addr3 += 3;
					out_buff.keep = 0b0111;
					state = 0; //Return to Literal state
				}
				else if(distance_remaining == 2){
					read_cb(2,read_config,read_addr0/4,read_addr1/4,0,0,literal0,literal1,literal2,literal3,circ_buff0,circ_buff1,circ_buff2,circ_buff3);
					if(distance != 32768){
						write_cb(2,write_config,write_addr0/4,write_addr1/4,0,0,literal0,literal1,0,0,circ_buff0,circ_buff1,circ_buff2,circ_buff3);
					}
					write_config += 2;
					write_addr0 += 2; write_addr1 += 2; write_addr2 += 2; write_addr3 += 2;
					out_buff.data(7,0)   = literal0; //Write 2 literals to output buffer
					out_buff.data(15,8)  = literal1;
					out_buff.keep = 0b0011;
					length_remaining -= 2;
					distance_remaining = distance; //Reset sliding window
					read_addr0 = first_read_addr; // Shift addresses back to start
					read_addr1 = first_read_addr + 1;
					read_addr2 = first_read_addr + 2;
					read_addr3 = first_read_addr + 3;
				}
				else if(distance_remaining == 1){
					read_cb(1,read_config,read_addr0/4,0,0,0,literal0,literal1,literal2,literal3,circ_buff0,circ_buff1,circ_buff2,circ_buff3);
					if(distance != 32768){
						write_cb(1,write_config,write_addr0/4,0,0,0,literal0,0,0,0,circ_buff0,circ_buff1,circ_buff2,circ_buff3);
					}
					write_config++;
					write_addr0++; write_addr1++; write_addr2++; write_addr3++;
					out_buff.data(7,0)   = literal0; //Read literal from buffer
					out_buff.keep = 0b0001;
					length_remaining -= 1;
					distance_remaining = distance; //Reset sliding window
					read_addr0 = first_read_addr; // Shift addresses back to start
					read_addr1 = first_read_addr + 1;
					read_addr2 = first_read_addr + 2;
					read_addr3 = first_read_addr + 3;
				}
			}
			else if(length_remaining == 2){
				if(distance_remaining >= 2){
					read_cb(2,read_config,read_addr0/4,read_addr1/4,0,0,literal0,literal1,literal2,literal3,circ_buff0,circ_buff1,circ_buff2,circ_buff3);
					if(distance != 32768){
						write_cb(2,write_config,write_addr0/4,write_addr1/4,0,0,literal0,literal1,0,0,circ_buff0,circ_buff1,circ_buff2,circ_buff3);
					}
					write_config += 2;
					write_addr0 += 2; write_addr1 += 2; write_addr2 += 2; write_addr3 += 2;
					out_buff.data(7,0)   = literal0; //Write 2 literals to output buffer
					out_buff.data(15,8)  = literal1;
					out_buff.keep = 0b0011;
					state = 0; //Return to Literal state
				}
				else if(distance_remaining == 1){
					read_cb(1,read_config,read_addr0/4,0,0,0,literal0,literal1,literal2,literal3,circ_buff0,circ_buff1,circ_buff2,circ_buff3);
					if(distance != 32768){
						write_cb(1,write_config,write_addr0/4,0,0,0,literal0,0,0,0,circ_buff0,circ_buff1,circ_buff2,circ_buff3);
					}
					write_config++;
					write_addr0++; write_addr1++; write_addr2++; write_addr3++;
					out_buff.data(7,0)   = literal0; //Read literal from buffer
					out_buff.keep = 0b0001;
					length_remaining -= 1;
					distance_remaining = distance; //Reset sliding window
					read_addr0 = first_read_addr; // Shift addresses back to start
					read_addr1 = first_read_addr + 1;
					read_addr2 = first_read_addr + 2;
					read_addr3 = first_read_addr + 3;
				}
			}
			else if(length_remaining == 1){
				read_cb(1,read_config,read_addr0/4,0,0,0,literal0,literal1,literal2,literal3,circ_buff0,circ_buff1,circ_buff2,circ_buff3);
				if(distance != 32768){
					write_cb(1,write_config,write_addr0/4,0,0,0,literal0,0,0,0,circ_buff0,circ_buff1,circ_buff2,circ_buff3);
				}
				write_config++;
				write_addr0++; write_addr1++; write_addr2++; write_addr3++;
				out_buff.data(7,0)   = literal0; //Read literal from buffer
				out_buff.keep = 0b0001;
				state = 0; //Return to Literal state
			}
			out_strm << out_buff; //Write to stream
		}
		else if(state == 3){ //Distance <= 4, Single-Read State before Write State 
			switch(distance){ //Set up copy array based on distance value
			case 1 :
				read_cb(1,read_config,read_addr0/4,0,0,0,literal0,literal1,literal2,literal3,circ_buff0,circ_buff1,circ_buff2,circ_buff3);
				copy_array[0][0] = literal0; //Store in copy array
				copy_array[0][1] = literal0;
				copy_array[0][2] = literal0;
				copy_array[0][3] = literal0;
				break;
			case 2 :
				read_cb(2,read_config,read_addr0/4,read_addr1/4,0,0,literal0,literal1,literal2,literal3,circ_buff0,circ_buff1,circ_buff2,circ_buff3);
				copy_array[0][0] = literal0; //Store in copy array
				copy_array[0][1] = literal1;
				copy_array[0][2] = literal0;
				copy_array[0][3] = literal1;
				break;
			case 3 :
				read_cb(3,read_config,read_addr0/4,read_addr1/4,read_addr2/4,0,literal0,literal1,literal2,literal3,circ_buff0,circ_buff1,circ_buff2,circ_buff3);
				copy_array[0][0] = literal0; //Store 0120 in copy array 0
				copy_array[0][1] = literal1;
				copy_array[0][2] = literal2;
				copy_array[0][3] = literal0;
				copy_array[1][0] = literal1; //Store 1201 in copy array 1
				copy_array[1][1] = literal2;
				copy_array[1][2] = literal0;
				copy_array[1][3] = literal1;
				copy_array[2][0] = literal2; //Store 2012 in copy array 2
				copy_array[2][1] = literal0;
				copy_array[2][2] = literal1;
				copy_array[2][3] = literal2;
				break;
			case 4 :
				read_cb(4,read_config,read_addr0/4,read_addr1/4,read_addr2/4,read_addr3/4,literal0,literal1,literal2,literal3,circ_buff0,circ_buff1,circ_buff2,circ_buff3);
				copy_array[0][0] = literal0; //Store in copy array
				copy_array[0][1] = literal1;
				copy_array[0][2] = literal2;
				copy_array[0][3] = literal3;
				break;
			}
			copy_pointer = 0;
			state = 4;
		}
		else if(state == 4){ //Distance <= 4, Write State
			if(length_remaining >= 4){
				literal0 = copy_array[copy_pointer][0];
				literal1 = copy_array[copy_pointer][1];
				literal2 = copy_array[copy_pointer][2];
				literal3 = copy_array[copy_pointer][3];
				if(distance == 3){ //If distance is 3, shift copy array pointer
					copy_pointer = (copy_pointer == 2) ? (ap_uint<2>)0b00 : (ap_uint<2>)(copy_pointer + 1); //Increment copy pointer or set it back to 0
				}
				write_cb(4,write_config,write_addr0/4,write_addr1/4,write_addr2/4,write_addr3/4,literal0,literal1,literal2,literal3,circ_buff0,circ_buff1,circ_buff2,circ_buff3);
				write_addr0 += 4; write_addr1 += 4; write_addr2 += 4; write_addr3 += 4;
				out_buff.data(7,0)   = literal0; //Write 4 literals to output buffer
				out_buff.data(15,8)  = literal1;
				out_buff.data(23,16) = literal2;
				out_buff.data(31,24) = literal3;
				out_buff.keep = 0b1111;
				if(length_remaining == 4){ //If only 4 bytes were left
					state = 0; //Return to Literal state
				}
				else{
					length_remaining -= 4;
				}
			}
			else if(length_remaining == 3){
				literal0 = copy_array[copy_pointer][0];
				literal1 = copy_array[copy_pointer][1];
				literal2 = copy_array[copy_pointer][2];
				write_cb(3,write_config,write_addr0/4,write_addr1/4,write_addr2/4,0,literal0,literal1,literal2,0,circ_buff0,circ_buff1,circ_buff2,circ_buff3);
				write_config += 3;
				write_addr0 += 3; write_addr1 += 3; write_addr2 += 3; write_addr3 += 3;
				out_buff.data(7,0)   = literal0; //Write 3 literals to output buffer
				out_buff.data(15,8)  = literal1;
				out_buff.data(23,16) = literal2;
				out_buff.keep = 0b0111;
				state = 0; //Return to Literal state
			}
			else if(length_remaining == 2){
				literal0 = copy_array[copy_pointer][0];
				literal1 = copy_array[copy_pointer][1];
				write_cb(2,write_config,write_addr0/4,write_addr1/4,0,0,literal0,literal1,0,0,circ_buff0,circ_buff1,circ_buff2,circ_buff3);
				write_config += 2;
				write_addr0 += 2; write_addr1 += 2; write_addr2 += 2; write_addr3 += 2;
				out_buff.data(7,0)   = literal0; //Write 2 literals to output buffer
				out_buff.data(15,8)  = literal1;
				out_buff.keep = 0b0011;
				state = 0; //Return to Literal state
			}
			else if(length_remaining == 1){
				literal0 = copy_array[copy_pointer][0];
				write_cb(1,write_config,write_addr0/4,0,0,0,literal0,0,0,0,circ_buff0,circ_buff1,circ_buff2,circ_buff3);
				write_config++;
				write_addr0++; write_addr1++; write_addr2++; write_addr3++;
				out_buff.data(7,0)   = literal0; //Read literal from buffer
				out_buff.keep = 0b0001;
				state = 0; //Return to Literal state
			}
			out_strm << out_buff; //Write to stream
		}
	}
	out_buff.keep = 0b0000; //Set TKEEP signal
	out_buff.last = 0b1; //Set TLAST signal
	out_strm << out_buff;
}
