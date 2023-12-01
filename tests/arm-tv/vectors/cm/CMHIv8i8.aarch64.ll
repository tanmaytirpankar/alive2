define <8 x i8> @vcgtu8(ptr %0, ptr %1) {
  %3 = load <8 x i8>, ptr %0, align 8
  %4 = load <8 x i8>, ptr %1, align 8
  %5 = icmp ugt <8 x i8> %3, %4
  %6 = sext <8 x i1> %5 to <8 x i8>
  ret <8 x i8> %6
}
