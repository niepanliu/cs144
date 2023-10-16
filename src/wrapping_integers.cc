#include "wrapping_integers.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  return zero_point + uint32_t(n);
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  uint64_t tow31 = 1UL << 31;
  uint64_t tow32 = 1UL << 32;
  Wrap32 checkpoint32 = wrap(checkpoint, zero_point);
  uint64_t dis = raw_value_ - checkpoint32.raw_value_;
  if(dis <= tow31)
    return checkpoint + dis;
  dis = tow32 - dis;
  if(checkpoint < dis)
    return checkpoint + tow32 - dis;
  return checkpoint - dis;
}
