/*************************  AlignedArray.cpp  *********************************
* Author:        Agner Fog
* Date created:  2008-06-12
* Last modified: 2008-06-12
* Description:
* Defines linear array of dynamic size.
* First entry is aligned at address divisible by 16
*
* The latest version of this file is available at:
* www.agner.org/optimize/cppexamples.zip
* (c) 2008 GNU General Public License www.gnu.org/copyleft/gpl.html
*******************************************************************************
*
* AlignedArray is a container class defining a dynamic array or memory pool 
* that can contain any number of objects of the same type. The beginning of
* the array is aligned at a memory address divisible by 16. This is useful
* when using the intrinsic vector types __m128, __m128d, __m128i.
*
* Array elements can be accessed one by one, or grouped together into vectors.
*
* The objects stored can be of any type that does not require a constructor
* or destructor. The size of the memory pool can grow, but not shrink.
* Objects cannot be removed randomly. Objects can only be removed by 
* calling SetNum, which removes all subsequently stored objects as well.
* While this restriction can cause some memory space to be wasted, it has
* the advantage that no time-consuming garbage collection is needed.
*
* AlignedArray is not thread safe if shared between threads. If your program 
* needs storage in multiple threads then each thread must have its own 
* private instance of AlignedArray, or you must prevent other threads from
* accessing AlignedArray while you are changing the size with Reserve(n)
* or SetNum(n).
*
* Note that you should not store a pointer to an object in AlignedArray
* if the size is modified with Reserve(n) or SetNum(n) because the pointer 
* will become invalid if memory is re-allocated.
*
* Attempts to access an object with an invalid index or access an unaligned
* vector will cause an error message to the standard error output. 
* You may change the AlignedArray::Error function to produce a message box 
* if the program has a graphical user interface.
*
* At the end of this file you will find a working example of how to use 
* AlignedArray.
*
* The first part of this file containing declarations may be placed in a 
* header file. The second part containing examples should be removed from the
* final application.
*
******************************************************************************/

/******************************************************************************
Header part. Put this in a .h file:
******************************************************************************/

#include <memory.h>                              // For memcpy and memset
#include <stdlib.h>                              // For exit in Error function
#include <stdio.h>                               // Needed for example only
#include <xmmintrin.h>                           // Needed for example only

#define BOUNDSCHECKING 1                         // 0 will skip array bounds checking

// Class AlignedArray makes an aligned dynamic array
template <typename TX>
class AlignedArray {
public:
   // Constructor
   AlignedArray();
   // Destructor
   ~AlignedArray();
   // Set the number of valid entries. New entries will be zero
   void SetNum(int num);
   // Allocate buffer for future growth to num objects
   void Reserve(int num);
   // Get number of objects in array
   int GetNum(){return NumEntries;};
   // Get number of objects that can be stored without re-allocating memory
   int GetMaxNum(){return MaxNum;};
   // Access object with index i
   TX & operator[] (int i) {
#if BOUNDSCHECKING
      if ((unsigned int)i >= (unsigned int)NumEntries) {Error(1, i); i = 0;} // Index i out of range
#endif
      return bufferA[i];}
   template <typename VEC>
   // Access vector starting at index i
   VEC & Vect(int i) {
      const int ElementsPerVector = sizeof(VEC) / sizeof(TX);// Number of elements per vector
#if BOUNDSCHECKING
      if (i & (ElementsPerVector-1)) {Error(2, i); i = 0;} // Index i not divisible by vector size
      if ((unsigned int)(ElementsPerVector - 1 + i) >= (unsigned int)NumEntries) {Error(1, i);  i = 0;}  // Index i out of range
#endif
      return *(VEC*)(bufferA + i);
   }
   // Define desired alignment. Must be a power of 2:
   enum DefineSize {
      AlignBy = 16                               // Desired alignment, must be a power of 2
   };
private:
   char * bufferU;                               // Unaligned data buffer
   TX * bufferA;                                 // Aligned pointer to data buffer
   int MaxNum;                                   // Maximum number of objects that buffer can contain
   AlignedArray(AlignedArray const&){};          // Make private copy constructor to prevent copying
   void operator = (AlignedArray const&){};      // Make private assignment operator to prevent copying
protected:
   int NumEntries;                               // Number of objects stored
   void Error(int e, int n);                     // Make fatal error message
};


// Members of class AlignedArray
template <typename TX>
AlignedArray<TX>::AlignedArray() {  
   // Constructor
   bufferU = 0;  bufferA = 0;
   MaxNum = NumEntries = 0;
}


template <typename TX>
AlignedArray<TX>::~AlignedArray() {
   // Destructor
   Reserve(0);                                   // De-allocate buffer
}


template <typename TX>
void AlignedArray<TX>::Reserve(int num) {
   // Allocate buffer for future growth to num objects.
   // Use this if it can be predicted that the size will be increased 
   // later with SetNum(num). This will minimize the number of
   // memory re-allocations.
   //
   // Setting num > current MaxNum will allocate a larger buffer and 
   // move all data to the new buffer.
   //
   // Setting num <= current MaxNum will do nothing. The buffer will 
   // only grow, not shrink.
   //
   // Setting num = 0 will discard all data and de-allocate the buffer.
   if (num <= MaxNum) {
      if (num <= 0) {
         if (num < 0) Error(1, num);
         // num = 0. Discard data and de-allocate buffer
         if (bufferU) delete[] bufferU;          // De-allocate buffer
         bufferU = 0;  bufferA = 0;
         MaxNum = NumEntries = 0;
         return;
      }
      // Request to reduce size. Ignore
      return;
   }
   // num > MaxNum. Allocate new buffer
   char * buffer2U = 0;                          // New buffer, unaligned
   // Aligned pointer to new buffer:
   union {
      char * b;                                  // Used for converting from char*
      TX * p;                                    // Converted to TX *
      long int i;                                // Used for alignment
   } buffer2A;
   // Note: On big-endian platforms buffer2A.i must have the same size as a pointer,
   // on little-endian platforms it doesn't matter.
   buffer2U = new char[num*sizeof(TX)+AlignBy-1];// Allocate new buffer
   if (buffer2U == 0) {Error(3,num); return;}    // Error can't allocate
   // Align new buffer by AlignBy (must be a power of 2)
   buffer2A.b = buffer2U + AlignBy - 1;
   buffer2A.i &= - (long int)AlignBy;

   if (bufferA) {
      // A smaller buffer is previously allocated
      memcpy(buffer2A.p, bufferA, NumEntries*sizeof(TX));// Copy contents of old buffer into new one
      delete[] bufferU;                          // De-allocate old buffer
   }
   bufferU = buffer2U;                           // Save pointer to buffer
   bufferA = buffer2A.p;                         // Save aligned pointer to new buffer
   MaxNum = num;                                 // Save new size
}


template <typename TX>
void AlignedArray<TX>::SetNum(int num) {
   // Set the number of objects that are considered used and valid.
   // NumEntries is initially zero. It is increased by Push or SetNum
   // Setting num > NumEntries is equivalent to pushing (num - NumEntries)
   // objects with zero contents.
   // Setting num < NumEntries will decrease NumEntries so that all objects 
   // with index >= num are erased.
   // Setting num = 0 will erase all objects, but not de-allocate the buffer.

   if (num < 0) { // Cannot be negative
      Error(1, num); return;
   }
   if (num > MaxNum) {
      // Allocate larger buffer. 
      Reserve(num);
   }
   if (num > NumEntries) {
      // Fill new entries with zero
      memset(bufferA + NumEntries, 0, (num - NumEntries) * sizeof(TX));
   }
   // Set new DataSize
   NumEntries = num;
}

   
// Produce fatal error message. Used internally and by StringElement.
// Note: If your program has a graphical user interface (GUI) then you
// must rewrite this function to produce a message box with the error message.
template <typename TX>
void AlignedArray<TX>::Error(int e, int n) {
   // Define error texts
   static const char * ErrorTexts[] = {
      "Unknown error",                           // 0
      "Index out of range",                      // 1
      "Index not divisible by vector size",      // 2
      "Memory allocation failed"                 // 3
   };
   // Number of texts in ErrorTexts
   const unsigned int NumErrorTexts = sizeof(ErrorTexts) / sizeof(*ErrorTexts);

   // check that index is within range
   if ((unsigned int)e >= NumErrorTexts) e = 0;

   // Replace this with your own error routine, possibly with a message box:
   fprintf(stderr, "\nAlignedArray error: %s (%i)\n", ErrorTexts[e], n);

   // Terminate execution
   exit(1);
}


/******************************************************************************
Example part. Remove this from final application:
******************************************************************************/

// Here follows a working example of how to use AlignedArray. 
// To run this example, just compile this file for console mode and run it.
// You may play with this example as you like

int main() {
   int i;                                        // Loop counter

   // Define a vector of 4 floats = (1,2,3,4)
   __m128 const f1234 = _mm_setr_ps(1.0f, 2.0f, 3.0f, 4.0f);

   // Make instance of AlignedArray containing objects of type float:
   AlignedArray<float> list;

   // Set the number of entries in the array. All are initialized to 0.
   // Make number divisible by the number of elements in a vector (4)
   list.SetNum(12);

   // Put data into the array by index
   list[0] = list[1] = list[2]  = list[3]  = 100.f;
   list[4] = list[5] = list[6]  = list[7]  = 200.f;
   list[8] = list[9] = list[10] = list[11] = 300.f;

   // Vectorized loop, step size corresponds to vectors of 4 floats
   for (i = 0; i < list.GetNum(); i += 4) {
      // list.Vect<__m128>(i) is the vector (list[i],list[i+1],list[i+2],list[i+3])
      // i must be divisible by the number of elements in a vector (4 in this case).
      // _mm_add_ps is an intrinsic function adding two vectors, defined in xmmintrin.h
      list.Vect<__m128>(i) = _mm_add_ps( list.Vect<__m128>(i), f1234 );
   }

   // Output all entries
   for (i = 0; i < list.GetNum(); i++) {
      printf("\n%2i:  %8.2f", i, list[i]);
   }

   return 0;
}
