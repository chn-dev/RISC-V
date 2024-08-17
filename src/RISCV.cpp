/*----------------------------------------------------------------------------*/
/*!
\file RISCV.cpp
\author Christian Nowak <chnowak@web.de>
\brief This implements the RISCV core emulation
*/
/*----------------------------------------------------------------------------*/
#include <stdio.h>
#include <string>

#include "RISCV.h"


/*----------------------------------------------------------------------------*/
/*! 2024-08-14
Constructor for class RISCV
*/
/*----------------------------------------------------------------------------*/
RISCV::RISCV( MemoryInterface *pMem ) :
   m_pMemory( pMem )
{
   reset();
}


/*----------------------------------------------------------------------------*/
/*! 2024-08-14
Destructor for class RISCV
*/
/*----------------------------------------------------------------------------*/
RISCV::~RISCV()
{
}


/*----------------------------------------------------------------------------*/
/*! 2024-08-14
Convert a register number to its name.
\param n The register number
\return The register's name
*/
/*----------------------------------------------------------------------------*/
std::string RISCV::registerName( int n )
{
   static std::string registerNames[32] =
   {
      "zero", "ra", "sp",  "gp",  "tp", "t0", "t1", "t2",
      "s0",   "s1", "a0",  "a1",  "a2", "a3", "a4", "a5",
      "a6",   "a7", "s2",  "s3",  "s4", "s5", "s6", "s7",
      "s8",   "s9", "s10", "s11", "t3", "t4", "t5", "t6"
   };

   return( registerNames[n & 31] );
}


/*----------------------------------------------------------------------------*/
/*! 2024-08-14
Set the PC to 0x80000000 and all registers to 0.
*/
/*----------------------------------------------------------------------------*/
void RISCV::reset()
{
   m_PC = 0x80000000;
   for( int i = 0; i < 32; i++ )
   {
      m_Registers[i] = 0;
   }
}


/*----------------------------------------------------------------------------*/
/*! 2024-08-14
\param r A register number
\return The current contents of the register
*/
/*----------------------------------------------------------------------------*/
uint32_t RISCV::getRegister( int r ) const
{
   r &= 0x1f;

   if( r == 0 )
   {
      return( 0 );
   } else
   {
      return( m_Registers[r & 0x1f] );
   }
}


/*----------------------------------------------------------------------------*/
/*! 2024-08-14
\return The current value of the PC (program counter)
*/
/*----------------------------------------------------------------------------*/
uint32_t RISCV::getPC() const
{
   return( m_PC );
}


/*----------------------------------------------------------------------------*/
/*! 2024-08-14
Set a register to a given value.
\param r The register number
\param v The value to set the register to
*/
/*----------------------------------------------------------------------------*/
void RISCV::setRegister( int r, uint32_t v )
{
   r &= 0x1f;

   if( r != 0 )
   {
      m_Registers[r] = v;
   }
}


/*----------------------------------------------------------------------------*/
/*! 2024-08-14
This method is called when the emulation encounters an unknown opcode.
*/
/*----------------------------------------------------------------------------*/
void RISCV::unknownOpcode()
{
   m_pMemory->unknownOpcode();
}


/*----------------------------------------------------------------------------*/
/*! 2024-08-14
Execute a single machine code instruction.
\param pInstruction Pointer to an Instruction object for a disassembly.
Pass nullptr if no disassembly is required.
*/
/*----------------------------------------------------------------------------*/
void RISCV::step( Instruction *pInstruction )
{
   uint32_t instr = readMem32( m_PC );
   uint32_t oldPC = getPC();

   uint8_t opcode = instr & 0x7f;

   uint8_t rd = ( instr >> 7 ) & 0x1f;
   uint8_t rs1 = ( instr >> 15 ) & 0x1f;
   uint8_t rs2 = ( instr >> 20 ) & 0x1f;
   uint8_t funct3 = ( instr >> 12 ) & 0x07;
   uint8_t funct7 = ( instr >> 25) & 0x7f;
   int32_t uTypeImm = (int32_t)( instr & 0xfffff000 );
   int32_t iTypeImm = (int32_t)instr >> 20;
   int32_t sTypeImm = ( ( ( (int32_t)instr >> 25 ) & 0x7f ) << 5 ) | // imm[11:5]
                      ( ( ( (int32_t)instr >> 7 ) & 0x1f ) << 0 ); // imm[4:0]
   // Propagate the sign bit of the S-Type immediate
   if( instr & 0x80000000 )
      sTypeImm |= ~0b111111111111;
   int32_t jTypeImm = ( ( ( (int32_t)instr >> 31 ) & 1            ) << 20 ) | // imm[20]
                      ( ( ( (int32_t)instr >> 21 ) & 0b1111111111 ) << 1  ) | // imm[10:1]
                      ( ( ( (int32_t)instr >> 20 ) & 1            ) << 11 ) | // imm[11]
                      ( ( ( (int32_t)instr >> 12 ) & 0xff         ) << 12 );  // imm[19:12]
   // Propagate the sign bit of the J-Type immediate
   if( instr & 0x80000000 )
      jTypeImm |= 0b111111111111 << 20;

   int32_t bTypeImm = ( ( ( (int32_t)instr >> 31 ) & 1            ) << 12 ) | // imm[12]
                      ( ( ( (int32_t)instr >> 25 ) & 0b111111     ) << 5  ) | // imm[10:5]
                      ( ( ( (int32_t)instr >> 8  ) & 0b1111       ) << 1  ) | // imm[4:1]
                      ( ( ( (int32_t)instr >> 7  ) & 1            ) << 11 );  // imm[11]
   // Propagate the sign bit of the B-Type immediate
   if( instr & 0x80000000 )
      bTypeImm |= 0b11111111111111111111 << 12;

   uint32_t newPC = oldPC;

   switch( opcode )
   {
      case 0x03: // LOAD
      {
         switch( funct3 ) // width
         {
            case 0: // LB
            {
               uint32_t d = readMem8( getRegister( rs1 ) + iTypeImm );
               if( d & 0x80 )
                  d |= 0xffffff00;
               setRegister( rd, d );
               newPC = oldPC + 4;

               if( pInstruction )
               {
                  pInstruction->set( oldPC, instr, "lb",
                     util::strvec
                     {
                        registerName( rd ),
                        stdformat( "{}({})", iTypeImm, registerName( rs1 ) )
                     }
                  );
               }
               break;
            }

            case 1: // LH
            {
               uint32_t d = readMem16( getRegister( rs1 ) + iTypeImm );
               if( d & 0x8000 )
                  d |= 0xffff0000;
               setRegister( rd, d );
               newPC = oldPC + 4;

               if( pInstruction )
               {
                  pInstruction->set( oldPC, instr, "lh",
                     util::strvec
                     {
                        registerName( rd ),
                        stdformat( "{}({})", iTypeImm, registerName( rs1 ) )
                     }
                  );
               }
               break;
            }

            case 2: // LW
            {
               setRegister( rd, readMem32( getRegister( rs1 ) + iTypeImm ) );
               newPC = oldPC + 4;

               if( pInstruction )
               {
                  pInstruction->set( oldPC, instr, "lw",
                     util::strvec
                     {
                        registerName( rd ),
                        stdformat( "{}({})", iTypeImm, registerName( rs1 ) )
                     }
                  );
               }
               break;
            }

            case 4: // LBU
            {
               setRegister( rd, readMem8( getRegister( rs1 ) + iTypeImm ) );
               newPC = oldPC + 4;

               if( pInstruction )
               {
                  pInstruction->set( oldPC, instr, "lbu",
                     util::strvec
                     {
                        registerName( rd ),
                        stdformat( "{}({})", iTypeImm, registerName( rs1 ) )
                     }
                  );
               }
               break;
            }

            case 5: // LHU
            {
               setRegister( rd, readMem16( getRegister( rs1 ) + iTypeImm ) );
               newPC = oldPC + 4;

               if( pInstruction )
               {
                  pInstruction->set( oldPC, instr, "lhu",
                     util::strvec
                     {
                        registerName( rd ),
                        stdformat( "{}({})", iTypeImm, registerName( rs1 ) )
                     }
                  );
               }
               break;
            }

            default:
            {
               unknownOpcode();
               break;
            }
         }
         break;
      }

      case 0x13:
      {
         switch( funct3 )
         {
            case 0: // ADDI
            {
               setRegister( rd, getRegister( rs1 ) + iTypeImm );
               newPC = oldPC + 4;

               if( pInstruction )
               {
                  pInstruction->set( oldPC, instr, "addi",
                     util::strvec
                     {
                        registerName( rd ),
                        registerName( rs1 ),
                        stdformat( "{}", iTypeImm )
                     }
                  );
               }
               break;
            }

            case 1: // SLLI
            {
               switch( funct7 )
               {
                  case 0:
                  {
                     uint32_t shamt = ( instr >> 20 ) & 0x1f;
                     setRegister( rd, getRegister( rs1 ) << shamt );
                     newPC = oldPC + 4;

                     if( pInstruction )
                     {
                        pInstruction->set( oldPC, instr, "slli",
                           util::strvec
                           {
                              registerName( rd ),
                              registerName( rs1 ),
                              stdformat( "{}", shamt )
                           }
                        );
                     }
                     break;
                  }

                  default:
                  {
                     unknownOpcode();
                     break;
                  }
               }
               break;
            }

            case 2: // SLTI
            {
               if( (int32_t)getRegister( rs1 ) < iTypeImm )
               {
                  setRegister( rd, 1 );
               } else
               {
                  setRegister( rd, 0 );
               }
               newPC = oldPC + 4;

               if( pInstruction )
               {
                  pInstruction->set( oldPC, instr, "slti",
                     util::strvec
                     {
                        registerName( rd ),
                        registerName( rs1 ),
                        stdformat( "{}", iTypeImm )
                     }
                  );
               }
               break;
            }

            case 3: // SLTIU
            {
               if( getRegister( rs1 ) < (uint32_t)iTypeImm )
               {
                  setRegister( rd, 1 );
               } else
               {
                  setRegister( rd, 0 );
               }
               newPC = oldPC + 4;

               if( pInstruction )
               {
                  pInstruction->set( oldPC, instr, "sltiu",
                     util::strvec
                     {
                        registerName( rd ),
                        registerName( rs1 ),
                        stdformat( "{}", (uint32_t)iTypeImm )
                     }
                  );
               }
               break;
            }

            case 4: // XORI
            {
               setRegister( rd, getRegister( rs1 ) ^ (uint32_t)iTypeImm );
               newPC = oldPC + 4;

               if( pInstruction )
               {
                  pInstruction->set( oldPC, instr, "xori",
                     util::strvec
                     {
                        registerName( rd ),
                        registerName( rs1 ),
                        stdformat( "{}", (uint32_t)iTypeImm )
                     }
                  );
               }
               break;
            }

            case 5:
            {
               switch( funct7 )
               {
                  case 0: // SRLI
                  {
                     uint32_t shamt = ( instr >> 20 ) & 0x1f;
                     setRegister( rd, getRegister( rs1 ) >> shamt );
                     newPC = oldPC + 4;

                     if( pInstruction )
                     {
                        pInstruction->set( oldPC, instr, "srli",
                           util::strvec
                           {
                              registerName( rd ),
                              registerName( rs1 ),
                              stdformat( "{}", shamt )
                           }
                        );
                     }
                     break;
                  }

                  case 0x20: // SRAI
                  {
                     uint32_t shamt = ( instr >> 20 ) & 0x1f;
                     setRegister( rd, (int32_t)getRegister( rs1 ) >> shamt );
                     newPC = oldPC + 4;

                     if( pInstruction )
                     {
                        pInstruction->set( oldPC, instr, "srai",
                           util::strvec
                           {
                              registerName( rd ),
                              registerName ( rs1 ),
                              stdformat( "{}", shamt )
                           }
                        );
                     }
                     break;
                  }

                  default:
                  {
                     unknownOpcode();
                     break;
                  }
               }
               break;
            }

            case 6: // ORI
            {
               setRegister( rd, getRegister( rs1 ) | (uint32_t)iTypeImm );
               newPC = oldPC + 4;

               if( pInstruction )
               {
                  pInstruction->set( oldPC, instr, "ori",
                     util::strvec
                     {
                        registerName( rd ),
                        registerName( rs1 ),
                        stdformat( "{}", (uint32_t)iTypeImm )
                     }
                  );
               }
               break;
            }

            case 7: // ANDI
            {
               setRegister( rd, getRegister( rs1 ) & (uint32_t)iTypeImm );
               newPC = oldPC + 4;

               if( pInstruction )
               {
                  pInstruction->set( oldPC, instr, "andi",
                     util::strvec
                     {
                        registerName( rd ),
                        registerName( rs1 ),
                        stdformat( "{}", (uint32_t)iTypeImm )
                     }
                  );
               }
               break;
            }

            default:
            {
               unknownOpcode();
               break;
            }
         }
         break;
      }

      case 0x17: // AUIPC
      {
         setRegister( rd, oldPC + uTypeImm );
         newPC = oldPC + 4;

         if( pInstruction )
         {
            pInstruction->set( oldPC, instr, "auipc",
               util::strvec
               {
                  registerName( rd ),
                  stdformat( "0x{:x}", uTypeImm >> 12 )
               }
            );
         }
         break;
      }

      case 0x23: // STORE
      {
         switch( funct3 ) // width
         {
            case 0: // SB
            {
               writeMem8( getRegister( rs1 ) + sTypeImm, getRegister( rs2 ) & 0xff );
               newPC = oldPC + 4;

               if( pInstruction )
               {
                  pInstruction->set( oldPC, instr, "sb",
                     util::strvec
                     {
                        registerName( rs2 ),
                        stdformat( "{}({})", sTypeImm, registerName( rs1 ) )
                     }
                  );
               }
               break;
            }

            case 1: // SH
            {
               writeMem16( getRegister( rs1 ) + sTypeImm, getRegister( rs2 ) & 0xffff );
               newPC = oldPC + 4;

               if( pInstruction )
               {
                  pInstruction->set( oldPC, instr, "sh",
                     util::strvec
                     {
                        registerName( rs2 ),
                        stdformat( "{}({})", sTypeImm, registerName( rs1 ) )
                     }
                  );
               }
               break;
            }

            case 2: // SW
            {
               writeMem32( getRegister( rs1 ) + sTypeImm, getRegister( rs2 ) );
               newPC = oldPC + 4;

               if( pInstruction )
               {
                  pInstruction->set( oldPC, instr, "sw",
                     util::strvec
                     {
                        registerName( rs2 ),
                        stdformat( "{}({})", sTypeImm, registerName( rs1 ) )
                     }
                  );
               }
               break;
            }

            default:
            {
               unknownOpcode();
               break;
            }
         }
         break;
      }

      case 0x2f:
      {
         switch( funct3 )
         {
            case 2:
            {
               switch( funct7 & ~3 ) // Ignore the aq and rl flags as we're emulating only a single hart
               {
                  case 0: // amoadd.w
                  {
                     uint32_t address = getRegister( rs1 );
                     uint32_t v = readMem32( address );
                     setRegister( rd, v );
                     v = v + getRegister( rs2 );
                     writeMem32( address, v );
                     newPC = oldPC + 4;

                     if( pInstruction )
                     {
                        pInstruction->set( oldPC, instr, "amoadd.w",
                           util::strvec
                           {
                              registerName( rd ),
                              registerName( rs2 ),
                              stdformat( "({})", registerName( rs1 ) )
                           }
                        );
                     }
                     break;
                  }

                  case 4: // amoswap.w
                  {
                     uint32_t address = getRegister( rs1 );
                     uint32_t v = readMem32( address );
                     setRegister( rd, v );
                     v = getRegister( rs2 );
                     writeMem32( address, v );
                     newPC = oldPC + 4;

                     if( pInstruction )
                     {
                        pInstruction->set( oldPC, instr, "amoswap.w",
                           util::strvec
                           {
                              registerName( rd ),
                              registerName( rs2 ),
                              stdformat( "({})", registerName( rs1 ) )
                           }
                        );
                     }
                     break;
                  }

                  case 8: // lr.w (load&reserve word)
                  {
                     uint32_t addr = getRegister( rs1 );
                     reserveAddr( addr, 4 );
                     setRegister( rd, readMem32( getRegister( rs1 ) ) );

                     newPC = oldPC + 4;

                     if( pInstruction )
                     {
                        pInstruction->set( oldPC, instr, "lr.w",
                           util::strvec
                           {
                              registerName( rd ),
                              stdformat( "({})", registerName( rs1 ) )
                           }
                        );
                     }
                     break;
                  }

                  case 12: // sc.w (store conditional)
                  {
                     if( numReservedAddresses( getRegister( rs1 ), 4 ) == 4 )
                     {
                        // All accessed addresses have been reserved -> success
                        writeMem32( getRegister( rs1 ), getRegister( rs2 ) );
                        setRegister( rd, 0 );
                     } else
                     {
                        setRegister( rd, 1 );
                     }

                     clearAllReservations();

                     newPC = oldPC + 4;

                     if( pInstruction )
                     {
                        pInstruction->set( oldPC, instr, "sc.w",
                           util::strvec
                           {
                              registerName( rd ),
                              registerName( rs2 ),
                              stdformat( "({})", registerName( rs1 ) )
                           }
                        );
                     }
                     break;
                  }

                  case 16: // amoxor.w
                  {
                     uint32_t address = getRegister( rs1 );
                     uint32_t v = readMem32( address );
                     setRegister( rd, v );
                     v = v ^ getRegister( rs2 );
                     writeMem32( address, v );
                     newPC = oldPC + 4;

                     if( pInstruction )
                     {
                        pInstruction->set( oldPC, instr, "amoxor.w",
                           util::strvec
                           {
                              registerName( rd ),
                              registerName( rs2 ),
                              stdformat( "({})", registerName( rs1 ) )
                           }
                        );
                     }
                     break;
                  }

                  case 32: // amoor.w
                  {
                     uint32_t address = getRegister( rs1 );
                     uint32_t v = readMem32( address );
                     setRegister( rd, v );
                     v = v | getRegister( rs2 );
                     writeMem32( address, v );
                     newPC = oldPC + 4;

                     if( pInstruction )
                     {
                        pInstruction->set( oldPC, instr, "amoor.w",
                           util::strvec
                           {
                              registerName( rd ),
                              registerName( rs2 ),
                              stdformat( "({})", registerName( rs1 ) )
                           }
                        );
                     }
                     break;
                  }

                  case 48: // amoand.w
                  {
                     uint32_t address = getRegister( rs1 );
                     uint32_t v = readMem32( address );
                     setRegister( rd, v );
                     v = v & getRegister( rs2 );
                     writeMem32( address, v );
                     newPC = oldPC + 4;

                     if( pInstruction )
                     {
                        pInstruction->set( oldPC, instr, "amoand.w",
                           util::strvec
                           {
                              registerName( rd ),
                              registerName( rs2 ),
                              stdformat( "({})", registerName( rs1 ) )
                           }
                        );
                     }
                     break;
                  }

                  case 64: // amomin.w
                  {
                     uint32_t address = getRegister( rs1 );
                     uint32_t v = readMem32( address );
                     setRegister( rd, v );
                     uint32_t vrs2 = getRegister( rs2 );
                     v = (int32_t)v < (int32_t)vrs2 ? v : vrs2;
                     writeMem32( address, v );
                     newPC = oldPC + 4;

                     if( pInstruction )
                     {
                        pInstruction->set( oldPC, instr, "amomin.w",
                           util::strvec
                           {
                              registerName( rd ),
                              registerName( rs2 ),
                              stdformat( "({})", registerName( rs1 ) )
                           }
                        );
                     }
                     break;
                  }

                  case 80: // amomax.w
                  {
                     uint32_t address = getRegister( rs1 );
                     uint32_t v = readMem32( address );
                     setRegister( rd, v );
                     uint32_t vrs2 = getRegister( rs2 );
                     v = (int32_t)v > (int32_t)vrs2 ? v : vrs2;
                     writeMem32( address, v );
                     newPC = oldPC + 4;

                     if( pInstruction )
                     {
                        pInstruction->set( oldPC, instr, "amomax.w",
                           util::strvec
                           {
                              registerName( rd ),
                              registerName( rs2 ),
                              stdformat( "({})", registerName( rs1 ) )
                           }
                        );
                     }
                     break;
                  }

                  case 96: // amominu.w
                  {
                     uint32_t address = getRegister( rs1 );
                     uint32_t v = readMem32( address );
                     setRegister( rd, v );
                     uint32_t vrs2 = getRegister( rs2 );
                     v = v < vrs2 ? v : vrs2;
                     writeMem32( address, v );
                     newPC = oldPC + 4;

                     if( pInstruction )
                     {
                        pInstruction->set( oldPC, instr, "amominu.w",
                           util::strvec
                           {
                              registerName( rd ),
                              registerName( rs2 ),
                              stdformat( "({})", registerName( rs1 ) )
                           }
                        );
                     }
                     break;
                  }

                  case 112: // amomaxu.w
                  {
                     uint32_t address = getRegister( rs1 );
                     uint32_t v = readMem32( address );
                     setRegister( rd, v );
                     uint32_t vrs2 = getRegister( rs2 );
                     v = v > vrs2 ? v : vrs2;
                     writeMem32( address, v );
                     newPC = oldPC + 4;

                     if( pInstruction )
                     {
                        pInstruction->set( oldPC, instr, "amomaxu.w",
                           util::strvec
                           {
                              registerName( rd ),
                              registerName( rs2 ),
                              stdformat( "({})", registerName( rs1 ) )
                           }
                        );
                     }
                     break;
                  }

                  default:
                  {
                     unknownOpcode();
                     break;
                  }
               }
               break;
            }

            default:
            {
               unknownOpcode();
               break;
            }
         }

         break;
      }

      case 0x33:
      {
         switch( funct3 )
         {
            case 0:
            {
               switch( funct7 )
               {
                  case 0x00: // ADD
                  {
                     setRegister( rd, (int32_t)getRegister( rs1 ) + (int32_t)getRegister( rs2 ) );
                     newPC = oldPC + 4;

                     if( pInstruction )
                     {
                        pInstruction->set( oldPC, instr, "add",
                           util::strvec
                           {
                              registerName( rd ),
                              registerName( rs1 ),
                              registerName( rs2 )
                           }
                        );
                     }
                     break;
                  }

                  case 0x01: // MUL
                  {
                     setRegister( rd, (uint32_t)( ( (int64_t)getRegister( rs1 ) * (int64_t)getRegister( rs2 ) ) & 0xffffffff ) );
                     newPC = oldPC + 4;

                     if( pInstruction )
                     {
                        pInstruction->set( oldPC, instr, "mul",
                           util::strvec
                           {
                              registerName( rd ),
                              registerName( rs1 ),
                              registerName( rs2 )
                           }
                        );
                     }
                     break;
                  }

                  case 0x20: // SUB
                  {
                     setRegister( rd, (int32_t)getRegister( rs1 ) - (int32_t)getRegister( rs2 ) );
                     newPC = oldPC + 4;

                     if( pInstruction )
                     {
                        pInstruction->set( oldPC, instr, "sub",
                           util::strvec
                           {
                              registerName( rd ),
                              registerName( rs1 ),
                              registerName( rs2 )
                           }
                        );
                     }
                     break;
                  }

                  default:
                  {
                     unknownOpcode();
                     break;
                  }
               }
               break;
            }

            case 1:
            {
               switch( funct7 )
               {
                  case 0: // SLL
                  {
                     setRegister( rd, getRegister( rs1 ) << ( getRegister( rs2 ) & 0x1f ) );
                     newPC = oldPC + 4;

                     if( pInstruction )
                     {
                        pInstruction->set( oldPC, instr, "sll",
                           util::strvec
                           {
                              registerName( rd ),
                              registerName( rs1 ),
                              registerName( rs2 )
                           }
                        );
                     }
                     break;
                  }

                  case 1: // MULH
                  {
                     setRegister( rd, ( ( (int64_t)getRegister( rs1 ) * (int64_t)getRegister( rs2 ) ) >> 32 ) & 0xffffffff );
                     newPC = oldPC + 4;

                     if( pInstruction )
                     {
                        pInstruction->set( oldPC, instr, "mulh",
                           util::strvec
                           {
                              registerName( rd ),
                              registerName( rs1 ),
                              registerName( rs2 )
                           }
                        );
                     }
                     break;
                  }

                  default:
                  {
                     unknownOpcode();
                     break;
                  }
               }
               break;
            }

            case 2:
            {
               switch( funct7 )
               {
                  case 0: // SLT
                  {
                     if( (int32_t)getRegister( rs1 ) < (int32_t)getRegister( rs2 ) )
                     {
                        setRegister( rd, 1 );
                     } else
                     {
                        setRegister( rd, 0 );
                     }
                     newPC = oldPC + 4;

                     if( pInstruction )
                     {
                        pInstruction->set( oldPC, instr, "slt",
                           util::strvec
                           {
                              registerName( rd ),
                              registerName( rs1 ),
                              registerName( rs2 )
                           }
                        );
                     }
                     break;
                  }

                  case 1: // MULHSU
                  {
                     setRegister( rd, (uint32_t)( ( ( (int64_t)getRegister( rs1 ) * (uint64_t)getRegister( rs2 ) ) >> 32 ) & 0xffffffff ) );
                     newPC = oldPC + 4;

                     if( pInstruction )
                     {
                        pInstruction->set( oldPC, instr, "mulhsu",
                           util::strvec
                           {
                              registerName( rd ),
                              registerName( rs1 ),
                              registerName( rs2 )
                           }
                        );
                     }
                     break;
                  }

                  default:
                  {
                     unknownOpcode();
                     break;
                  }
               }
               break;
            }

            case 3:
            {
               switch( funct7 )
               {
                  case 0: // SLTU
                  {
                     if( getRegister( rs1 ) < getRegister( rs2 ) )
                     {
                        setRegister( rd, 1 );
                     } else
                     {
                        setRegister( rd, 0 );
                     }
                     newPC = oldPC + 4;

                     if( pInstruction )
                     {
                        pInstruction->set( oldPC, instr, "sltu",
                           util::strvec
                           {
                              registerName( rd ),
                              registerName( rs1 ),
                              registerName( rs2 )
                           }
                        );
                     }
                     break;
                  }

                  case 1: // MULHU
                  {
                     setRegister( rd, (uint32_t)( ( ( (uint64_t)getRegister( rs1 ) * (uint64_t)getRegister( rs2 ) ) >> 32 ) & 0xffffffff ) );
                     newPC = oldPC + 4;

                     if( pInstruction )
                     {
                        pInstruction->set( oldPC, instr, "mulhu",
                           util::strvec
                           {
                              registerName( rd ),
                              registerName( rs1 ),
                              registerName( rs2 )
                           }
                        );
                     }
                     break;
                  }

                  default:
                  {
                     unknownOpcode();
                     break;
                  }
               }
               break;
            }

            case 4:
            {
               switch( funct7 )
               {
                  case 0: // XOR
                  {
                     setRegister( rd, getRegister( rs1 ) ^ getRegister( rs2 ) );
                     newPC = oldPC + 4;

                     if( pInstruction )
                     {
                        pInstruction->set( oldPC, instr, "xor",
                           util::strvec
                           {
                              registerName( rd ),
                              registerName( rs1 ),
                              registerName( rs2 )
                           }
                        );
                     }
                     break;
                  }

                  case 1: // DIV
                  {
                     if( getRegister( rs2 ) == 0 )
                     {
                        setRegister( rd, 0xffffffff );
                     } else
                     if( ( getRegister( rs1 ) == 0x80000000 ) && ( getRegister( rs2 ) == 0xffffffff ) ) // -2^LEN-1 / -1 -> overflow
                     {
                        setRegister( rd, 0x80000000 );
                     } else
                     {
                        setRegister( rd, (uint32_t)( (int32_t)getRegister( rs1 ) / (int32_t)getRegister( rs2 ) ) );
                     }
                     newPC = oldPC + 4;

                     if( pInstruction )
                     {
                        pInstruction->set( oldPC, instr, "div",
                           util::strvec
                           {
                              registerName( rd ),
                              registerName( rs1 ),
                              registerName( rs2 )
                           }
                        );
                     }
                     break;
                  }

                  default:
                  {
                     unknownOpcode();
                     break;
                  }
               }
               break;
            }

            case 5:
            {
               switch( funct7 )
               {
                  case 0: // SRL
                  {
                     setRegister( rd, getRegister( rs1 ) >> ( getRegister( rs2 ) & 0x1f ) );
                     newPC = oldPC + 4;

                     if( pInstruction )
                     {
                        pInstruction->set( oldPC, instr, "srl",
                           util::strvec
                           {
                              registerName( rd ),
                              registerName( rs1 ),
                              registerName( rs2 )
                           }
                        );
                     }
                     break;
                  }

                  case 1: // DIVU
                  {
                     if( getRegister( rs2 ) == 0 )
                     {
                        setRegister( rd, 0xffffffff );
                     } else
                     {
                        setRegister( rd, getRegister( rs1 ) / getRegister( rs2 ) );
                     }
                     newPC = oldPC + 4;

                     if( pInstruction )
                     {
                        pInstruction->set( oldPC, instr, "divu",
                           util::strvec
                           {
                              registerName( rd ),
                              registerName( rs1 ),
                              registerName( rs2 )
                           }
                        );
                     }
                     break;
                  }

                  case 0x20: // SRA
                  {
                     setRegister( rd, (int32_t)getRegister( rs1 ) >> ( getRegister( rs2 ) & 0x1f ) );
                     newPC = oldPC + 4;

                     if( pInstruction )
                     {
                        pInstruction->set( oldPC, instr, "sra",
                           util::strvec
                           {
                              registerName( rd ),
                              registerName( rs1 ),
                              registerName( rs2 )
                           }
                        );
                     }
                     break;
                  }

                  default:
                  {
                     unknownOpcode();
                     break;
                  }
               }
               break;
            }

            case 6:
            {
               switch( funct7 )
               {
                  case 0: // OR
                  {
                     setRegister( rd, getRegister( rs1 ) | getRegister( rs2 ) );
                     newPC = oldPC + 4;

                     if( pInstruction )
                     {
                        pInstruction->set( oldPC, instr, "or",
                           util::strvec
                           {
                              registerName( rd ),
                              registerName( rs1 ),
                              registerName( rs2 )
                           }
                        );
                     }
                     break;
                  }

                  case 1: // REM
                  {
                     if( getRegister( rs2 ) == 0 )
                     {
                        setRegister( rd, getRegister( rs1 ) );
                     } else
                     if( ( getRegister( rs1 ) == 0x80000000 ) && ( getRegister( rs2 ) == 0xffffffff ) ) // -2^LEN-1 / -1 -> overflow
                     {
                        setRegister( rd, 0 );
                     } else
                     {
                        setRegister( rd, (uint32_t)( (int32_t)getRegister( rs1 ) % (int32_t)getRegister( rs2 ) ) );
                     }
                     newPC = oldPC + 4;

                     if( pInstruction )
                     {
                        pInstruction->set( oldPC, instr, "rem",
                           util::strvec
                           {
                              registerName( rd ),
                              registerName( rs1 ),
                              registerName( rs2 )
                           }
                        );
                     }
                     break;
                  }

                  default:
                  {
                     unknownOpcode();
                     break;
                  }
               }
               break;
            }

            case 7:
            {
               switch( funct7 )
               {
                  case 0: // AND
                  {
                     setRegister( rd, getRegister( rs1 ) & getRegister( rs2 ) );
                     newPC = oldPC + 4;

                     if( pInstruction )
                     {
                        pInstruction->set( oldPC, instr, "and",
                           util::strvec
                           {
                              registerName( rd ),
                              registerName( rs1 ),
                              registerName( rs2 )
                           }
                        );
                     }
                     break;
                  }

                  case 1: // REMU
                  {
                     if( getRegister( rs2 ) == 0 )
                     {
                        setRegister( rd, getRegister( rs1 ) );
                     } else
                     {
                        setRegister( rd, getRegister( rs1 ) % getRegister( rs2 ) );
                     }
                     newPC = oldPC + 4;

                     if( pInstruction )
                     {
                        pInstruction->set( oldPC, instr, "remu",
                           util::strvec
                           {
                              registerName( rd ),
                              registerName( rs1 ),
                              registerName( rs2 )
                           }
                        );
                     }
                     break;
                  }

                  default:
                  {
                     unknownOpcode();
                     break;
                  }
               }
               break;
            }

            default:
            {
               unknownOpcode();
               break;
            }
         }
         break;
      }

      case 0x37: // LUI
      {
         setRegister( rd, uTypeImm );
         newPC = oldPC + 4;

         if( pInstruction )
         {
            pInstruction->set( oldPC, instr, "lui",
               util::strvec
               {
                  registerName( rd ),
                  stdformat( "0x{:x}", uTypeImm >> 12 )
               }
            );
         }
         break;
      }

      case 0x63: // CONDITIONAL BRANCH
      {
         switch( funct3 )
         {
            case 0: // BEQ
            {
               if( getRegister( rs1 ) == getRegister( rs2 ) )
               {
                  newPC = oldPC + bTypeImm;
               } else
               {
                  newPC = oldPC + 4;
               }

               if( pInstruction )
               {
                  pInstruction->set( oldPC, instr, "beq",
                     util::strvec
                     {
                        registerName( rs1 ),
                        registerName( rs2 ),
                        stdformat( "{}", bTypeImm )
                     },
                     stdformat( "{:x}", oldPC + bTypeImm )
                  );
               }
               break;
            }

            case 1: // BNE
            {
               if( getRegister( rs1 ) != getRegister( rs2 ) )
               {
                  newPC = oldPC + bTypeImm;
               } else
               {
                  newPC = oldPC + 4;
               }

               if( pInstruction )
               {
                  pInstruction->set( oldPC, instr, "bne",
                     util::strvec
                     {
                        registerName( rs1 ),
                        registerName( rs2 ),
                        stdformat( "{}", bTypeImm )
                     },
                     stdformat( "{:x}", oldPC + bTypeImm )
                  );
               }
               break;
            }

            case 4: // BLT
            {
               if( (int32_t)getRegister( rs1 ) < (int32_t)getRegister( rs2 ) )
               {
                  newPC = oldPC + bTypeImm;
               } else
               {
                  newPC = oldPC + 4;
               }

               if( pInstruction )
               {
                  pInstruction->set( oldPC, instr, "blt",
                     util::strvec
                     {
                        registerName( rs1 ),
                        registerName( rs2 ),
                        stdformat( "{}", bTypeImm )
                     },
                     stdformat( "{:x}", oldPC + bTypeImm )
                  );
               }
               break;
            }

            case 5: // BGE
            {
               if( (int32_t)getRegister( rs1 ) >= (int32_t)getRegister( rs2 ) )
               {
                  newPC = oldPC + bTypeImm;
               } else
               {
                  newPC = oldPC + 4;
               }

               if( pInstruction )
               {
                  pInstruction->set( oldPC, instr, "bge",
                     util::strvec
                     {
                        registerName( rs1 ),
                        registerName( rs2 ),
                        stdformat( "{}", bTypeImm )
                     },
                     stdformat( "{:x}", oldPC + bTypeImm )
                  );
               }
               break;
            }

            case 6: // BLTU
            {
               if( getRegister( rs1 ) < getRegister( rs2 ) )
               {
                  newPC = oldPC + bTypeImm;
               } else
               {
                  newPC = oldPC + 4;
               }

               if( pInstruction )
               {
                  pInstruction->set( oldPC, instr, "bltu",
                     util::strvec
                     {
                        registerName( rs1 ),
                        registerName( rs2 ),
                        stdformat( "{}", bTypeImm )
                     },
                     stdformat( "{:x}", oldPC + bTypeImm )
                  );
               }
               break;
            }

            case 7: // BGEU
            {
               if( getRegister( rs1 ) >= getRegister( rs2 ) )
               {
                  newPC = oldPC + bTypeImm;
               } else
               {
                  newPC = oldPC + 4;
               }

               if( pInstruction )
               {
                  pInstruction->set( oldPC, instr, "bgeu",
                     util::strvec
                     {
                        registerName( rs1 ),
                        registerName( rs2 ),
                        stdformat( "{}", bTypeImm )
                     },
                     stdformat( "{:x}", oldPC + bTypeImm )
                  );
               }
               break;
            }

            default:
            {
               unknownOpcode();
               break;
            }
         }
         break;
      }

      case 0x67:
      {
         switch( funct3 )
         {
            case 0: // JALR
            {
               setRegister( rd, oldPC + 4 );
               newPC = ( (int32_t)getRegister( rs1 ) + iTypeImm ) & 0xfffffffe;

               if( pInstruction )
               {
                  std::string comment;
                  if( ( rd == 0 ) && ( iTypeImm == 0 ) && ( rs1 == 1 ) )
                  {
                     comment = "ret";
                  }

                  pInstruction->set( oldPC, instr, "jalr",
                     util::strvec
                     {
                        registerName( rd ),
                        stdformat( "{}({})", iTypeImm, registerName( rs1 ) )
                     },
                     comment
                  );
               }
               break;
            }

            default:
            {
               unknownOpcode();
               break;
            }
         }
         break;
      }

      case 0x6f: // JAL
      {
         setRegister( rd, oldPC + 4 );
         newPC = oldPC + jTypeImm;

         if( pInstruction )
         {
            pInstruction->set( oldPC, instr, "jal",
               util::strvec
               {
                  registerName( rd ),
                  stdformat( "{}", jTypeImm )
               },
               stdformat( "{:8x}", oldPC + jTypeImm )
            );
         }
         break;
      }

      default:
      {
         unknownOpcode();
         break;
      }
   }

   m_PC = newPC;

   if( pInstruction )
   {
      std::string disass = pInstruction->toString();
      if( disass.size() > 1 )
      {
         printf( "%08x\t%s\n", pInstruction->getAddress(), disass.c_str() );
      } else
      {
         printf( "%08x\n", pInstruction->getAddress() );
      }
   }
}


/*----------------------------------------------------------------------------*/
/*! 2024-08-15
Invalidate an address reservation.
\param addr Start address
\param n Number of bytes
*/
/*----------------------------------------------------------------------------*/
void RISCV::invalidateReservation( uint32_t addr, int n )
{
   if( n < 1 )
      return;

   for( int i = 0; i < n; i++ )
   {
      if( m_ReservedAddresses.find( addr ) != m_ReservedAddresses.end() )
      {
         m_ReservedAddresses.erase( addr );
      }
      addr++;
   }
}


/*----------------------------------------------------------------------------*/
/*! 2024-08-15
Reserve some address(es)
\param The first memory address to be reserved
\param n The number of addresses
*/
/*----------------------------------------------------------------------------*/
void RISCV::reserveAddr( uint32_t addr, int n )
{
   if( n < 1 )
      return;

   clearAllReservations();
   for( int i = 0; i < n; i++ )
   {
      m_ReservedAddresses[addr++] = true;
   }
}


/*----------------------------------------------------------------------------*/
/*! 2024-08-15
Clear all address reservations.
*/
/*----------------------------------------------------------------------------*/
void RISCV::clearAllReservations()
{
   m_ReservedAddresses.clear();
}


/*----------------------------------------------------------------------------*/
/*! 2024-08-15
*/
/*----------------------------------------------------------------------------*/
int RISCV::numReservedAddresses( uint32_t addr, int n ) const
{
   if( n < 1 )
      return( 0 );

   int numReserved = 0;
   for( int i = 0; i < n; i++ )
   {
      if( m_ReservedAddresses.find( addr ) != m_ReservedAddresses.end() )
         numReserved++;
   }

   return( numReserved );
}


/*----------------------------------------------------------------------------*/
/*! 2024-08-15
Read a byte.
*/
/*----------------------------------------------------------------------------*/
uint8_t RISCV::readMem8( uint32_t address )
{
   return( m_pMemory->readMem8( address ) );
}


/*----------------------------------------------------------------------------*/
/*! 2024-08-15
Read a half-word.
*/
/*----------------------------------------------------------------------------*/
uint16_t RISCV::readMem16( uint32_t address )
{
   return( m_pMemory->readMem16( address ) );
}


/*----------------------------------------------------------------------------*/
/*! 2024-08-15
Read a word.
*/
/*----------------------------------------------------------------------------*/
uint32_t RISCV::readMem32( uint32_t address )
{
   return( m_pMemory->readMem32( address ) );
}


/*----------------------------------------------------------------------------*/
/*! 2024-08-15
Write a byte.
*/
/*----------------------------------------------------------------------------*/
void RISCV::writeMem8( uint32_t address, uint8_t d )
{
   invalidateReservation( address );
   m_pMemory->writeMem8( address, d );
}


/*----------------------------------------------------------------------------*/
/*! 2024-08-15
Write a half-word.
*/
/*----------------------------------------------------------------------------*/
void RISCV::writeMem16( uint32_t address, uint16_t d )
{
   invalidateReservation( address, 2 );
   m_pMemory->writeMem16( address, d );
}


/*----------------------------------------------------------------------------*/
/*! 2024-08-15
Write a word.
*/
/*----------------------------------------------------------------------------*/
void RISCV::writeMem32( uint32_t address, uint32_t d )
{
   invalidateReservation( address, 4 );
   m_pMemory->writeMem32( address, d );
}


/*----------------------------------------------------------------------------*/
/*! 2024-08-15
Constructor for class RISCV::Instruction.
\param address Memory address of the instruction
\param code The full binary instruction code
\param instruction The disassembly of the opcode
\param parameters All operands in textual form
\param comment An optional comment with further explanations
*/
/*----------------------------------------------------------------------------*/
RISCV::Instruction::Instruction( uint32_t address, uint32_t code, std::string instruction, std::vector<std::string> parameters, std::string comment )
{
   set( address, code, instruction, parameters, comment );
}


/*----------------------------------------------------------------------------*/
/*! 2024-08-15
Constructor for class RISCV::Instruction.
*/
/*----------------------------------------------------------------------------*/
RISCV::Instruction::Instruction()
{
}


/*----------------------------------------------------------------------------*/
/*! 2024-08-15
Destructor for class RISCV::Instruction.
*/
/*----------------------------------------------------------------------------*/
RISCV::Instruction::~Instruction()
{
}


/*----------------------------------------------------------------------------*/
/*! 2024-08-15
Convert the instruction to a complete textual line of assembly code.
\return The assembly code
*/
/*----------------------------------------------------------------------------*/
std::string RISCV::Instruction::toString() const
{
   std::string r = stdformat( "{}\t{}", m_Instruction, util::join( m_Parameters, "," ) );
   if( m_Comment.size() > 0 )
   {
      r = r + " # " + m_Comment;
   }

   return( r );
}


/*----------------------------------------------------------------------------*/
/*! 2024-08-15
*/
/*----------------------------------------------------------------------------*/
void RISCV::Instruction::set( uint32_t address, uint32_t code, std::string instruction, std::vector<std::string> parameters, std::string comment )
{
   m_Address = address;
   m_Code = code;
   m_Instruction = instruction;
   m_Parameters = parameters;
   m_Comment = comment;
}


/*----------------------------------------------------------------------------*/
/*! 2024-08-15
Set the object attributes from another Instruction instance.
\param o Reference to the other Instruction instance
*/
/*----------------------------------------------------------------------------*/
void RISCV::Instruction::set( const Instruction &o )
{
   set( o.getAddress(), o.getCode(), o.getInstruction(), o.getParameters(), o.getComment() );
}


/*----------------------------------------------------------------------------*/
/*! 2024-08-15
\return Memory address of the instruction
*/
/*----------------------------------------------------------------------------*/
uint32_t RISCV::Instruction::getAddress() const
{
   return( m_Address );
}


/*----------------------------------------------------------------------------*/
/*! 2024-08-15
\return The full binary instruction code
*/
/*----------------------------------------------------------------------------*/
uint32_t RISCV::Instruction::getCode() const
{
   return( m_Code );
}


/*----------------------------------------------------------------------------*/
/*! 2024-08-15
\return The disassembly of the opcode
*/
/*----------------------------------------------------------------------------*/
std::string RISCV::Instruction::getInstruction() const
{
   return( m_Instruction );
}


/*----------------------------------------------------------------------------*/
/*! 2024-08-15
\return All operands in textual form
*/
/*----------------------------------------------------------------------------*/
std::vector<std::string> RISCV::Instruction::getParameters() const
{
   return( m_Parameters );
}


/*----------------------------------------------------------------------------*/
/*! 2024-08-15
\return An optional comment with further explanations
*/
/*----------------------------------------------------------------------------*/
std::string RISCV::Instruction::getComment() const
{
   return( m_Comment );
}
