-- #################################################################################################
-- # << NEORV32 - CPU Co-Processor: Custom (Instructions) Functions Unit >>                        #
-- # ********************************************************************************************* #
-- # Intended for user-defined custom RISC-V instructions (R2-type format only). See the CPU's     #
-- # documentation for more information.                                                           #
-- #                                                                                               #
-- # NOTE: Take a look at the "software-counterpart" of this CFU example in 'sw/example/demo_cfu'. #
-- #                                                                                               #
-- # TODO: Maybe turn this into a wrapper for CFU-playground templates.                            #
-- #       -> https://github.com/google/CFU-Playground                                             #
-- # ********************************************************************************************* #
-- # BSD 3-Clause License                                                                          #
-- #                                                                                               #
-- # Copyright (c) 2022, Stephan Nolting. All rights reserved.                                     #
-- #                                                                                               #
-- # Redistribution and use in source and binary forms, with or without modification, are          #
-- # permitted provided that the following conditions are met:                                     #
-- #                                                                                               #
-- # 1. Redistributions of source code must retain the above copyright notice, this list of        #
-- #    conditions and the following disclaimer.                                                   #
-- #                                                                                               #
-- # 2. Redistributions in binary form must reproduce the above copyright notice, this list of     #
-- #    conditions and the following disclaimer in the documentation and/or other materials        #
-- #    provided with the distribution.                                                            #
-- #                                                                                               #
-- # 3. Neither the name of the copyright holder nor the names of its contributors may be used to  #
-- #    endorse or promote products derived from this software without specific prior written      #
-- #    permission.                                                                                #
-- #                                                                                               #
-- # THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS   #
-- # OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF               #
-- # MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE    #
-- # COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,     #
-- # EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE #
-- # GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED    #
-- # AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING     #
-- # NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED  #
-- # OF THE POSSIBILITY OF SUCH DAMAGE.                                                            #
-- # ********************************************************************************************* #
-- # The NEORV32 Processor - https://github.com/stnolting/neorv32              (c) Stephan Nolting #
-- #################################################################################################

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library neorv32;
use neorv32.neorv32_package.all;

entity neorv32_cpu_cp_cfu is
  port (
    -- global control --
    clk_i   : in  std_ulogic; -- global clock, rising edge
    rstn_i  : in  std_ulogic; -- global reset, low-active, async
    ctrl_i  : in  std_ulogic_vector(ctrl_width_c-1 downto 0); -- main control bus
    start_i : in  std_ulogic; -- trigger operation
    -- data input --
    rs1_i   : in  std_ulogic_vector(data_width_c-1 downto 0); -- rf source 1
    rs2_i   : in  std_ulogic_vector(data_width_c-1 downto 0); -- rf source 2
    -- result and status --
    res_o   : out std_ulogic_vector(data_width_c-1 downto 0); -- operation result
    valid_o : out std_ulogic -- data output valid
  );
end neorv32_cpu_cp_cfu;

architecture neorv32_cpu_cp_cfu_rtl of neorv32_cpu_cp_cfu is
    --attribute keep_hierarchy : string;
    --attribute keep_hierarchy of neorv32_cpu_cp_cfu_rtl : architecture  is "yes";


  TYPE vector_of_signed8 IS ARRAY (NATURAL RANGE <>) OF signed(7 DOWNTO 0);
  TYPE vector_of_unsigned16 IS ARRAY (NATURAL RANGE <>) OF unsigned(15 DOWNTO 0);
  -- CFU controller - do not modify --
  type control_t is record
    busy   : std_ulogic; -- CFU is busy
    done   : std_ulogic; -- set to '1' when processing is done
    result : std_ulogic_vector(data_width_c-1 downto 0); -- user's processing result (for write-back to register file)
    funct3 : std_ulogic_vector(2 downto 0); -- "funct3" bit-field from custom instruction
    funct7 : std_ulogic_vector(6 downto 0); -- "funct7" bit-field from custom instruction
  end record;
  
  signal control : control_t;

  -- Constants
  CONSTANT alpha1_D_Lookup_Table1_data    : vector_of_signed8(0 TO 63) := 
    (to_signed(-16#80#, 8), to_signed(-16#80#, 8), to_signed(-16#80#, 8), to_signed(-16#80#, 8),
     to_signed(-16#80#, 8), to_signed(-16#80#, 8), to_signed(-16#80#, 8), to_signed(-16#7F#, 8),
     to_signed(-16#7F#, 8), to_signed(-16#7F#, 8), to_signed(-16#7F#, 8), to_signed(-16#7F#, 8),
     to_signed(-16#7E#, 8), to_signed(-16#7E#, 8), to_signed(-16#7D#, 8), to_signed(-16#7C#, 8),
     to_signed(-16#7B#, 8), to_signed(-16#7A#, 8), to_signed(-16#78#, 8), to_signed(-16#76#, 8),
     to_signed(-16#73#, 8), to_signed(-16#6F#, 8), to_signed(-16#6B#, 8), to_signed(-16#65#, 8),
     to_signed(-16#5F#, 8), to_signed(-16#57#, 8), to_signed(-16#4D#, 8), to_signed(-16#42#, 8),
     to_signed(-16#35#, 8), to_signed(-16#27#, 8), to_signed(-16#18#, 8), to_signed(-16#08#, 8),
     to_signed(16#08#, 8), to_signed(16#18#, 8), to_signed(16#27#, 8), to_signed(16#35#, 8), to_signed(16#42#, 8),
     to_signed(16#4D#, 8), to_signed(16#57#, 8), to_signed(16#5F#, 8), to_signed(16#65#, 8), to_signed(16#6B#, 8),
     to_signed(16#6F#, 8), to_signed(16#73#, 8), to_signed(16#76#, 8), to_signed(16#78#, 8), to_signed(16#7A#, 8),
     to_signed(16#7B#, 8), to_signed(16#7C#, 8), to_signed(16#7D#, 8), to_signed(16#7E#, 8), to_signed(16#7E#, 8),
     to_signed(16#7F#, 8), to_signed(16#7F#, 8), to_signed(16#7F#, 8), to_signed(16#7F#, 8), to_signed(16#7F#, 8),
     to_signed(16#7F#, 8), to_signed(16#7F#, 8), to_signed(16#7F#, 8), to_signed(16#7F#, 8), to_signed(16#7F#, 8),
     to_signed(16#7F#, 8), to_signed(16#7F#, 8));  -- sfix8 [64]

  -- Signals
  SIGNAL In1_signed                       : signed(14 DOWNTO 0);  -- sfix14_En9 
  SIGNAL Product3_out1                    : signed(14 DOWNTO 0);  -- sfix14_En7
  SIGNAL Constant7_out1                   : unsigned(6 DOWNTO 0);  -- ufix7_En1
  SIGNAL Add2_add_cast                    : signed(14 DOWNTO 0);  -- sfix14_En7
  SIGNAL Add2_add_temp                    : signed(14 DOWNTO 0);  -- sfix14_En7
  SIGNAL Add2_out1                        : unsigned(6 DOWNTO 0);  -- ufix7
  SIGNAL alpha1_D_Lookup_Table1_k         : unsigned(5 DOWNTO 0);  -- ufix6
  SIGNAL alpha1_D_Lookup_Table1_out1      : signed(7 DOWNTO 0);  -- sfix8_En7


begin

-- ****************************************************************************************************************************
-- This controller is required to handle the CPU/pipeline interface. Do not modify!
-- ****************************************************************************************************************************

  -- CFU Controller -------------------------------------------------------------------------
  -- -------------------------------------------------------------------------------------------
  cfu_control: process(rstn_i, clk_i)
  begin
    if (rstn_i = '0') then
      control.busy <= '0';
    elsif rising_edge(clk_i) then
      if (control.busy = '0') then -- idle
        if (start_i = '1') then
          control.busy <= '1';
        end if;
      else -- busy
        if (control.done = '1') or (ctrl_i(ctrl_trap_c) = '1') then -- processing done? abort if trap
          control.busy <= '0';
        end if;
      end if;
    end if;
  end process cfu_control;

  -- CPU feedback --
  valid_o <= control.busy and control.done; -- set one cycle before result data
  
  cfu_output: process(rstn_i, clk_i)
  begin
    if (rstn_i = '0') then
      res_o <= (others => '0');
    elsif rising_edge(clk_i) then
      if (control.busy = '1' and control.done = '1') then 
      res_o <= control.result;

      else
      res_o <= (others => '0');

      end if;
    end if;
  end process cfu_output;
 

  -- pack user-defined instruction function bits --
  control.funct3 <= ctrl_i(ctrl_ir_funct3_2_c downto ctrl_ir_funct3_0_c);
  control.funct7 <= ctrl_i(ctrl_ir_funct12_11_c downto ctrl_ir_funct12_5_c);


-- ****************************************************************************************************************************
-- Actual CFU user logic - ADD YOUR CUSTOM LOGIC BELOW
-- ****************************************************************************************************************************

  -- ----------------------------------------------------------------------------------------
  -- CFU Instruction Format
  -- ----------------------------------------------------------------------------------------
  -- The CFU only supports the R2-type RISC-V instruction format. This format consists of two source registers (rs1 and rs2),
  -- a destination register (rd) and two "immediate" bit-fields (funct7 and funct3). It is up to the user to decide which
  -- of these fields are actually used by the CFU logic.


  -- ----------------------------------------------------------------------------------------
  -- Input Operands
  -- ----------------------------------------------------------------------------------------
  -- > rs1_i          (input, 32-bit): source register 1
  -- > rs2_i          (input, 32-bit): source register 2
  -- > control.funct3 (input,  3-bit): 3-bit function select / immediate, driven by instruction word's funct3 bit field
  -- > control.funct7 (input,  7-bit): 7-bit function select / immediate, driven by instruction word's funct7 bit field
  --
  -- The two signal rs1_i and rs2_i provide the data read from the CPU's register file, which is adressed by the
  -- instruction word's rs1 and rs2 bit-fields.
  --
  -- The actual CFU operation can be defined by using the funct3 and funct7 signals. Both signals are directly driven by
  -- the according bit-fields of the custom instruction. Note that these signals represent "immediates" that have to be
  -- static already at compile time. These immediates can be used to select the actual function to be executed or they
  -- can be used as immediates for certain operations (like shift amounts, addresses or offsets).
  --
  -- [NOTE] rs1_i and rs2_i are directly driven by the register file (block RAM). For complex CFU designs it is recommended
  --        to buffer these signals using CFU-internal registers before using them for computations as the rs1 and rs2 nets
  --        need to drive a lot of logic in the CPU. Obviously, this will increase the CFU latency by one cycle.
  --
  -- [NOTE] It is not possible for the CFU and it's according instruction words to cause any kind of exception. The CPU
  --        control logic only verifies the custom instructions OPCODE and checks if the CFU is implemented at all. No
  --        combinations of funct7 and funct3 will cause an exception.


  -- ----------------------------------------------------------------------------------------
  -- Result Output
  -- ----------------------------------------------------------------------------------------
  -- > control.result (output, 32-bit): processing result
  --
  -- When the CFU has finished computation, the data in the control.result signal will be written to the CPU's register
  -- file. The destination register is addressed by the rd bit-field in the instruction. The CFU result output is
  -- registered in the CFU controller (see above) so do not worry too much about increasing the CPU's critical path. ;)


  -- ----------------------------------------------------------------------------------------
  -- Control
  -- ----------------------------------------------------------------------------------------
  -- > rstn_i       (input,  1-bit): asynchronous reset, low-active
  -- > clk_i        (input,  1-bit): main clock, triggering on rising edge
  -- > start_i      (input,  1-bit): operation trigger (start processing, high for one cycle)
  -- > control.done (output, 1-bit): set high when processing is done
  --
  -- For pure-combinatorial instructions (without internal state) a subset of those signals is sufficient; see the minimal
  -- example below. If the CFU shall also include states (like memories, registers or "buffers") the start_i signal can be
  -- used to trigger a new CFU operation. As soon as all internal computations have completed, the control.done signal has
  -- to be set to indicate completion. This will finish CFU operation and will write the processing result (control.result)
  -- to the CPU register file.
  --
  -- [NOTE] The control.done **has to be set at some time**, otherwise the CPU will get stalled forever.


  -- User Logic Example ---------------------------------------------------------------------
  -- -------------------------------------------------------------------------------------------
  user_logic_function_select: process(Add2_add_cast,
  Add2_add_temp(13 DOWNTO 7),
  Add2_add_temp(6), Add2_out1,
  Constant7_out1,
   In1_signed, 
  Product3_out1, alpha1_D_Lookup_Table1_k,
  alpha1_D_Lookup_Table1_out1,
   control,
  rs1_i )
begin
-- This is a simple ALU that implements four pure-combinatorial instructions.
-- The actual function to-be-executed is selected by the "funct3" bit-field of the custom instruction.
case control.funct3 is
when "000"  => --sigmoid
  if((rs1_i(31 downto 12) = (x"0000" & b"0000"))  or  (rs1_i(31 downto 12) = x"FFFF" & b"1111" ))then

      In1_signed(11 downto 0) <= signed( rs1_i(11 downto 0));
      In1_signed(14 downto 12) <= (others => rs1_i(31));
      Product3_out1 <= shift_left(In1_signed,2) ;       
      Constant7_out1 <= to_unsigned(16#3F#, 7);
      Add2_add_cast <= signed(resize( Constant7_out1 & '0' & '0' & '0' & '0' & '0' & '0'& '0' & '0', 15));
      Add2_add_temp <= Product3_out1 + Add2_add_cast;
      Add2_out1 <= unsigned(Add2_add_temp(14 DOWNTO 8)) + ('0' & Add2_add_temp(8));

      if(Add2_out1(6 DOWNTO 1) = to_unsigned(16#00#, 7)) then  
        alpha1_D_Lookup_Table1_k <= to_unsigned(16#00#, 6);
      elsif(Add2_out1(6 DOWNTO 1) >= to_unsigned(16#3F#, 7)) then            
        alpha1_D_Lookup_Table1_k <= to_unsigned(16#3F#, 6);
      else
        alpha1_D_Lookup_Table1_k <= Add2_out1(6 DOWNTO 1);
      end if;

      alpha1_D_Lookup_Table1_out1 <= alpha1_D_Lookup_Table1_data(to_integer(alpha1_D_Lookup_Table1_k));        

      control.result(7 downto 0)  <= std_ulogic_vector(alpha1_D_Lookup_Table1_out1); 
      control.result(31 downto 8) <= (others => std_ulogic(alpha1_D_Lookup_Table1_out1(7))); 
      

    else
      In1_signed <= (others => '0');
      Product3_out1 <= (others => '0');
      Constant7_out1 <= (others => '0');
      Add2_add_cast <= (others => '0');
      Add2_add_temp <=(others => '0');
      Add2_out1 <= (others => '0');
      alpha1_D_Lookup_Table1_k <= (others => '0');
      alpha1_D_Lookup_Table1_out1 <= (others => '0');


        if(rs1_i(31)= '1')then
          control.result(7 downto 0)  <= std_ulogic_vector(alpha1_D_Lookup_Table1_data(0));    

          control.result(31 downto 8)  <= (others => '1');  
    
        else
          control.result(7 downto 0)  <= std_ulogic_vector(alpha1_D_Lookup_Table1_data(63));    
          control.result(31 downto 8) <= (others => '0');
        end if;
  end if;
when "001"  =>
if((rs1_i(31 downto 11) = (x"0000" & b"00000"))  or  (rs1_i(31 downto 11) = x"FFFF" & b"11111" ))then

  In1_signed(10 downto 0) <= signed( rs1_i(10 downto 0));
  In1_signed(14 downto 11) <= (others => rs1_i(31));
  Product3_out1 <= shift_left(In1_signed,3) ;       
  Constant7_out1 <= to_unsigned(16#3F#, 7);
  Add2_add_cast <= signed(resize( Constant7_out1 & '0' & '0' & '0' & '0' & '0' & '0'& '0' & '0', 15));
  Add2_add_temp <= Product3_out1 + Add2_add_cast;
  Add2_out1 <= unsigned(Add2_add_temp(14 DOWNTO 8)) + ('0' & Add2_add_temp(8));

  if(Add2_out1(6 DOWNTO 1) = to_unsigned(16#00#, 7)) then  
    alpha1_D_Lookup_Table1_k <= to_unsigned(16#00#, 6);
  elsif(Add2_out1(6 DOWNTO 1) >= to_unsigned(16#3F#, 7)) then            
    alpha1_D_Lookup_Table1_k <= to_unsigned(16#3F#, 6);
  else
    alpha1_D_Lookup_Table1_k <= Add2_out1(6 DOWNTO 1);
  end if;

  alpha1_D_Lookup_Table1_out1 <= alpha1_D_Lookup_Table1_data(to_integer(alpha1_D_Lookup_Table1_k));

  control.result(7 downto 0)  <= std_ulogic_vector(alpha1_D_Lookup_Table1_out1); 
  control.result(31 downto 8) <= (others => std_ulogic(alpha1_D_Lookup_Table1_out1(7)));
else  
  In1_signed <= (others => '0');
  Product3_out1 <= (others => '0');
  Constant7_out1 <= (others => '0');
  Add2_add_cast <= (others => '0');
  Add2_add_temp <=(others => '0');
  Add2_out1 <= (others => '0');
  alpha1_D_Lookup_Table1_k <= (others => '0');
  alpha1_D_Lookup_Table1_out1 <= (others => '0');


  if(rs1_i(31)= '1')then
    control.result(7 downto 0)  <= std_ulogic_vector(alpha1_D_Lookup_Table1_data(0));    
    control.result(31 downto 8) <= (others => '1');
  else
    control.result(7 downto 0)  <= std_ulogic_vector(alpha1_D_Lookup_Table1_data(63));    
    control.result(31 downto 8) <= (others => '0');
  end if;
end if;

when others => 
  control.result <= (others => '0');      -- not implemented, set to zero
  In1_signed <= (others => '0');
  Product3_out1 <= (others => '0');
  Constant7_out1 <= (others => '0');
  Add2_add_cast <= (others => '0');
  Add2_add_temp <=(others => '0');
  Add2_out1 <= (others => '0');
  alpha1_D_Lookup_Table1_k <= (others => '0');
  alpha1_D_Lookup_Table1_out1 <= (others => '0');

end case;
end process user_logic_function_select;

  -- processing done? --
  control.done <= '1'; -- we are just doing pure-combinatorial data processing here, which is done "immediately"


end neorv32_cpu_cp_cfu_rtl;
