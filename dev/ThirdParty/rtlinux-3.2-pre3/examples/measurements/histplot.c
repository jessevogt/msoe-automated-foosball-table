/* 
	program to take the min max output and create histgram 
	Author:  Phil Wilshire
        DAta:    May 1999
        Licence: GPL

*/
#include <stdio.h>
#include <stddef.h>

#define NUMBINS  400
#define BINDELTA 500
#define LOWBIN   0
#define LINESIZE 132

int usage()
{
   printf(" use : histplot infile [outfile]\n");
}

int main( int argc , char * argv[] )
{

   FILE * infile;
   FILE * outfile;
   int bins[NUMBINS];
   char line[LINESIZE];
   char junk1[LINESIZE];
   char junk2[LINESIZE];
   int minval;
   int maxval;
   int scount;
   int nsamps ;
   int nover ;
   int i;
   int ival;

   if ( argc < 2 ) {
      usage();
      exit(1);
   }
   infile = fopen(argv[1],"r");
   if (infile == NULL ) {
      printf(" unable to open input file [%s] \n",argv[1]);
      exit(1);
   }

   if ( argc > 2 ) {
      outfile = fopen(argv[2],"w");
      if (outfile == NULL ) {
         printf(" unable to open output file [%s] \n",argv[2]);
         exit(1);
      }
   }  else {
      outfile = stdout;
   }
   nsamps = 0;
   nover = 0;
   for ( i = 0 ; i < NUMBINS ; i++ ) {
      bins[i] = 0;
   }

   while ( fgets(line,LINESIZE,infile) != NULL ) {
      scount = sscanf(line,"%s %d,%s %d",junk1,&minval,junk2,&maxval);
      if ( scount < 4 ) {
         printf(" error in line <%s> scount = %d \n",line,scount);
         exit(1); 
      };

      maxval = (maxval-LOWBIN) / BINDELTA;
      if ( maxval < NUMBINS  ) {
         bins[maxval]++;
      } else {
         nover++;
      }
   }
   
   for ( i = 0 ; i < NUMBINS ; i++ ) {
      ival = i * BINDELTA + (LOWBIN);
      fprintf(outfile, "%5d %5d\n",ival,bins[i]); 
   }

   fclose(infile);
   fclose(outfile);  
   exit(0);

	
}
