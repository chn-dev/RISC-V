/*******************************************************************************
 *  Copyright (c) 2024 Christian Nowak <chnowak@web.de>                        *
 *   This file is part of chn's RISC-V-Emulator.                               *
 *                                                                             *
 *  RISC-V-Emulator is free software: you can redistribute it and/or modify it *
 *  under the terms of the GNU General Public License as published by the Free *
 *  Software Foundation, either version 3 of the License, or (at your option)  *
 *  any later version.                                                         *
 *                                                                             *          
 *  RISC-V-Emulator is distributed in the hope that it will be useful, but     * 
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY *
 *  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License    *
 *  for more details.                                                          *
 *                                                                             *
 *  You should have received a copy of the GNU General Public License along    *
 *  with RISC-V-Emulator. If not, see <https://www.gnu.org/licenses/>.         *
 *******************************************************************************/


/*----------------------------------------------------------------------------*/
/*!
\file Emulator.h
\author Christian Nowak <chnowak@web.de>
\brief Headerfile for class Emulator.
*/
/*----------------------------------------------------------------------------*/
#ifndef __EMULATOR_H__
#define __EMULATOR_H__

#include <string>

#include "RISCV.h"

class Emulator : public RISCV::MemoryInterface
{
   public:
      static Emulator *create( std::string fileName );
      ~Emulator();

      void step();

      bool emulationStopped() const;

      virtual uint8_t readMem8( uint32_t address );
      virtual uint16_t readMem16( uint32_t address );
      virtual uint32_t readMem32( uint32_t address );
      virtual void writeMem8( uint32_t address, uint8_t d );
      virtual void writeMem16( uint32_t address, uint16_t d );
      virtual void writeMem32( uint32_t address, uint32_t d );
      virtual void unknownOpcode();

   private:
      Emulator( uint8_t *pProgramData, size_t programDataSize );

      RISCV *m_pCPU;
      uint8_t *m_pProgramData;
      size_t m_ProgramDataSize;
      uint32_t m_RAMStart;
      uint32_t m_RAMSize;
      uint8_t *m_pRAM;
      bool m_StopEmulation;
};

#endif
