void inplace_point32_twise_multiplication(int32_t *a, int32_t *b, int32_t n)
{
       
    int32_t * vec_b_help = b;
    int32_t * vec_a_help = a;
    uint32_t vl = 8;
    int32_t acc = 0;
    uint32_t j = 0;
   do 
    {
      if((j+vl)<n){
          vl=8;
        }
      else{
          vl=n-j;                  
        }


        asm volatile(".insn u 0b1011011, %3, 0b11000000001100000111\n\t"
    :
    :"r"(vec_a_help), "r"(vec_b_help), "r"(acc), "r"(vl),  "r"(&acc)
    : );

         asm volatile(    
     ".insn i 0b1111011, 0b010,x24, %1,  0b000000000100\n\t"
     :
     :"r"(vec_a_help), "r"(vec_b_help), "r"(acc), "r"(vl),  "r"(&acc)
     :  "x24", "x25", "x26", "x27", "x28","x29","x30", "x31" ); 


                
      asm volatile(
     ".insn i 0b1111011, 0b010,x16, %0,  0b000000000100\n\t"
      :
      :"r"(vec_a_help), "r"(vec_b_help), "r"(acc), "r"(vl),  "r"(&acc)
      :   "x16", "x17", "x18","x19","x20", "x21", "x22","x23", "x24", "x25", "x26", "x27", "x28","x29","x30", "x31" );

      asm volatile(
      ".insn r 0b1011011, 0b100, 0x00, x16, x24, x16\n\t"         
      :
      :"r"(vec_a_help), "r"(vec_b_help), "r"(acc), "r"(vl),  "r"(&acc)
      : "x16", "x17", "x18","x19","x20", "x21", "x22","x23", "x24", "x25", "x26", "x27", "x28","x29","x30", "x31","memory" );

      asm volatile(        
      ".insn s 0b0101011, 0b010, x16 ,  0b000000000000(%0)\n\t"   
      :
      :"r"(vec_a_help), "r"(vec_b_help), "r"(acc), "r"(vl),  "r"(&acc)
      :"x16", "x17", "x18","x19","x20", "x21", "x22","x23","memory");

      j=j+vl;
      vec_a_help=vec_a_help+vl;
      vec_b_help=vec_b_help+vl;



    }while( j < n );
   
        
}
