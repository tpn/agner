/**************************  MixedPool.cpp   **********************************
* Author:        Agner Fog
* Date created:  2008-06-12
* Last modified: 2008-06-12
* Description:
* Defines memory pool for storing data of mixed type and size.
*
* The latest version of this file is available at:
* www.agner.org/optimize/cppexamples.zip
* (c) 2008 GNU General Public License www.gnu.org/copyleft/gpl.html
*******************************************************************************
*
* MixedPool is a container class defining a memory pool that can contain
* objects of any type and size. It is possible to store objects of different
* types in the same MixedPool.
*
* MixedPool is useful for storing objects of any type and size in a memory
* buffer of dynamic size. This is often more efficient that allocating a
* separate memory buffer for each object.
*
* MixedPool is also useful for reading and writing binary files containing
* mixed data structures.
*
* The objects stored can be of any type that do not require a constructor
* or destructor. The size of the memory pool can grow, but not shrink.
* Objects cannot be removed randomly. Objects can only be removed by 
* calling SetDataSize, which removes all subsequently stored objects as well.
* While this restriction can cause some memory space to be wasted, it has
* the advantage that no time-consuming garbage collection is needed.
*
* MixedPool is not thread safe if shared between threads. If your program 
* needs storage in multiple threads then each thread must have its own 
* private instance of MixedPool, or you must prevent other threads from
* accessing MixedPool while you are adding objects.
*
* Note that you should never store a pointer to an object in the memory
* pool because the pointer will become invalid in case the subsequent 
* addition of another object causes the memory to be re-allocated.
* All objects in the pool should be identified by the offset returned
* by the Push function, not by pointers.
*
* Attempts to read an object with an offset beyond the current DataSize
* will cause an error message to the standard error output. You may change
* the MixedPool::Error function to produce a message box if the program has
* a graphical user interface.
*
* It is possible, but not necessary, to allocate a certain amount of 
* memory before adding any objects. This can reduce the risk of having
* to re-allocate memory if the first allocated memory block turns out
* to be too small. Use the function ReserveSize to set the desired size
* of memory if it is possible to make a reasonable estimate of how much
* memory will be needed.
*
* MixedPool does not remember the type, size and offset of each object
* stored. It is the responsibility of the programmer to make sure that 
* the correct offset and type is specified when retrieving a previously 
* stored object from MixedPool.
*
* At the end of this file you will find a working example of how to use 
* MixedPool.
*
* The first part of this file containing declarations may be placed in a 
* header file. The second part containing function definitions should be in
* a .cpp file. The third part containing examples should be removed from your
* final application.
*
******************************************************************************/

/******************************************************************************
Header part. Put this in a .h file:
******************************************************************************/

#include <memory.h>                              // For memcpy and memset
#include <string.h>                              // For strlen
#include <stdlib.h>                              // For exit in Error function
#include <stdio.h>                               // Needed for example only


// Class MixedPool makes a dynamic array which can grow as new data are
// added. Data can be of mixed type and size
class MixedPool {
public:
   MixedPool();                                  // Constructor
   ~MixedPool();                                 // Destructor
   void ReserveSize(int size);                   // Allocate buffer of specified size
   void SetDataSize(int size);                   // Set the size of the data area that is considered used
   int GetDataSize()  {return DataSize;};        // Size of data stored so far
   int GetBufferSize(){return BufferSize;};      // Size of allocated buffer
   int GetNumEntries(){return NumEntries;};      // Get number of items pushed
   int Push(void const * obj, int size);         // Add object to buffer, return offset
   template<typename TX> int Push(TX const & x){ // Same, size detected automatically
      return Push(&x, sizeof(TX));}
   int PushString(char * s) {                    // Add zero-terminated string to buffer, return offset
      return Push(s, strlen(s) + 1);}
   void Align(int a);                            // Align next entry to address divisible by a (relative to buffer start)
   char * Buf() const {return buffer;};          // Access to buffer
   template <typename TX> TX & Get(int Offset) { // Get object of arbitrary type at specified offset in buffer
      if (Offset >= DataSize) {Error(1,Offset); Offset = 0;} // Offset out of range
      return *(TX*)(buffer + Offset);}
   // Define desired allocation size
   enum DefineSize {
      AllocateSpace  = 1024};                    // Minimum size of allocated memory block
private:
   char * buffer;                                // Buffer containing binary data. To be modified only by SetSize
   int BufferSize;                               // Size of allocated buffer ( > DataSize)
   MixedPool(MixedPool const&){};                // Make private copy constructor to prevent copying
   void operator = (MixedPool const&){};         // Make private assignment operator to prevent copying
protected:
   int NumEntries;                               // Number of objects pushed
   int DataSize;                                 // Size of data, offset to vacant space
   void Error(int e, int n);                     // Make fatal error message
};



/******************************************************************************
Function definition part. Put this in a .cpp file:
******************************************************************************/

// Members of class MixedPool
MixedPool::MixedPool() {  
   // Constructor
   buffer = 0;
   NumEntries = DataSize = BufferSize = 0;
}

MixedPool::~MixedPool() {
   // Destructor
   ReserveSize(0);                     // De-allocate buffer
}

void MixedPool::ReserveSize(int size) {
   // Allocate buffer of at least the specified size
   // Setting size > current BufferSize will allocate a larger buffer and 
   // move all data to the new buffer.
   // Setting size <= current BufferSize will do nothing. The buffer will 
   // only grow, not shrink.
   // Setting size = 0 will discard all data and de-allocate the buffer.
   if (size <= BufferSize) {
      if (size <= 0) {
         if (size < 0) Error(1, size);
         // size = 0. Discard data and de-allocate buffer
         if (buffer) delete[] buffer;  // De-allocate buffer
         buffer = 0;
         NumEntries = DataSize = BufferSize = 0;
         return;
      }
      // Request to reduce size. Ignore
      return;
   }
   size = (size + 15) & (-16);         // Round up size to value divisible by 16
   char * buffer2 = 0;                 // New buffer
   buffer2 = new char[size];           // Allocate new buffer
   if (buffer2 == 0) {Error(3,size); return;} // Error can't allocate
   memset (buffer2, 0, size);          // Initialize to all zeroes
   if (buffer) {
      // A smaller buffer is previously allocated
      memcpy (buffer2, buffer, BufferSize); // Copy contents of old buffer into new one
      delete[] buffer;                 // De-allocate old buffer
   }
   buffer = buffer2;                   // Save pointer to buffer
   BufferSize = size;                  // Save size
}

void MixedPool::SetDataSize(int size) {
   // Set the size of the data area that is considered used and valid.
   // DataSize is initially zero. It is increased by Push.
   // Setting size > DataSize is equivalent to pushing zeroes until 
   // DataSize = size.
   // Setting size < DataSize will decrease DataSize so that all data 
   // with offset >= size is erased.
   // NumEntries is not changed by SetDataSize(). Calls to GetNumEntries()
   // will be meaningless after calling SetDataSize().

   if (size < 0) Error(1, size);
   if (size > BufferSize) {
      // Allocate larger buffer. Add AllocateSpace and round up to divisible by 16
      ReserveSize((size + AllocateSpace + 15) & (-16));
   }
   else if (size < DataSize) {
      // Request to delete some data. Overwrite with zeroes
      memset(buffer + size, 0, DataSize - size);
   }
   // Set new DataSize
   DataSize = size;
}

   
int MixedPool::Push(void const * obj, int size) {
   // Add object to buffer, return offset
   // Parameters: 
   // obj = pointer to object, 0 if fill with zeroes
   // size = size of object to push

   // Old offset will be offset to new object
   int OldOffset = DataSize;

   // New data size will be old data size plus size of new object
   int NewOffset = DataSize + size;

   if (NewOffset > BufferSize) {
      // Buffer too small, allocate more space.
      // We can use SetSize for this only if it is certain that obj is not 
      // pointing to an object previously allocated in the old buffer
      // because it would be deallocated before copied into the new buffer.

      // Allocate more space without using SetSize:
      // Double the size + AllocateSpace, and round up size to value divisible by 16
      int NewSize = (NewOffset * 2 + AllocateSpace + 15) & (-16);
      char * buffer2 = 0;              // New buffer
      buffer2 = new char[NewSize];     // Allocate new buffer
      if (buffer2 == 0) {
         Error(3, NewSize); return 0;  // Error can't allocate
      }
      // Initialize to all zeroes
      memset (buffer2, 0, NewSize);
      if (buffer) {
         // A smaller buffer is previously allocated
         // Copy contents of old buffer into new
         memcpy (buffer2, buffer, BufferSize);
      }
      BufferSize = NewSize;                      // Save size
      if (obj && size) {                         
         // Copy object to new buffer
         memcpy (buffer2 + OldOffset, obj, size);
         obj = 0;                                // Prevent copying once more
      }
      // Delete old buffer after copying object
      if (buffer) delete[] buffer;

      // Save pointer to new buffer
      buffer = buffer2;
   }
   // Copy object to buffer if nonzero
   if (obj && size) {
      memcpy (buffer + OldOffset, obj, size);
   }
   if (size) {
      // Adjust new offset
      DataSize = NewOffset;
      NumEntries++;
   }
   // Return offset to allocated object
   return OldOffset;
}


void MixedPool::Align(int a) {
   // Align next entry to address divisible by a, relative to buffer start.
   // If a is not sure to be a power of 2:
   int NewOffset = (DataSize + a - 1) / a * a;
   // If a is sure to be a power of 2:
   // int NewOffset = (DataSize + a - 1) & (-a);
   if (NewOffset > BufferSize) {
      // Allocate more space
      ReserveSize (NewOffset * 2 + AllocateSpace);
   }
   // Set DataSize to after alignment space
   DataSize = NewOffset;
}


// Produce fatal error message. Used internally and by StringElement.
// Note: If your program has a graphical user interface (GUI) then you
// must rewrite this function to produce a message box with the error message.
void MixedPool::Error(int e, int n) {
   // Define error texts
   static const char * ErrorTexts[] = {
      "Unknown error",                 // 0
      "Offset out of range",           // 1
      "Size out of range",             // 2
      "Memory allocation failed"       // 3
   };
   // Number of texts in ErrorTexts
   const unsigned int NumErrorTexts = sizeof(ErrorTexts) / sizeof(*ErrorTexts);

   // check that index is within range
   if ((unsigned int)e >= NumErrorTexts) e = 0;

   // Replace this with your own error routine, possibly with a message box:
   fprintf(stderr, "\nMixedPool error: %s (%i)\n", ErrorTexts[e], n);

   // Terminate execution
   exit(1);
}


/******************************************************************************
Example part. Remove this from final application:
******************************************************************************/

// Here follows a working example of how to use MixedPool. 
// To run this example, just compile this file for console mode and run it.
// You may play with this example as you like

int main() {
   // Make instance of MixedPool
   MixedPool pool;

   // Set the buffer size if you can make a reasonable estimate of how
   // much memory will be needed. Leave this out if you have no guess:
   pool.ReserveSize(100);

   // Define data to put into pool
   int    a = 0;
   double b = 2.345678;
   char * c = "Hello";

   // Put data into pool
   int ia = pool.Push(a);                      // a stored at offset ia
   int ib = pool.Push(b);                      // b stored at offset ib
   int ic = pool.PushString(c);                // c stored at offset ic

   // Modify data in pool
   pool.Get<int>(ia) = 1;                      // Change value of int stored at offset ia

   // Retrieve data from pool
   printf("\na = %i",  pool.Get<int>(ia));     // Read int from offset ia
   printf("\nb = %f",  pool.Get<double>(ib));  // Read double from offset ib
   printf("\nc = %s", &pool.Get<char>(ic));    // Read string starting at offset ic

   // Get number of entries
   printf("\nNumber of items: %i\n", pool.GetNumEntries());

   return 0;
}
