/*----------------------------------------------------------------------------*/
/*!
\file Emulator.cpp
\author Christian Nowak <chnowak@web.de>
\brief This implements the Emulator
*/
/*----------------------------------------------------------------------------*/
#include <iostream>
#include <fstream>
#include <string.h>

#include "Emulator.h"


/*----------------------------------------------------------------------------*/
/*! 2024-08-15
Constructor for class Emulator
\param pProgramData Pointer to the machine code to be executed
\param programDataSize Size in bytes of the machine code
*/
/*----------------------------------------------------------------------------*/
Emulator::Emulator( uint8_t *pProgramData, size_t programDataSize ) :
   m_pProgramData( pProgramData ),
   m_ProgramDataSize( programDataSize ),
   m_RAMStart( 0x80000000 ),
   m_RAMSize(  0x08000000 ),
   m_StopEmulation( false )
{
   m_pCPU = new RISCV( this );
   m_pRAM = new uint8_t[m_RAMSize];
   memcpy( m_pRAM, pProgramData, programDataSize > m_RAMSize ? m_RAMSize : programDataSize );
}


/*----------------------------------------------------------------------------*/
/*! 2024-08-15
Destructor for class RISCV
*/
/*----------------------------------------------------------------------------*/
Emulator::~Emulator()
{
   delete m_pProgramData;
   delete m_pCPU;
   delete m_pRAM;
}


/*----------------------------------------------------------------------------*/
/*! 2024-08-15
Execute a single machine code instruction
*/
/*----------------------------------------------------------------------------*/
void Emulator::step()
{
   m_pCPU->step();
}


/*----------------------------------------------------------------------------*/
/*! 2024-08-15
Create an instance of the Emulator and load a binary file into its virtual
memory for execution.
\param fileName The file to be loaded as machine code
\return A pointer to the new Emulator instance or nullptr on failure
*/
/*----------------------------------------------------------------------------*/
Emulator *Emulator::create( std::string fileName )
{
   std::ifstream binFile( fileName, std::ios::binary );
   if( !binFile )
      return( 0 );

   binFile.seekg( 0, std::ios::end );
   size_t binSize = binFile.tellg();
   binFile.seekg( 0, std::ios::beg );

   uint8_t *pBinData = new uint8_t[binSize];
   binFile.read( (char*)pBinData, binSize );
   binFile.close();

   Emulator *pEmu = new Emulator( pBinData, binSize );

   return( pEmu );
}


/*----------------------------------------------------------------------------*/
/*! 2024-08-15
This method is invoked when the CPU reads a byte.
\param address The memory address to read from
\return The value
*/
/*----------------------------------------------------------------------------*/
uint8_t Emulator::readMem8( uint32_t address )
{
   if( address >= m_RAMStart && address < m_RAMStart + m_RAMSize )
   {
      uint32_t a = address - m_RAMStart;
      uint8_t d = m_pRAM[a];
      return( m_pRAM[a] );
   } else
   {
      return( 0xff );
   }
}


/*----------------------------------------------------------------------------*/
/*! 2024-08-15
This method is invoked when the CPU reads a half-word.
\param address The memory address to read from
\return The value
*/
/*----------------------------------------------------------------------------*/
uint16_t Emulator::readMem16( uint32_t address )
{
   return(   ( readMem8( address ) & 0xff ) |
           ( ( readMem8( address + 1 ) & 0xff ) << 8 ) );
}


/*----------------------------------------------------------------------------*/
/*! 2024-08-15
This method is invoked when the CPU reads a word.
\param address The memory address to read from
\return The value
*/
/*----------------------------------------------------------------------------*/
uint32_t Emulator::readMem32( uint32_t address )
{
   return(   ( (uint32_t)readMem8( address )     & 0xff ) |
           ( ( (uint32_t)readMem8( address + 1 ) & 0xff ) << 8 ) |
           ( ( (uint32_t)readMem8( address + 2 ) & 0xff ) << 16 ) |
           ( ( (uint32_t)readMem8( address + 3 ) & 0xff ) << 24 ) );
}


/*----------------------------------------------------------------------------*/
/*! 2024-08-15
This method is invoked when the CPU writes a byte.
\param address The memory address to write to
\param d The value to write
*/
/*----------------------------------------------------------------------------*/
void Emulator::writeMem8( uint32_t address, uint8_t d )
{
   if( address >= m_RAMStart && address < m_RAMStart + m_RAMSize )
   {
      // RAM access
      uint32_t a = address - m_RAMStart;
      m_pRAM[a] = d;
   } else
   {
      // When writing to address 0x0, output the byte to the console
      if( address == 0 )
         printf( "%c", d );
   }
}


/*----------------------------------------------------------------------------*/
/*! 2024-08-15
This method is invoked when the CPU writes a half-word.
\param address The memory address to write to
\param d The value to write
*/
/*----------------------------------------------------------------------------*/
void Emulator::writeMem16( uint32_t address, uint16_t d )
{
   writeMem8( address,       d        & 0xff );
   writeMem8( address + 1, ( d >> 8 ) & 0xff );
}


/*----------------------------------------------------------------------------*/
/*! 2024-08-15
This method is invoked when the CPU writes a word.
\param address The memory address to write to
\param d The value to write
*/
/*----------------------------------------------------------------------------*/
void Emulator::writeMem32( uint32_t address, uint32_t d )
{
   writeMem8( address,       d         & 0xff );
   writeMem8( address + 1, ( d >> 8  ) & 0xff );
   writeMem8( address + 2, ( d >> 16 ) & 0xff );
   writeMem8( address + 3, ( d >> 24 ) & 0xff );
}


/*----------------------------------------------------------------------------*/
/*! 2024-08-15
This method is invoked when the CPU encounters an unknown/illegal optode.
*/
/*----------------------------------------------------------------------------*/
void Emulator::unknownOpcode()
{
   m_StopEmulation = true;
}


/*----------------------------------------------------------------------------*/
/*! 2024-08-15
\return true if the emulation has been stopped, i.e. the CPU has encountered
an illegal opcode.
*/
/*----------------------------------------------------------------------------*/
bool Emulator::emulationStopped() const
{
   return( m_StopEmulation );
}
