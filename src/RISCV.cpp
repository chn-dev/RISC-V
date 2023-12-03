#include <stdio.h>
#include <string>

#include "RISCV.h"

RISCV::Instruction::Instruction( uint32_t address, uint32_t code, std::string instruction, std::vector<std::string> parameters, std::string comment )
{
   set( address, code, instruction, parameters, comment );
}


RISCV::Instruction::Instruction()
{
}


RISCV::Instruction::~Instruction()
{
}


std::string RISCV::Instruction::toString() const
{
   std::string r = stdformat( "{}\t{}", m_Instruction, util::join( m_Parameters, "," ) );
   if( m_Comment.size() > 0 )
   {
      r = r + " # " + m_Comment;
   }

   return( r );
}


void RISCV::Instruction::set( uint32_t address, uint32_t code, std::string instruction, std::vector<std::string> parameters, std::string comment )
{
   m_Address = address;
   m_Code = code;
   m_Instruction = instruction;
   m_Parameters = parameters;
   m_Comment = comment;
}


void RISCV::Instruction::set( const Instruction &o )
{
   set( o.getAddress(), o.getCode(), o.getInstruction(), o.getParameters(), o.getComment() );
}


uint32_t RISCV::Instruction::getAddress() const
{
   return( m_Address );
}


uint32_t RISCV::Instruction::getCode() const
{
   return( m_Code );
}


std::string RISCV::Instruction::getInstruction() const
{
   return( m_Instruction );
}


std::vector<std::string> RISCV::Instruction::getParameters() const
{
   return( m_Parameters );
}


std::string RISCV::Instruction::getComment() const
{
   return( m_Comment );
}


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
   while(1);
   exit( 1 );
}


uint32_t RISCV::step( uint32_t oldPC, uint32_t instr, RISCV *pCPU, Instruction *pInstruction )
{
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

   bool oldPCValid = false;
   if( pCPU )
   {
      oldPC = pCPU->m_PC;
      oldPCValid = true;
   }

   uint32_t newPC = oldPC;

   switch( opcode )
   {
      case 0x03: // LOAD
      {
         switch( funct3 ) // width
         {
            case 0: // LB
            {
               if( pCPU )
               {
                  uint32_t d = pCPU->readMem8( pCPU->getRegister( rs1 ) + iTypeImm );
                  if( d & 0x80 )
                     d |= 0xffffff00;
                  pCPU->setRegister( rd, d );
                  newPC = oldPC + 4;
               }

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
               if( pCPU )
               {
                  uint32_t d = pCPU->readMem16( pCPU->getRegister( rs1 ) + iTypeImm );
                  if( d & 0x8000 )
                     d |= 0xffff0000;
                  pCPU->setRegister( rd, d );
                  newPC = oldPC + 4;
               }

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
               if( pCPU )
               {
                  pCPU->setRegister( rd, pCPU->readMem32( pCPU->getRegister( rs1 ) + iTypeImm ) );
                  newPC = oldPC + 4;
               }

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
               if( pCPU )
               {
                  pCPU->setRegister( rd, pCPU->readMem8( pCPU->getRegister( rs1 ) + iTypeImm ) );
                  newPC = oldPC + 4;
               }

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
               if( pCPU )
               {
                  pCPU->setRegister( rd, pCPU->readMem16( pCPU->getRegister( rs1 ) + iTypeImm ) );
                  newPC = oldPC + 4;
               }

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

            default:
            {
               if( pCPU )
               {
                  pCPU->unknownOpcode();
               }
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
               if( pCPU )
               {
                  pCPU->setRegister( rd, pCPU->getRegister( rs1 ) + iTypeImm );
                  newPC = oldPC + 4;
               }

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
                     if( pCPU )
                     {
                        pCPU->setRegister( rd, pCPU->getRegister( rs1 ) << shamt );
                        newPC = oldPC + 4;
                     }

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
                     if( pCPU )
                     {
                        pCPU->unknownOpcode();
                     }
                     break;
                  }
               }
               break;
            }

            case 2: // SLTI
            {
               if( pCPU )
               {
                  if( (int32_t)pCPU->getRegister( rs1 ) < iTypeImm )
                  {
                     pCPU->setRegister( rd, 1 );
                  } else
                  {
                     pCPU->setRegister( rd, 0 );
                  }
                  newPC = oldPC + 4;
               }

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
               if( pCPU )
               {
                  if( pCPU->getRegister( rs1 ) < (uint32_t)iTypeImm )
                  {
                     pCPU->setRegister( rd, 1 );
                  } else
                  {
                     pCPU->setRegister( rd, 0 );
                  }
                  newPC = oldPC + 4;
               }

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
               if( pCPU )
               {
                  pCPU->setRegister( rd, pCPU->getRegister( rs1 ) ^ (uint32_t)iTypeImm );
                  newPC = oldPC + 4;
               }

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
                     if( pCPU )
                     {
                        pCPU->setRegister( rd, pCPU->getRegister( rs1 ) >> shamt );
                        newPC = oldPC + 4;
                     }

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
                     if( pCPU )
                     {
                        pCPU->setRegister( rd, (int32_t)pCPU->getRegister( rs1 ) >> shamt );
                        newPC = oldPC + 4;
                     }

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
                     if( pCPU )
                     {
                        pCPU->unknownOpcode();
                     }
                     break;
                  }
               }
               break;
            }

            case 6: // ORI
            {
               if( pCPU )
               {
                  pCPU->setRegister( rd, pCPU->getRegister( rs1 ) | (uint32_t)iTypeImm );
                  newPC = oldPC + 4;
               }

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
               if( pCPU )
               {
                  pCPU->setRegister( rd, pCPU->getRegister( rs1 ) & (uint32_t)iTypeImm );
                  newPC = oldPC + 4;
               }

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
               if( pCPU )
               {
                  pCPU->unknownOpcode();
               }
               break;
            }
         }
         break;
      }

      case 0x17: // AUIPC
      {
         if( pCPU )
         {
            pCPU->setRegister( rd, oldPC + uTypeImm );
            newPC = oldPC + 4;
         }

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
               if( pCPU )
               {
                  pCPU->writeMem8( pCPU->getRegister( rs1 ) + sTypeImm, pCPU->getRegister( rs2 ) & 0xff );
                  newPC = oldPC + 4;
               }

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
               if( pCPU )
               {
                  pCPU->writeMem16( pCPU->getRegister( rs1 ) + sTypeImm, pCPU->getRegister( rs2 ) & 0xffff );
                  newPC = oldPC + 4;
               }

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
               if( pCPU )
               {
                  pCPU->writeMem32( pCPU->getRegister( rs1 ) + sTypeImm, pCPU->getRegister( rs2 ) );
                  newPC = oldPC + 4;
               }

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
               if( pCPU )
               {
                  pCPU->unknownOpcode();
               }
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
                     if( pCPU )
                     {
                        uint32_t address = pCPU->getRegister( rs1 );
                        uint32_t v = pCPU->readMem32( address );
                        pCPU->setRegister( rd, v );
                        v = v + pCPU->getRegister( rs2 );
                        pCPU->writeMem32( address, v );
                        newPC = oldPC + 4;
                     }

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
                     if( pCPU )
                     {
                        uint32_t address = pCPU->getRegister( rs1 );
                        uint32_t v = pCPU->readMem32( address );
                        pCPU->setRegister( rd, v );
                        v = pCPU->getRegister( rs2 );
                        pCPU->writeMem32( address, v );
                        newPC = oldPC + 4;
                     }

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
                     if( pCPU )
                     {
                        uint32_t addr = pCPU->getRegister( rs1 );
                        pCPU->reserveAddr( addr, 4 );
                        pCPU->setRegister( rd, pCPU->readMem32( pCPU->getRegister( rs1 ) ) );

                        newPC = oldPC + 4;
                     }

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
                     if( pCPU )
                     {
                        if( pCPU->numReservedAddresses( pCPU->getRegister( rs1 ), 4 ) == 4 )
                        {
                           // All accessed addresses have been reserved -> success
                           pCPU->writeMem32( pCPU->getRegister( rs1 ), pCPU->getRegister( rs2 ) );
                           pCPU->setRegister( rd, 0 );
                        } else
                        {
                           pCPU->setRegister( rd, 1 );
                        }

                        pCPU->clearAllReservations();

                        newPC = oldPC + 4;
                     }

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
                     if( pCPU )
                     {
                        uint32_t address = pCPU->getRegister( rs1 );
                        uint32_t v = pCPU->readMem32( address );
                        pCPU->setRegister( rd, v );
                        v = v ^ pCPU->getRegister( rs2 );
                        pCPU->writeMem32( address, v );
                        newPC = oldPC + 4;
                     }

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
                     if( pCPU )
                     {
                        uint32_t address = pCPU->getRegister( rs1 );
                        uint32_t v = pCPU->readMem32( address );
                        pCPU->setRegister( rd, v );
                        v = v | pCPU->getRegister( rs2 );
                        pCPU->writeMem32( address, v );
                        newPC = oldPC + 4;
                     }

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
                     if( pCPU )
                     {
                        uint32_t address = pCPU->getRegister( rs1 );
                        uint32_t v = pCPU->readMem32( address );
                        pCPU->setRegister( rd, v );
                        v = v & pCPU->getRegister( rs2 );
                        pCPU->writeMem32( address, v );
                        newPC = oldPC + 4;
                     }

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
                     if( pCPU )
                     {
                        uint32_t address = pCPU->getRegister( rs1 );
                        uint32_t v = pCPU->readMem32( address );
                        pCPU->setRegister( rd, v );
                        uint32_t vrs2 = pCPU->getRegister( rs2 );
                        v = (int32_t)v < (int32_t)vrs2 ? v : vrs2;
                        pCPU->writeMem32( address, v );
                        newPC = oldPC + 4;
                     }

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
                     if( pCPU )
                     {
                        uint32_t address = pCPU->getRegister( rs1 );
                        uint32_t v = pCPU->readMem32( address );
                        pCPU->setRegister( rd, v );
                        uint32_t vrs2 = pCPU->getRegister( rs2 );
                        v = (int32_t)v > (int32_t)vrs2 ? v : vrs2;
                        pCPU->writeMem32( address, v );
                        newPC = oldPC + 4;
                     }

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
                     if( pCPU )
                     {
                        uint32_t address = pCPU->getRegister( rs1 );
                        uint32_t v = pCPU->readMem32( address );
                        pCPU->setRegister( rd, v );
                        uint32_t vrs2 = pCPU->getRegister( rs2 );
                        v = v < vrs2 ? v : vrs2;
                        pCPU->writeMem32( address, v );
                        newPC = oldPC + 4;
                     }

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
                     if( pCPU )
                     {
                        uint32_t address = pCPU->getRegister( rs1 );
                        uint32_t v = pCPU->readMem32( address );
                        pCPU->setRegister( rd, v );
                        uint32_t vrs2 = pCPU->getRegister( rs2 );
                        v = v > vrs2 ? v : vrs2;
                        pCPU->writeMem32( address, v );
                        newPC = oldPC + 4;
                     }

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
                     if( pCPU )
                     {
                        pCPU->unknownOpcode();
                     }
                     break;
                  }
               }
               break;
            }

            default:
            {
               if( pCPU )
               {
                  pCPU->unknownOpcode();
               }
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
                     if( pCPU )
                     {
                        pCPU->setRegister( rd, (int32_t)pCPU->getRegister( rs1 ) + (int32_t)pCPU->getRegister( rs2 ) );
                        newPC = oldPC + 4;
                     }

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
                     if( pCPU )
                     {
                        pCPU->setRegister( rd, (uint32_t)( ( (int64_t)pCPU->getRegister( rs1 ) * (int64_t)pCPU->getRegister( rs2 ) ) & 0xffffffff ) );
                        newPC = oldPC + 4;
                     }

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
                     if( pCPU )
                     {
                        pCPU->setRegister( rd, (int32_t)pCPU->getRegister( rs1 ) - (int32_t)pCPU->getRegister( rs2 ) );
                        newPC = oldPC + 4;
                     }

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
                     if( pCPU )
                     {
                        pCPU->unknownOpcode();
                     }
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
                     if( pCPU )
                     {
                        pCPU->setRegister( rd, pCPU->getRegister( rs1 ) << ( pCPU->getRegister( rs2 ) & 0x1f ) );
                        newPC = oldPC + 4;
                     }

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
                     if( pCPU )
                     {
                        pCPU->setRegister( rd, ( ( (int64_t)pCPU->getRegister( rs1 ) * (int64_t)pCPU->getRegister( rs2 ) ) >> 32 ) & 0xffffffff );
                        newPC = oldPC + 4;
                     }

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
                     if( pCPU )
                     {
                        pCPU->unknownOpcode();
                     }
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
                     if( pCPU )
                     {
                        if( (int32_t)pCPU->getRegister( rs1 ) < (int32_t)pCPU->getRegister( rs2 ) )
                        {
                           pCPU->setRegister( rd, 1 );
                        } else
                        {
                           pCPU->setRegister( rd, 0 );
                        }
                        newPC = oldPC + 4;
                     }

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
                     if( pCPU )
                     {
                        pCPU->setRegister( rd, (uint32_t)( ( ( (int64_t)pCPU->getRegister( rs1 ) * (uint64_t)pCPU->getRegister( rs2 ) ) >> 32 ) & 0xffffffff ) );
                        newPC = oldPC + 4;
                     }

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
                     if( pCPU )
                     {
                        pCPU->unknownOpcode();
                     }
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
                     if( pCPU )
                     {
                        if( pCPU->getRegister( rs1 ) < pCPU->getRegister( rs2 ) )
                        {
                           pCPU->setRegister( rd, 1 );
                        } else
                        {
                           pCPU->setRegister( rd, 0 );
                        }
                        newPC = oldPC + 4;
                     }

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
                     if( pCPU )
                     {
                        pCPU->setRegister( rd, (uint32_t)( ( ( (uint64_t)pCPU->getRegister( rs1 ) * (uint64_t)pCPU->getRegister( rs2 ) ) >> 32 ) & 0xffffffff ) );
                        newPC = oldPC + 4;
                     }

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
                     if( pCPU )
                     {
                        pCPU->unknownOpcode();
                     }
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
                     if( pCPU )
                     {
                        pCPU->setRegister( rd, pCPU->getRegister( rs1 ) ^ pCPU->getRegister( rs2 ) );
                        newPC = oldPC + 4;
                     }

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
                     if( pCPU )
                     {
                        if( pCPU->getRegister( rs2 ) == 0 )
                        {
                           pCPU->setRegister( rd, 0xffffffff );
                        } else
                        if( ( pCPU->getRegister( rs1 ) == 0x80000000 ) && ( pCPU->getRegister( rs2 ) == 0xffffffff ) ) // -2^LEN-1 / -1 -> overflow
                        {
                           pCPU->setRegister( rd, 0x80000000 );
                        } else
                        {
                           pCPU->setRegister( rd, (uint32_t)( (int32_t)pCPU->getRegister( rs1 ) / (int32_t)pCPU->getRegister( rs2 ) ) );
                        }
                        newPC = oldPC + 4;
                     }

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
                     if( pCPU )
                     {
                        pCPU->unknownOpcode();
                     }
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
                     if( pCPU )
                     {
                        pCPU->setRegister( rd, pCPU->getRegister( rs1 ) >> ( pCPU->getRegister( rs2 ) & 0x1f ) );
                        newPC = oldPC + 4;
                     }

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
                     if( pCPU )
                     {
                        if( pCPU->getRegister( rs2 ) == 0 )
                        {
                           pCPU->setRegister( rd, 0xffffffff );
                        } else
                        {
                           pCPU->setRegister( rd, pCPU->getRegister( rs1 ) / pCPU->getRegister( rs2 ) );
                        }
                        newPC = oldPC + 4;
                     }

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
                     if( pCPU )
                     {
                        pCPU->setRegister( rd, (int32_t)pCPU->getRegister( rs1 ) >> ( pCPU->getRegister( rs2 ) & 0x1f ) );
                        newPC = oldPC + 4;
                     }

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
                     if( pCPU )
                     {
                        pCPU->unknownOpcode();
                     }
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
                     if( pCPU )
                     {
                        pCPU->setRegister( rd, pCPU->getRegister( rs1 ) | pCPU->getRegister( rs2 ) );
                        newPC = oldPC + 4;
                     }

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
                     if( pCPU )
                     {
                        if( pCPU->getRegister( rs2 ) == 0 )
                        {
                           pCPU->setRegister( rd, pCPU->getRegister( rs1 ) );
                        } else
                        if( ( pCPU->getRegister( rs1 ) == 0x80000000 ) && ( pCPU->getRegister( rs2 ) == 0xffffffff ) ) // -2^LEN-1 / -1 -> overflow
                        {
                           pCPU->setRegister( rd, 0 );
                        } else
                        {
                           pCPU->setRegister( rd, (uint32_t)( (int32_t)pCPU->getRegister( rs1 ) % (int32_t)pCPU->getRegister( rs2 ) ) );
                        }
                        newPC = oldPC + 4;
                     }

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
                     if( pCPU )
                     {
                        pCPU->unknownOpcode();
                     }
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
                     if( pCPU )
                     {
                        pCPU->setRegister( rd, pCPU->getRegister( rs1 ) & pCPU->getRegister( rs2 ) );
                        newPC = oldPC + 4;
                     }

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
                     if( pCPU )
                     {
                        if( pCPU->getRegister( rs2 ) == 0 )
                        {
                           pCPU->setRegister( rd, pCPU->getRegister( rs1 ) );
                        } else
                        {
                           pCPU->setRegister( rd, pCPU->getRegister( rs1 ) % pCPU->getRegister( rs2 ) );
                        }
                        newPC = oldPC + 4;
                     }

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
                     if( pCPU )
                     {
                        pCPU->unknownOpcode();
                     }
                     break;
                  }
               }
               break;
            }

            default:
            {
               if( pCPU )
               {
                  pCPU->unknownOpcode();
               }
               break;
            }
         }
         break;
      }

      case 0x37: // LUI
      {
         if( pCPU )
         {
            pCPU->setRegister( rd, uTypeImm );
            newPC = oldPC + 4;
         }

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
               if( pCPU )
               {
                  if( pCPU->getRegister( rs1 ) == pCPU->getRegister( rs2 ) )
                  {
                     newPC = oldPC + bTypeImm;
                  } else
                  {
                     newPC = oldPC + 4;
                  }
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
               if( pCPU )
               {
                  if( pCPU->getRegister( rs1 ) != pCPU->getRegister( rs2 ) )
                  {
                     newPC = oldPC + bTypeImm;
                  } else
                  {
                     newPC = oldPC + 4;
                  }
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
               if( pCPU )
               {
                  if( (int32_t)pCPU->getRegister( rs1 ) < (int32_t)pCPU->getRegister( rs2 ) )
                  {
                     newPC = oldPC + bTypeImm;
                  } else
                  {
                     newPC = oldPC + 4;
                  }
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
               if( pCPU )
               {
                  if( (int32_t)pCPU->getRegister( rs1 ) >= (int32_t)pCPU->getRegister( rs2 ) )
                  {
                     newPC = oldPC + bTypeImm;
                  } else
                  {
                     newPC = oldPC + 4;
                  }
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
               if( pCPU )
               {
                  if( pCPU->getRegister( rs1 ) < pCPU->getRegister( rs2 ) )
                  {
                     newPC = oldPC + bTypeImm;
                  } else
                  {
                     newPC = oldPC + 4;
                  }
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
               if( pCPU )
               {
                  if( pCPU->getRegister( rs1 ) >= pCPU->getRegister( rs2 ) )
                  {
                     newPC = oldPC + bTypeImm;
                  } else
                  {
                     newPC = oldPC + 4;
                  }
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
               if( pCPU )
               {
                  pCPU->unknownOpcode();
               }
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
               if( pCPU )
               {
                  pCPU->setRegister( rd, oldPC + 4 );
                  newPC = ( (int32_t)pCPU->getRegister( rs1 ) + iTypeImm ) & 0xfffffffe;
               }

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
               if( pCPU )
               {
                  pCPU->unknownOpcode();
               }
               break;
            }
         }
         break;
      }

      case 0x6f: // JAL
      {
         if( pCPU )
         {
            pCPU->setRegister( rd, oldPC + 4 );
            newPC = oldPC + jTypeImm;
         }

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
         if( pCPU )
         {
            pCPU->unknownOpcode();
         }
         break;
      }
   }

   if( pCPU )
   {
      pCPU->m_PC = newPC;
   }

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


void RISCV::step()
{
   uint32_t instr = readMem32( m_PC );
   Instruction disassembly;
   step( m_PC, instr, this, &disassembly );
}


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


void RISCV::clearAllReservations()
{
   m_ReservedAddresses.clear();
}


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


uint8_t RISCV::readMem8( uint32_t address )
{
   return( m_pMemory->readMem8( address ) );
}


uint16_t RISCV::readMem16( uint32_t address )
{
   return( m_pMemory->readMem16( address ) );
}


uint32_t RISCV::readMem32( uint32_t address )
{
   return( m_pMemory->readMem32( address ) );
}


void RISCV::writeMem8( uint32_t address, uint8_t d )
{
   invalidateReservation( address );
   m_pMemory->writeMem8( address, d );
}


void RISCV::writeMem16( uint32_t address, uint16_t d )
{
   invalidateReservation( address, 2 );
   m_pMemory->writeMem16( address, d );
}


void RISCV::writeMem32( uint32_t address, uint32_t d )
{
   invalidateReservation( address, 4 );
   m_pMemory->writeMem32( address, d );
}
