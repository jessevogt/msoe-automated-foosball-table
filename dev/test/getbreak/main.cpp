#include <iostream>

void GetBreaks(unsigned char value, int *buffer)
{
	int index = 0;

	for(unsigned int t=0; t<8; ++t)
	{
		if((value & 0x80) == 0)
		{
			buffer[index] = t;
			++index;
		}

		value = value << 1;
	}
}

int main(int argc, char** argv)
{
	int buffer[8];

	for(int t=0; t<8; ++t)
		buffer[t] = -1;

	GetBreaks(0xF7,buffer);

	for(int t=0; t<8; ++t)
		std::cout << buffer[t] << " ";

	std::cout << std::endl;
	
	return 0;
}
