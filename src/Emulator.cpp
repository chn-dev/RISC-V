#include <iostream>
#include <fstream>
#include <string.h>

#include "Emulator.h"

Emulator::Emulator( uint8_t *pProgramData, size_t programDataSize ) :
   m_pProgramData( pProgramData ),
   m_ProgramDataSize( programDataSize ),
   m_RAMStart( 0x80000000 ),
   m_RAMSize(  0x08000000 )
{
   m_pCPU = new RISCV( this );
   m_pRAM = new uint8_t[m_RAMSize];
   memcpy( m_pRAM, pProgramData, programDataSize > m_RAMSize ? m_RAMSize : programDataSize );
}


Emulator::~Emulator()
{
   delete m_pProgramData;
   delete m_pCPU;
   delete m_pRAM;
}


void Emulator::step()
{
   m_pCPU->step();
}


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


uint16_t Emulator::readMem16( uint32_t address )
{
   return(   ( readMem8( address ) & 0xff ) |
           ( ( readMem8( address + 1 ) & 0xff ) << 8 ) );
}


uint32_t Emulator::readMem32( uint32_t address )
{
   return(   ( (uint32_t)readMem8( address )     & 0xff ) |
           ( ( (uint32_t)readMem8( address + 1 ) & 0xff ) << 8 ) |
           ( ( (uint32_t)readMem8( address + 2 ) & 0xff ) << 16 ) |
           ( ( (uint32_t)readMem8( address + 3 ) & 0xff ) << 24 ) );
}


void Emulator::writeMem8( uint32_t address, uint8_t d )
{
   if( address >= m_RAMStart && address < m_RAMStart + m_RAMSize )
   {
      uint32_t a = address - m_RAMStart;
      m_pRAM[a] = d;
   }
}


void Emulator::writeMem16( uint32_t address, uint16_t d )
{
   writeMem8( address,       d        & 0xff );
   writeMem8( address + 1, ( d >> 8 ) & 0xff );
}


void Emulator::writeMem32( uint32_t address, uint32_t d )
{
   writeMem8( address,       d         & 0xff );
   writeMem8( address + 1, ( d >> 8  ) & 0xff );
   writeMem8( address + 2, ( d >> 16 ) & 0xff );
   writeMem8( address + 3, ( d >> 24 ) & 0xff );
}
