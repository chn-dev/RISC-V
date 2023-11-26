#include <stdio.h>
#include <string>

#include "RISCV.h"

RISCV::RISCV( MemoryInterface *pMem ) :
   m_pMemory( pMem )
{
   reset();
}


RISCV::~RISCV()
{
}


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


void RISCV::reset()
{
   m_PC = 0x80000000;
   for( int i = 0; i < 32; i++ )
   {
      m_Registers[i] = 0;
   }
}


uint32_t RISCV::getRegister( int r )
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


void RISCV::setRegister( int r, uint32_t v )
{
   r &= 0x1f;

   if( r != 0 )
   {
      m_Registers[r] = v;
   }
}


void RISCV::unknownOpcode()
{
   printf("\n");
   exit( 1 );
}


void RISCV::step()
{
   uint32_t instr = m_pMemory->readMem32( m_PC );
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
   // Propagate the sign bit of the J-Type immediate
   if( instr & 0x80000000 )
      bTypeImm |= 0b11111111111 << 21;

   uint32_t oldPC = m_PC;

   printf( "PC=0x%08x\n", m_PC );

   switch( opcode )
   {
      case 0x03: // LOAD
      {
         switch( funct3 ) // width
         {
            case 0: // LB
            {
               uint32_t d = m_pMemory->readMem8( getRegister( rs1 ) + iTypeImm );
               if( d & 0x80 )
                  d |= 0xffffff00;
               setRegister( rd, d );
               m_PC += 4;
               break;
            }

            case 1: // LH
            {
               uint32_t d = m_pMemory->readMem16( getRegister( rs1 ) + iTypeImm );
               if( d & 0x8000 )
                  d |= 0xffff0000;
               setRegister( rd, d );
               m_PC += 4;
               break;
            }

            case 2: // LW
            {
               setRegister( rd, m_pMemory->readMem32( getRegister( rs1 ) + iTypeImm ) );
               m_PC += 4;
               break;
            }

            case 4: // LBU
            {
               setRegister( rd, m_pMemory->readMem8( getRegister( rs1 ) + iTypeImm ) );
               m_PC += 4;
               break;
            }

            case 5: // LHU
            {
               setRegister( rd, m_pMemory->readMem16( getRegister( rs1 ) + iTypeImm ) );
               m_PC += 4;
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
               fprintf( stderr, "[0x%08x] - 0x%08x: addi\t%s,%s,%d # -> 0x%08x\n",
                  m_PC,
                  instr,
                  registerName( rd ).c_str(),
                  registerName( rs1 ).c_str(),
                  iTypeImm,
                  getRegister( rs1 ) + iTypeImm );
               setRegister( rd, getRegister( rs1 ) + iTypeImm );
               m_PC += 4;
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
                     m_PC += 4;
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
               m_PC += 4;
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
               m_PC += 4;
               break;
            }

            case 4: // XORI
            {
               setRegister( rd, getRegister( rs1 ) ^ (uint32_t)iTypeImm );
               m_PC += 4;
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
                     m_PC += 4;
                     break;
                  }

                  case 0x20: // SRAI
                  {
                     uint32_t shamt = ( instr >> 20 ) & 0x1f;
                     setRegister( rd, (int32_t)getRegister( rs1 ) >> shamt );
                     m_PC += 4;
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
               m_PC += 4;
               break;
            }

            case 7: // ANDI
            {
               setRegister( rd, getRegister( rs1 ) & (uint32_t)iTypeImm );
               m_PC += 4;
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
         setRegister( rd, m_PC + uTypeImm );
         fprintf( stderr, "[0x%08x] - 0x%08x: auipc\t%s,0x%08x (->%08x)\n", m_PC, instr, registerName( rd ).c_str(), ( instr & 0xfffff000 ) >> 12, (uint32_t)( m_PC + uTypeImm ) );
         m_PC += 4;
         break;
      }

      case 0x23: // STORE
      {
         switch( funct3 ) // width
         {
            case 0: // SB
            {
               m_pMemory->writeMem8( getRegister( rs1 ) + sTypeImm, getRegister( rs2 ) & 0xff );
               fprintf( stderr, "%08x sb\t%s,%d(%s) # writeMem( 0x%08x, 0x%02x )\n",
                  m_PC,
                  registerName( rs2 ).c_str(),
                  sTypeImm,
                  registerName( rs1 ).c_str(),
                  getRegister( rs1 ) + sTypeImm, getRegister( rs2 ) & 0xff );
               m_PC += 4;
               break;
            }

            case 1: // SH
            {
               m_pMemory->writeMem16( getRegister( rs1 ) + sTypeImm, getRegister( rs2 ) & 0xffff );
               m_PC += 4;
               break;
            }

            case 2: // SW
            {
               fprintf( stderr, "%08x sw\t%s,%d(%s) # writeMem( 0x%08x, 0x%08x )\n",
                  m_PC,
                  registerName( rs2 ).c_str(),
                  sTypeImm,
                  registerName( rs1 ).c_str(),
                  getRegister( rs1 ) + sTypeImm, getRegister( rs2 ) );

               m_pMemory->writeMem32( getRegister( rs1 ) + sTypeImm, getRegister( rs2 ) );
               m_PC += 4;
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
                     m_PC += 4;
                     break;
                  }

                  case 0x01: // MUL
                  {
                     setRegister( rd, (uint32_t)( ( (int64_t)getRegister( rs1 ) * (int64_t)getRegister( rs2 ) ) & 0xffffffff ) );
                     m_PC += 4;
                     break;
                  }

                  case 0x20: // SUB
                  {
                     setRegister( rd, (int32_t)getRegister( rs1 ) - (int32_t)getRegister( rs2 ) );
                     m_PC += 4;
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
                     m_PC += 4;
                     break;
                  }

                  case 1: // MULH
                  {
                     setRegister( rd, ( ( (int64_t)getRegister( rs1 ) * (int64_t)getRegister( rs2 ) ) >> 32 ) & 0xffffffff );
                     m_PC += 4;
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
                     m_PC += 4;
                     break;
                  }

                  case 1: // MULHSU
                  {
                     setRegister( rd, (uint32_t)( ( ( (int64_t)getRegister( rs1 ) * (uint64_t)getRegister( rs2 ) ) >> 32 ) & 0xffffffff ) );
                     m_PC += 4;
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
                     m_PC += 4;
                     break;
                  }

                  case 1: // MULHU
                  {
                     setRegister( rd, (uint32_t)( ( ( (uint64_t)getRegister( rs1 ) * (uint64_t)getRegister( rs2 ) ) >> 32 ) & 0xffffffff ) );
                     m_PC += 4;
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
                     m_PC += 4;
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
                     m_PC += 4;
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
                     m_PC += 4;
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
                     m_PC += 4;
                     break;
                  }

                  case 0x20: // SRA
                  {
                     setRegister( rd, (int32_t)getRegister( rs1 ) >> ( getRegister( rs2 ) & 0x1f ) );
                     m_PC += 4;
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
                     m_PC += 4;
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
                     m_PC += 4;
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
                     m_PC += 4;
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
                     m_PC += 4;
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
         m_PC += 4;
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
                  m_PC += bTypeImm;
               } else
               {
                  m_PC += 4;
               }
               break;
            }

            case 1: // BNE
            {
               if( getRegister( rs1 ) != getRegister( rs2 ) )
               {
                  m_PC += bTypeImm;
               } else
               {
                  m_PC += 4;
               }
               break;
            }

            case 4: // BLT
            {
               if( (int32_t)getRegister( rs1 ) < (int32_t)getRegister( rs2 ) )
               {
                  m_PC += bTypeImm;
               } else
               {
                  m_PC += 4;
               }
               break;
            }

            case 5: // BGE
            {
               if( (int32_t)getRegister( rs1 ) >= (int32_t)getRegister( rs2 ) )
               {
                  m_PC += bTypeImm;
               } else
               {
                  m_PC += 4;
               }
               break;
            }

            case 6: // BLTU
            {
               if( getRegister( rs1 ) < getRegister( rs2 ) )
               {
                  m_PC += bTypeImm;
               } else
               {
                  m_PC += 4;
               }
               break;
            }

            case 7: // BGEU
            {
               if( getRegister( rs1 ) >= getRegister( rs2 ) )
               {
                  m_PC += bTypeImm;
               } else
               {
                  m_PC += 4;
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
               setRegister( rd, m_PC + 4 );
               m_PC = ( (int32_t)getRegister( rs1 ) + iTypeImm ) & 0xfffffffe;
               break;
            }

            default:
            {
               unknownOpcode();
               break;
            }
         }
      }

      case 0x6f: // JAL
      {
         setRegister( rd, m_PC + 4 );
         m_PC += jTypeImm;
         break;
      }

      default:
      {
         unknownOpcode();
         break;
      }
   }
}
