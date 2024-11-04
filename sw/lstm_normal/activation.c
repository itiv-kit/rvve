#include "activation.h"

int8_t tanh_lut[64] ={-128,	-128	,-128	,-128,	-128,	-128,	-128,	-127,	-127	,-127	,-127,	-127	,-126,	-126,
	-125,	-124,	-123,	-122,	-120,	-118	,-115	,-111	,-107	,-101	,-95,	-87,	-77,	-66	,-53	,-39	,-24	,-8,	8,
  	24,	39,	53,	66,	77,	87,	95,	101,	107	,111	,115,	118,	120,	122,	123,	124,	125,	126	,126,	127	,127	,127,	127	,127	,127	,127	,127	,127,	127	,127,	127};

/*
void tanh_activation(int64_t *data, int32_t length)
{
  for(int32_t i= 0; i<length; i++)
  {  
    asm volatile(
            ".insn r 0b0001011, 0b001, 0b0000000,  %0, %1,%1\n\t"
            "sw   %0, 0(%2)\n\t"
    :
    :"r"(*(data+i)), "r"(*(data+i)), "r"(data+i)
    : );
  }

}

void logistic(int64_t *data, int32_t length)
{
      for(int32_t i= 0; i<length; i++)
  { 
    if(data[i]<)
    asm volatile(
          ".insn r 0b0001011, 0b000, 0b0000000,  %0, %1,%1\n\t"
          "sw   %0, 0(%2)\n\t"
    :
    :"r"(*(data+i)), "r"(*(data+i)), "r"((data+i))
    : );
    else

  }

}
*/
/*
int8_t tanh_lut[256] = {
-128,
-128,
-128,
-128,
-128,
-128,
-128,
-128,
-128,
-128,
-128,
-128,
-128,
-128,
-128,
-128,
-128,
-128,
-128,
-128,
-128,
-128,
-128,
-128,
-128,
-128,
-128,
-128,
-128,
-127,
-127,
-127,
-127,
-127,
-127,
-127,
-127,
-127,
-127,
-127,
-127,
-127,
-127,
-127,
-127,
-127,
-126,
-126,
-126,
-126,
-126,
-126,
-126,
-126,
-125,
-125,
-125,
-125,
-125,
-125,
-124,
-124,
-124,
-124,
-123,
-123,
-123,
-122,
-122,
-122,
-121,
-121,
-120,
-120,
-119,
-119,
-118,
-118,
-117,
-116,
-116,
-115,
-114,
-113,
-112,
-111,
-110,
-109,
-108,
-107,
-106,
-104,
-103,
-102,
-100,
-99,
-97,
-95,
-93,
-91,
-89,
-87,
-85,
-83,
-80,
-78,
-75,
-73,
-70,
-67,
-64,
-61,
-58,
-55,
-51,
-48,
-44,
-41,
-37,
-33,
-30,
-26,
-22,
-18,
-14,
-10,
-6,
-2,
2,
6,
10,
14,
18,
22,
26,
30,
33,
37,
41,
44,
48,
51,
55,
58,
61,
64,
67,
70,
73,
75,
78,
80,
83,
85,
87,
89,
91,
93,
95,
97,
99,
100,
102,
103,
104,
106,
107,
108,
109,
110,
111,
112,
113,
114,
115,
116,
116,
117,
118,
118,
119,
119,
120,
120,
121,
121,
122,
122,
122,
123,
123,
123,
124,
124,
124,
124,
125,
125,
125,
125,
125,
125,
126,
126,
126,
126,
126,
126,
126,
126,
127,
127,
127,
127,
127,
127,
127,
127,
127,
127,
127,
127,
127,
127,
127,
127,
127,
127,
127,
127,
127,
127,
127,
127,
127,
127,
127,
127,
127,
127,
127,
127,
127,
127,
127,
127,
127,
127,
127,
127,
127,
127,
127,
127,
127,
127};



void tanh_activation(int64_t *data, int32_t length)
{
  for(uint32_t i = 0; i<length; i++)
  {
    int64_t index_val = 0;
    index_val = data[i] << 5;
    index_val = index_val + 0b00000000000000001111111100000000; //+31,5  F32.9
    index_val = index_val +  (1 <<8);

    index_val = index_val >> 9;
    if(index_val >255)
    {
      data[i]=127;
    }
    else if(index_val < 0)
    {
      data[i] = -128;
    }
    else{
    data[i] = (int64_t)tanh_lut[index_val];

    }
  }
}

void logistic(int64_t *data, int32_t length)
{
      for(int32_t i= 0; i<length; i++)
  {  
    int64_t index_val = 0;
    index_val = data[i] << 4;
    index_val = index_val + 0b00000000000000001111111100000000; //+127.5 F32.9
    index_val = index_val+ (1<<8);
    index_val = index_val >> 9;
        if(index_val >255)
    {
      data[i]=127;
    }
    else if(index_val < 0)
    {
      data[i] = -128;
    }
    else{
    data[i] = (int64_t)tanh_lut[index_val];    
    }
    
  }
}
*/

void tanh_activation(int64_t *data, int32_t length)
{
  for(uint32_t i = 0; i<length; i++)
  {
    int32_t index_val = 0;
    index_val = data[i] << 3;
    index_val = index_val + 0b00000000000000000011111100000000; //+31,5  F32.9
    index_val = index_val +  (1 <<8);

    index_val = index_val >> 9;
    if(index_val >63)
    {
      data[i]=127;
    }
    else if(index_val < 0)
    {
      data[i] = -128;
    }
    else{
    data[i] = (int64_t)tanh_lut[index_val];

    }
  }
}

void logistic(int64_t *data, int32_t length)
{
      for(int32_t i= 0; i<length; i++)
  {  
    int32_t index_val = 0;
    index_val = data[i] << 2;
    index_val = index_val + 0b00000000000000000011111100000000; //+127.5 F32.9
    index_val = index_val+ (1<<8);
    index_val = index_val >> 9;
        if(index_val >63)
    {
      data[i]=127;
    }
    else if(index_val < 0)
    {
      data[i] = -128;
    }
    else{
    data[i] = (int64_t)tanh_lut[index_val];    
    }
    
  }
}
