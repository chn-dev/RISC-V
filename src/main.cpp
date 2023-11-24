#include "Emulator.h"

int main( int argc, const char *argv[] )
{
   if( argc < 2 )
      return( -1 );

   std::string fileName = argv[1];

   Emulator *pEmu = Emulator::create( fileName );
   if( !pEmu )
      return( -1 );

   while( 1 )
   {
      pEmu->step();
   }

   delete pEmu;

   return( 0 );
}
