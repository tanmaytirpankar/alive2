define <16 x i8> @cmhi16xi8(<16 x i8> %0, <16 x i8> %1) {
  %3 = icmp ugt <16 x i8> %0, %1
  %4 = sext <16 x i1> %3 to <16 x i8>
  ret <16 x i8> %4
}
