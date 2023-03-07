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
int bit_scan_fw(uint64_t x) 
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
int bit_scan_rv(uint64_t x) {
   asm ("bsrq %0, %0" : "=r" (x) : "0" (x));
   return (int) x;
}
#endif