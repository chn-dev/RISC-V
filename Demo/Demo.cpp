void numberToString( int number, char *pStr )
{
   if( number == 0 )
   {
      pStr[0] = '0';
      pStr[1] = '\0';
   } else
   {
      bool isNegative = number < 0;
      if( isNegative )
      {
         pStr[0] = '-';
         number = -number;
      }

      int iTail = isNegative ? 1 : 0;
      while( number > 0 )
      {
         int r = number % 10;
         pStr[iTail] = '0' + r;
         pStr[iTail + 1] = '\0';
         iTail++;
         number /= 10;
      }
   
      iTail--;
   
      for( int iFront = isNegative ? 1 : 0; iFront < iTail; iFront++, iTail-- )
      {
         char tmp = pStr[iFront];
         pStr[iFront] = pStr[iTail];
         pStr[iTail] = tmp;
      }
   }
}


void printString( const char *pStr )
{
	for( int i = 0; pStr[i]; i++ )
	{
	   *( (unsigned char *)0 ) = pStr[i];
	}
}


int main( int argc, const char *argv[] )
{
	const char *pMsg = "Hello, world!\nI'm running from within RISC-V-Emulator.\n\n";
   const char *pNL = "\n";

	printString( pMsg );

   for( int i = -10; i <= 10; i++ )
   {
      char tmp[256];

      numberToString( i, tmp );
      printString( tmp );
      printString( pNL );
   }

	return( 0 );
}
