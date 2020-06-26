#ifndef UTIL_HPP
#define UTIL_HPP

#include <cstdint>
#include <assert.h>

uint64_t BTOL(const uint8_t *msb, uint8_t size)
{
   uint8_t be[8] = { 0 };

   for (int i = 0; i < size; i++)
   {
     be[i] = msb[i];
   }

   for (int i = 0; i < size/2; i++)
   {
      be[i]        =  be[i] ^ be[size-1-i];
      be[size-1-i] =  be[i] ^ be[size-1-i];
      be[i]        =  be[i] ^ be[size-1-i];
   }

   if (size == 8) 
   {
     return (*((uint64_t *)&be));
   }
   else if (size == 4)
   {
     return (*((uint32_t *)&be));
   }
   else if (size == 2)
   {
     return (*((uint16_t *)&be));
   }
   else
   {
     assert(false);
     return 0;
   }
}

void LTOB(uint64_t value, unsigned char *memory, int size)
{
  for (int i = 0; i < size; i++)
  {
    memory[size - 1 - i] = *((unsigned char *)(&value) + i);
  }
}

#endif