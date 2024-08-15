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
