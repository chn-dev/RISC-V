int main( int argc, const char *argv[] )
{
	unsigned char *pDest = 0;

	for( int i = 0; i < 32; i++ )
	{
		*pDest = (unsigned char)( i );
		pDest++;
	}
	
	return( 0 );
}

