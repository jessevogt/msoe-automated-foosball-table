#include <stdio.h>

int main(int argc, char** argv)
{
	int index = 0;
	
	for(index = 0; index < 8; ++index)
	{
		printf("%d\n",(unsigned int)~(1 << index));
	}
	
	return 0;
}