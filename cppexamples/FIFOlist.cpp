/***************************  FIFOlist.cpp   **********************************
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
* FIFOlist defines a circular array with First-In-First-Out access
* 
* The size is defined at compile time. The elements in the array can be of
* any type that do not require a constructor or destructor.
*
* An example of how to use FIFOlist is provided at the end of this file.
*
******************************************************************************/

/******************************************************************************
Header part. Put this in a .h file:
******************************************************************************/

#include <memory.h>                    // For memcpy and memset
#include <stdlib.h>                    // For exit in Error function
#include <stdio.h>                     // Needed for example only


// Template for First-In-First-Out list
template <typename OBJTYPE, int MAXSIZE>
class FIFOlist {
protected:
   OBJTYPE * head, * tail;             // Pointers to current head and tail
   int n;                              // Number of objects in list
   OBJTYPE list[MAXSIZE];              // Circular buffer
public:
   FIFOlist() {                        // Constructor
      head = tail = list;              // Initialize
      n = 0;
   }
   bool Put(OBJTYPE const & x) {       // Put object into list
      if (n >= MAXSIZE) {
         return false;                 // Return false if list full
      }
      n++;                             // Increment count
      *head = x;                       // Copy x to list
      if (++head >= list + MAXSIZE) {  // Increment head pointer
         head = list;                  // Wrap around
      }
      return true;                     // Return true if success
   }
   OBJTYPE Get() {                     // Get object from list
      if (n <= 0) {
         // Error: list empty.
         // ... Put an error message here or return an empty object !
         exit(1);
      }
      n--;                             // Decrement count
      OBJTYPE * p = tail;              // Pointer to object
      if (++tail >= list + MAXSIZE) {  // Increment tail pointer
         tail = list;                  // Wrap around
      }
      return *p;                       // Return object
   }
   int NumObjects() {                  // Tell number of objects in list
      return n;                        // Return number of objects
   }
};


/******************************************************************************
Example part. Remove this from final application:
******************************************************************************/

int main() {

   FIFOlist <int,1000> List;           // Make list of max 1000 int
   List.Put(10);                       // Put 10 into the list
   List.Put(20);                       // Put 20 into the list
   List.Put(30);                       // Put 30 into the list
   while (List.NumObjects() > 0) {     // While list not empty
      printf("\n%3i ", List.Get());    // Get item from list and print
   }                                   // Will print "10 20 30 "

   return 0;
}
