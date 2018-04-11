//          testmemcpy.cpp                                  Agner Fog 2012-02-22

// Test file for memcpy functions
// Function name mversion must be defined on the command line

#include <stdio.h>
//#include <process.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>


extern "C" {
void * mversion (void * dest, const void * src, size_t count);  // selected version of memcpy
void * MEMCPYR (void * dest, const void * src, size_t count);   // simplest version of memcpy

//extern int IInstrSet;
// function prototypes for CPU specific function versions
//memcpyF memcpy386, memcpySSE2, memcpySSSE3, memcpyU;
//memcpyF memmove386, memmoveSSE2, memmoveSSSE3, memmoveU;
}

void error(const char * s, int a, int b, int c) {
   printf("\nError %s: %i %i %i\n", s, a, b, c);
   exit (1);
}

int main () {

   int ao, bo, os, len;
   int version;
   const int pagesize = 0x1000;  // 4 kbytes
   const int n = 16*pagesize;
   char a[n], b[n], c[n];

   printf("\nTest memcpy");

   int i, x;
   for (i=0, x=91; i<n; i++) {
      x += 23;
      a[i] = (char)x;
   }

   memset(b, -1, n);

   // Test memcpy for correctness
   // Loop through versions
//   for (version = 0; version < NUMFUNC; version++) 
   {
      for (len=0; len<500; len++) {
         for (ao = 0; ao <=16; ao++) {
            for (bo = 0; bo <=16; bo++) {
               memset(b, -1, len+64);
               mversion(b+bo, a+ao, len);
               if (bo && b[bo-1] != -1) error("A", ao, bo, len);
               if (b[bo+len] != -1) error("B", ao, bo, len);
               if (len==0) continue;
               if (b[bo] != a[ao]) error("C", ao, bo, len);
               if (b[bo+len-1] != a[ao+len-1]) error("D", ao, bo, len);
               if (memcmp(b+bo, a+ao, len)) error("E", ao, bo, len);
            }
         }
      }
      // check false memory dependence branches
      len = 300;
      memcpy(b, a, 3*pagesize);
      for (ao = pagesize-200; ao < pagesize+200; ao++) {
         for (bo = 3*pagesize; bo <=3*pagesize+16; bo++) {
            memset(b+bo-64, -1, len+128);
            mversion(b+bo, b+ao, len);
            if (b[bo-1] != -1) error("A1", ao, bo, len);
            if (b[bo+len] != -1) error("B1", ao, bo, len);
            if (memcmp(b+bo, b+ao, len)) error("E1", ao, bo, len);
         }
      }
      // check false memory dependence branches with overlap
      // src > dest and overlap: must copy forwards
      len = pagesize+1000;
      for (ao = 2*pagesize; ao <=2*pagesize+16; ao++) {
         for (bo = pagesize-200; bo < pagesize+200; bo++) {
            memcpy(b, a, 4*pagesize);
            memcpy(c, a, 4*pagesize);
            mversion(b+bo, b+ao, len);
            //  memcpy(c+bo, c+ao, len);  // MS and Gnu versions of memcpy are actually memmove
            MEMCPYR(c+bo, c+ao, len);            
            if (memcmp(b, c, 4*pagesize)) {
               error("E2", ao-pagesize, bo-2*pagesize, len);
            }
         }
      }
      // check false memory dependence branches with overlap
      // dest > src and overlap: undefined behavior
#if 1
      len = pagesize+1000;
      for (ao = pagesize-200; ao < pagesize+200; ao++) {
         for (bo = 2*pagesize; bo <=2*pagesize+16; bo++) {
            memcpy(b, a, 4*pagesize);
            memcpy(c, a, 4*pagesize);
            mversion(b+bo, b+ao, len);
            //memcpy(c+bo, c+ao, len);  // MS and Gnu versions of memcpy are actually memmove
            MEMCPYR(c+bo, c+ao, len);            
            if (memcmp(b, c, 4*pagesize)) {
               error("E3", ao-pagesize, bo-2*pagesize, len);
            }
         }
      }
#endif
   }

   printf("\nSuccess\n");
   
   return 0;
}
