#include "Huffman_Encoder.h"
#include "code_tables.cpp"

void symbol_encoder(
		ap_uint<24>  data, 	      //Input (3 bytes)
		ap_uint<2>   type,   	  //Input
		ap_uint<26>  &code,  	  //Output (Can be from 0 to 26 bits long)
		ap_uint<5>   &code_length //Output
){
#pragma HLS INLINE
	ap_uint<9>    length;
	ap_uint<15>   distance;
	ap_uint<8>    length_code;
	distance_code distance_symbol;
	ap_uint<13>   extra_distance_value;

	switch(type){
	case 0b00 : //Matched literal
		code = 0;
		code_length = 0;
		break;

	case 0b01 : //Unmatched Literal
		assert(data >= 0);
		assert(data <= 255);
		if(data <= 143){
			code_length = 8;
			data = data + 48; //Convert literal value to code
			code = data(0,7); //Mirror code and output it
		}
		else{
			code_length = 9;
			data = data + 256; //Convert literal value to code
			code = data(0,8);  //Mirror code and output it
		}
		break;

	case 0b10 : //Match
		length = data(23,15);
		distance = data(14,0);
		assert(length >= 3); //Assertions for C debugging
		assert(length <= 16);
		assert(distance >= 0); //Distance is subtracted by 1 to fit within 15 bits (Done by LZ77 Encoder)
		assert(distance <= 32767);

		//Length Encoding
		if(length <= 10){ //If length is 10 or less, no extra bits in code length
			code_length = 7;
		}
		else{
			code_length = 8; //7-bit length code + 1 extra bit
		}
		length -= 3; //Adjust length before looking up in table
		length_code = length_code_table[length]; //Length codes are pre-mirrored with extra bits included

		//Distance Encoding
		if(distance < 256){ //Distance is from 1 to 256
			distance_symbol = distance_code_table[distance];
		}
		else{ //Distance is from 257 to 32768
			distance_symbol = distance_code_table[distance(14,7) + 256]; //Top 8 bits of distance + 256
		}
		extra_distance_value = distance - distance_symbol.base;

		//Concatenate codes as follows: extra distance bits, distance code (mirrored), length code (already mirrored)
		code = (extra_distance_value, distance_symbol.code(0,4), length_code(code_length-1,0));
		code_length += 5 + distance_symbol.bits; //Add distance bits to code length
		break;
	}
}

template <int instance>
void window_packer(
		in_stream  *input_window_in,	//Input: Window of 3-byte boxes to be encoded
		in_stream  *input_window_out,   //Output
		t_enc_win  *encoded_window_in,  //Input: Window for packing Huffman codes
		ap_uint<8> *encoded_bits_in,	//Input: Number of bits in encoded window
		t_enc_win  *encoded_window_out, //Output
		ap_uint<8> *encoded_bits_out	//Output
){
#pragma HLS INLINE

	ap_uint<26> code;  	   //Huffman code to be packed
	ap_uint<5>  code_length;  //Length of Huffman code
	t_enc_win   shifted_code;

	//Input window is Little Endian, process lower end first
	ap_uint<24> input_box =  input_window_in->data((instance*24)+23,instance*24);
	ap_uint<2>  input_type = input_window_in->user( (instance*2) +1, instance*2);

	symbol_encoder(input_box, input_type, code, code_length); //Encode box from input window

	*encoded_window_out = *encoded_window_in | (t_enc_win(code) << *encoded_bits_in); //Pack code in window
	*encoded_bits_out = *encoded_bits_in + code_length;  //Update bit count of window
	assert(*encoded_bits_out <= ENC_WIN_BITS); //C Debug: Check for encoded window overfill

	*input_window_out = *input_window_in;     //Pass input window to output
}

void output_packer(
		t_enc_win   	   	  encoded_window_in, //Input: Window for packing Huffman codes
		ap_uint<8> 		      encoded_bits_in,   //Input: Number of bits in encoded window
		ap_uint<OUT_WIN_BITS> &output_window,    //In/Out
		ap_uint<10>	 	  	  &output_bits,	     //In/Out
		stream<out_stream>    &output_strm 	     //Output
){
#pragma HLS INLINE
	out_stream out_buff = {0};

	output_window |= ap_uint<OUT_WIN_BITS>(encoded_window_in) << output_bits; //Pack encoded window in output packer
	output_bits += encoded_bits_in; //Update bit count of packer
	assert(output_bits <= OUT_WIN_BITS); //C Debug: Check for output window overfill

	if(output_bits >= OUT_STRM_BITS){ //If packer is more than half full
		out_buff.data = output_window(OUT_STRM_BITS-1,0); //Write out bottom half of output packer
		out_buff.keep = 0xFFFFFFFF; //Set all TKEEP bits high
		out_buff.last = 0;
		output_strm << out_buff;
		output_window >>= OUT_STRM_BITS; //Shift out bottom half of output packer
		output_bits -= OUT_STRM_BITS; //Update bit count of packer
	}
}

//Pipelined Huffman encoder for encoding all symbols of a window in parallel
void huffman_encoder(
		stream<in_stream>  &strm_in,
		stream<out_stream> &strm_out
){
#pragma HLS INTERFACE axis off port=strm_out
#pragma HLS INTERFACE axis off port=strm_in
	in_stream input_window0;
	in_stream input_window1;
	in_stream input_window2;
	in_stream input_window3;
	in_stream input_window4;
	in_stream input_window5;
	in_stream input_window6;
	in_stream input_window7;
	in_stream input_window8;
	in_stream input_window9;
	in_stream input_window10;
	in_stream input_window11;
	in_stream input_window12;
	in_stream input_window13;
	in_stream input_window14;
	in_stream input_window15;
	in_stream input_window16;
	t_enc_win encoded_window0;
	t_enc_win encoded_window1;
	t_enc_win encoded_window2;
	t_enc_win encoded_window3;
	t_enc_win encoded_window4;
	t_enc_win encoded_window5;
	t_enc_win encoded_window6;
	t_enc_win encoded_window7;
	t_enc_win encoded_window8;
	t_enc_win encoded_window9;
	t_enc_win encoded_window10;
	t_enc_win encoded_window11;
	t_enc_win encoded_window12;
	t_enc_win encoded_window13;
	t_enc_win encoded_window14;
	t_enc_win encoded_window15;
	t_enc_win encoded_window16;
	ap_uint<8> encoded_bits0;
	ap_uint<8> encoded_bits1;
	ap_uint<8> encoded_bits2;
	ap_uint<8> encoded_bits3;
	ap_uint<8> encoded_bits4;
	ap_uint<8> encoded_bits5;
	ap_uint<8> encoded_bits6;
	ap_uint<8> encoded_bits7;
	ap_uint<8> encoded_bits8;
	ap_uint<8> encoded_bits9;
	ap_uint<8> encoded_bits10;
	ap_uint<8> encoded_bits11;
	ap_uint<8> encoded_bits12;
	ap_uint<8> encoded_bits13;
	ap_uint<8> encoded_bits14;
	ap_uint<8> encoded_bits15;
	ap_uint<8> encoded_bits16;

	//Initialize output_window
	ap_uint<OUT_WIN_BITS> output_window = 0b011; //Output buffer for packing encoded windows
	ap_uint<10> output_bits = 3; //Number of bits currently in output_packer
	out_stream out_buff = {0};

	do{
#pragma HLS PIPELINE
		encoded_window0 = 0; //Clear encoded window before packing
		encoded_bits0 = 0;

		strm_in >> input_window0; //Read an input window from the input stream

		window_packer<0>(&input_window0, &input_window1, &encoded_window0, &encoded_bits0, &encoded_window1, &encoded_bits1);
		window_packer<1>(&input_window1, &input_window2, &encoded_window1, &encoded_bits1, &encoded_window2, &encoded_bits2);
		window_packer<2>(&input_window2, &input_window3, &encoded_window2, &encoded_bits2, &encoded_window3, &encoded_bits3);
		window_packer<3>(&input_window3, &input_window4, &encoded_window3, &encoded_bits3, &encoded_window4, &encoded_bits4);
		window_packer<4>(&input_window4, &input_window5, &encoded_window4, &encoded_bits4, &encoded_window5, &encoded_bits5);
		window_packer<5>(&input_window5, &input_window6, &encoded_window5, &encoded_bits5, &encoded_window6, &encoded_bits6);
		window_packer<6>(&input_window6, &input_window7, &encoded_window6, &encoded_bits6, &encoded_window7, &encoded_bits7);
		window_packer<7>(&input_window7, &input_window8, &encoded_window7, &encoded_bits7, &encoded_window8, &encoded_bits8);
		window_packer<8>(&input_window8, &input_window9, &encoded_window8, &encoded_bits8, &encoded_window9, &encoded_bits9);
		window_packer<9>(&input_window9, &input_window10, &encoded_window9, &encoded_bits9, &encoded_window10, &encoded_bits10);
		window_packer<10>(&input_window10, &input_window11, &encoded_window10, &encoded_bits10, &encoded_window11, &encoded_bits11);
		window_packer<11>(&input_window11, &input_window12, &encoded_window11, &encoded_bits11, &encoded_window12, &encoded_bits12);
		window_packer<12>(&input_window12, &input_window13, &encoded_window12, &encoded_bits12, &encoded_window13, &encoded_bits13);
		window_packer<13>(&input_window13, &input_window14, &encoded_window13, &encoded_bits13, &encoded_window14, &encoded_bits14);
		window_packer<14>(&input_window14, &input_window15, &encoded_window14, &encoded_bits14, &encoded_window15, &encoded_bits15);
		window_packer<15>(&input_window15, &input_window16, &encoded_window15, &encoded_bits15, &encoded_window16, &encoded_bits16);

		output_packer(encoded_window16, encoded_bits16, output_window, output_bits, strm_out);

	}while(!input_window0.last);
	output_bits += 7; //Add EOB code to end of stream (7 zeroes)

	if(output_bits >= OUT_STRM_BITS){ //If packer is more than half full, perform two final writes to stream
		out_buff.data = output_window(OUT_STRM_BITS-1,0); //Write out bottom half of output packer
		out_buff.keep = 0xFFFFFFFF; //Set all TKEEP bits high
		strm_out << out_buff;
		output_window >>= OUT_STRM_BITS; //Shift out bottom half of output packer
		output_bits -= OUT_STRM_BITS; //Update bit count of packer
		out_buff.data = output_window(OUT_STRM_BITS-1,0); //Write out remaining data in output packer
		out_buff.keep = decoder32[(output_bits+7)/8]; //Set TKEEP using decoder. Number of bytes rounded up
		out_buff.last = true;
		strm_out << out_buff;
	}
	else{ //Otherwise, just one final write to stream
		out_buff.data = output_window(OUT_STRM_BITS-1,0); //Write out remaining data in output packer
		out_buff.keep = decoder32[(output_bits+7)/8]; //Set TKEEP using decoder. Number of bytes rounded up
		out_buff.last = true;
		strm_out << out_buff;
	}
}
