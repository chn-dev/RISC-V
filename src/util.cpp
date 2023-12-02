#include "util.h"

std::string util::join( const util::strvec &v, const std::string &sep )
{
   std::string r;
   for( int i = 0; i < v.size(); i++ )
   {
      r = r + v[i];
      if( i != v.size() - 1 )
         r += sep;
   }

   return( r );
}
