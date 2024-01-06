define <8 x i8> @f(<8 x i8> %0, <8 x i8> %1, <8 x i8> %2) {
  %4 = icmp eq <8 x i8> %0, <i8 1, i8 1, i8 1, i8 1, i8 1, i8 1, i8 1, i8 1>
  %5 = icmp eq <8 x i8> %1, <i8 1, i8 1, i8 1, i8 1, i8 1, i8 1, i8 1, i8 1>
  %6 = xor <8 x i1> %5, <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>
  %7 = icmp eq <8 x i8> %2, <i8 1, i8 1, i8 1, i8 1, i8 1, i8 1, i8 1, i8 1>
  %8 = or <8 x i1> %4, %6
  %9 = and <8 x i1> %7, %8
  %10 = sext <8 x i1> %9 to <8 x i8>
  ret <8 x i8> %10
}
