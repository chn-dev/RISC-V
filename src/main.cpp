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


#include "Emulator.h"

int main( int argc, const char *argv[] )
{
   if( argc < 2 )
   {
      fprintf( stderr, "Usage: %s BINFILE\n", argv[0] );
      return( -1 );
   }

   std::string fileName = argv[1];

   Emulator *pEmu = Emulator::create( fileName );
   if( !pEmu )
   {
      fprintf( stderr, "Couldn't load binary file from %s\n", argv[1] );
      return( -1 );
   }

   while( 1 )
   {
      pEmu->step();
      if( pEmu->emulationStopped() )
      {
         break;
      }
   }

   delete pEmu;

   return( 0 );
}
