define <4 x i16> @cmhi4xi16(<4 x i16> %0, <4 x i16> %1) {
  %3 = icmp ugt <4 x i16> %0, %1
  %4 = sext <4 x i1> %3 to <4 x i16>
  ret <4 x i16> %4
}
