/*
 * RTLinux user-space FPU test example
 *
 * Written by Michael Barabanov, 1998
 *  (C) FSMLabs  1999. baraban@fsmlabs.com
 *  Released under the GNU GENERAL PUBLIC LICENSE Version 2, June 1991
 */

#include <math.h>
#include <stdio.h>
#include <unistd.h>

int main( int argc, char **argv )
{
	int i = 0;
	double f = 0;

	nice(20);
	while (1) {
		i++;
		f++;
		if (i != f) {
			printf("FP error: i = %d, f = %f\n", i, f);
			i = 0;
			f = 0;
		}
		if (i > 200000) {
			i = 0;
			f = 0;
		}
	}
}
