#pragma once

#ifdef USE_X86INTRINSICS
#include <intrin.h>
#pragma intrinsic(_BitScanForward64)
#pragma intrinsic(_BitScanReverse64)

/**
 * bitScanForward
 * @param bb bitboard to scan
 * @precondition bb != 0
 * @return index (0..63) of least significant one bit
 */
int bit_scan_fw(U64 x) {
   unsigned long index;
   assert (x != 0);
   _BitScanForward64(&index, x);
   return (int) index;
}

/**
 * bitScanReverse
 * @param bb bitboard to scan
 * @precondition bb != 0
 * @return index (0..63) of most significant one bit
 */
int bit_scan_rv(U64 x) {
   unsigned long index;
   assert (x != 0);
   _BitScanReverse64(&index, x);
   return (int) index;
}
#else

/**
 * bitScanForward
 * @param bb bitboard to scan
 * @precondition bb != 0
 * @return index (0..63) of least significant one bit
 */
inline int bit_scan_fw(uint64_t x) 
{
   asm ("bsfq %0, %0" : "=r" (x) : "0" (x));
   return (int) x;
}

/**
 * bitScanReverse
 * @param bb bitboard to scan
 * @precondition bb != 0
 * @return index (0..63) of most significant one bit
 */
inline int bit_scan_rv(uint64_t x) {
   asm ("bsrq %0, %0" : "=r" (x) : "0" (x));
   return (int) x;
}
#endif

#include <cmath>
#include <iostream>
#include <bitset> 

inline uint64_t find_set_bit(uint64_t bits)
{
   if (!(bits && !(bits & (bits-1)))) return 0;
   return log2(bits) + 1;
}

inline void print_bitset(uint64_t x)
{
   std::bitset<64> pretty = x;
   for (size_t i = 0; i < 8; i++)
   {
      for (size_t y = 0; y < 8; y++)
      {
            std::cout << pretty[i * 8 + y];
      }
      std::cout << std::endl;
   }
   std::cout << std::endl;
}
