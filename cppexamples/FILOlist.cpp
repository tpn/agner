/***************************  FILOlist.cpp   **********************************
* Author:        Agner Fog
* Date created:  2008-06-12
* Last modified: 2008-06-12
* Description:
* Template class for First-In-First-Out list
*
* The latest version of this file is available at:
* www.agner.org/optimize/cppexamples.zip
* (c) 2008 GNU General Public License www.gnu.org/copyleft/gpl.html
*******************************************************************************
*
* FILOlist defines a stack with First-In-Last-Out access
* 
* The maximum size is defined at compile time. The elements in the array can be of
* any type that do not require a constructor or destructor.
*
* An example of how to use FILOlist is provided at the end of this file.
*
******************************************************************************/

/******************************************************************************
Header part. Put this in a .h file:
******************************************************************************/

#include <memory.h>                    // For memcpy and memset
#include <stdlib.h>                    // For exit in Error function
#include <stdio.h>                     // Needed for example only


// Template for First-In-Last-Out list
template <typename OBJTYPE, int MAXSIZE>
class FILOlist {
protected:
   OBJTYPE * top;                      // Pointer to top of stack
   int n;                              // Number of objects in list
   OBJTYPE list[MAXSIZE];              // Data buffer
public:
   FILOlist() {                        // Constructor
      top = list;                      // Initialize
      n = 0;
   }
   bool Put(OBJTYPE const & x) {       // Put object into list
      if (n >= MAXSIZE) {
         return false;                 // Return false if list full
      }
      n++;                             // Increment count
      *(top++) = x;                    // Copy x to list
      return true;                     // Return true if success
   }
   OBJTYPE Get() {                     // Get object from list
      if (n <= 0) {
         // Error: list empty
         // Put an error message here or return an empty object !
         exit(1);
      }
      n--;                             // Decrement count
      top--;                           // Decrement pointer
      return *top;                     // Return object
   }
   int NumObjects() {                  // Tell number of objects in list
      return n;                        // Return number of objects
   }
};


/******************************************************************************
Example part. Remove this from final application:
******************************************************************************/

int main() {

   FILOlist <int,1000> List;              // Make list of max 1000 int
   List.Put(10);                       // Put 10 into the list
   List.Put(20);                       // Put 20 into the list
   List.Put(30);                       // Put 30 into the list
   while (List.NumObjects() > 0) {     // While list not empty
      printf("\n%3i ", List.Get());    // Get item from list and print
   }                                   // Will print "10 20 30 "

   return 0;
}
