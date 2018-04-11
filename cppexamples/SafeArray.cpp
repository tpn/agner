/***************************  SafeArray.cpp   *********************************
* Author:        Agner Fog
* Date created:  2008-06-12
* Last modified: 2008-06-12
* Description:
* Template class for array with bounds checking
*
* The latest version of this file is available at:
* www.agner.org/optimize/cppexamples.zip
* (c) 2008 GNU General Public License www.gnu.org/copyleft/gpl.html
*******************************************************************************
*
* SafeArray defines an array with bounds checking.
* 
* The size is defined at compile time. The elements in the array can be of
* any type that do not require a constructor or destructor.
*
* An example of how to use SafeArray is provided at the end of this file.
*
******************************************************************************/

/******************************************************************************
Header part. Put this in a .h file:
******************************************************************************/

#include <memory.h>                              // For memcpy and memset
#include <stdio.h>                               // Needed for example only


// Template for safe array with bounds checking
template <typename T, unsigned int N> class SafeArray {
protected:
   T a[N];                             // Array with N elements of type T
public:
   // Constructor
   SafeArray() {
      memset(a, 0, sizeof(a));         // Initialize array to zero
   }
   // Return the size of the array
   int Size() const {                 
      return N;
   }
   // Safe [] array index operator
   T & operator[] (unsigned int i) {
      if (i >= N) {
         // Index out of range. The next line provokes an error.
         // You may insert any other error reporting here:
         return *(T*)0;  // Return a null reference to provoke error
      }
      // No error
      return a[i];     // Return reference to a[i]
   }
};


/******************************************************************************
Example part. Remove this from final application:
******************************************************************************/

int main() {

   // Declare a safe array of 20 float's
   SafeArray<float, 10> list;

   // Output all elements
   for (int i = 0; i < list.Size(); i++) {
      printf("\n%8.3f", list[i]);
   }   

   return 0;
}
