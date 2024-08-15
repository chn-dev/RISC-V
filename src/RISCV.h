/*----------------------------------------------------------------------------*/
/*!
\file RISCV.h
\author Christian Nowak <chnowak@web.de>
\brief Headerfile for class RISCV.
*/
/*----------------------------------------------------------------------------*/
#ifndef __RISCV_H__
#define __RISCV_H__

#include <vector>
#include <string>
#include <cstdint>
#include <map>

#include "util.h"

/*----------------------------------------------------------------------------*/
/*!
\class RISCV
\date  2024-08-14
*/
/*----------------------------------------------------------------------------*/
class RISCV
{
   public:
      class MemoryInterface
      {
         public:
            virtual uint8_t readMem8( uint32_t address ) = 0;
            virtual uint16_t readMem16( uint32_t address ) = 0;
            virtual uint32_t readMem32( uint32_t address ) = 0;
            virtual void writeMem8( uint32_t address, uint8_t d ) = 0;
            virtual void writeMem16( uint32_t address, uint16_t d ) = 0;
            virtual void writeMem32( uint32_t address, uint32_t d ) = 0;
            virtual void unknownOpcode() = 0;
      };

      class Instruction
      {
         public:
            Instruction( uint32_t address, uint32_t code, std::string instruction, std::vector<std::string> parameters, std::string comment = std::string() );
            Instruction();
            ~Instruction();

            void set( uint32_t address, uint32_t code, std::string instruction, std::vector<std::string> parameters, std::string comment = std::string() );
            void set( const Instruction &o );

            std::string toString() const;

            uint32_t getAddress() const;
            uint32_t getCode() const;
            std::string getInstruction() const;
            std::vector<std::string> getParameters() const;
            std::string getComment() const;

         private:
            uint32_t m_Address;
            uint32_t m_Code;
            std::string m_Instruction;
            util::strvec m_Parameters;
            std::string m_Comment;
      };
      RISCV( MemoryInterface *pMem );
      ~RISCV();

      void step( Instruction *pInstruction = 0 );

      void reset();

      static std::string registerName( int n );
      uint32_t getRegister( int r ) const;
      uint32_t getPC() const;

   private:
      void unknownOpcode();
      void reserveAddr( uint32_t addr, int n );
      void invalidateReservation( uint32_t addr, int n = 1 );
      void setRegister( int r, uint32_t v );
      uint8_t readMem8( uint32_t address );
      uint16_t readMem16( uint32_t address );
      uint32_t readMem32( uint32_t address );
      void writeMem8( uint32_t address, uint8_t d );
      void writeMem16( uint32_t address, uint16_t d );
      void writeMem32( uint32_t address, uint32_t d );
      int numReservedAddresses( uint32_t addr, int n = 1 ) const;
      void clearAllReservations();

      MemoryInterface *m_pMemory;
      uint32_t m_Registers[32];
      uint32_t m_PC;
      std::map<uint32_t, bool> m_ReservedAddresses;
};

#endif
