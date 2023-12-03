int main( int argc, const char *argv[] )
{
	unsigned char *pDest = 0;

	for( int i = 0; i < 32; i++ )
	{
		*pDest = (unsigned char)( i << 3 );
		*(unsigned long int *)pDest = 0x12345678;
		pDest++;
	}
	
	return( 0 );
}

