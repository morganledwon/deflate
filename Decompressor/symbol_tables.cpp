//The permuted order found when reading Code Length code length sequence
const ap_uint<8> permuted_order[MAX_CL_CODES] = {16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15};

//Each index (symbol) contains that symbols base value and extra bits
//Structure: {Extra bits, Base Value-3}
const length_symbol_values length_symbol_table[29] = { //Length Symbols 257 to 285 are indexed here from 0 to 28
		/* 0*/	{0,	0},   {0, 1},   {0,	2},   {0, 3},
		/* 4*/  {0,	4},   {0, 5},   {0,	6},   {0, 7},
		/* 8*/  {1,	8},   {1, 10},  {1,	12},  {1, 14},
		/*12*/  {2,	16},  {2, 20},  {2,	24},  {2, 28},
		/*16*/  {3,	32},  {3, 40},  {3,	48},  {3, 56},
		/*20*/  {4,	64},  {4, 80},  {4,	96},  {4, 112},
		/*24*/  {5,	128}, {5, 160}, {5, 192}, {5, 224},
		/*28*/  {0,	255}
};

//Structure: {Extra bits, Base value}
const distance_symbol_values distance_symbol_table[30] = {
		/* 0*/  {0, 1},      {0, 2},     {0, 3},     {0, 4},
		/* 4*/	{1,	5},      {1, 7},     {2, 9},     {2, 13},
		/* 8*/	{3,	17},     {3, 25},    {4, 33},    {4, 49},
		/*12*/  {5,	65},     {5, 97},    {6, 129},   {6, 193},
		/*16*/  {7,	257},    {7, 385},   {8, 513},   {8, 769},
		/*20*/	{9,	1025},   {9, 1537},  {10, 2049}, {10, 3073},
		/*24*/	{11, 4097},  {11, 6145}, {12, 8193}, {12, 12289},
		/*28*/  {13, 16385}, {13, 24577}
};
