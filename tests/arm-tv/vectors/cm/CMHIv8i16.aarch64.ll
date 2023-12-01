define <8 x i16> @f7(<8 x i16> %0, <8 x i16> %1, <8 x i16> %2) {
  %4 = icmp ugt <8 x i16> %1, %2
  %5 = sext <8 x i1> %4 to <8 x i16>
  ret <8 x i16> %5
}
