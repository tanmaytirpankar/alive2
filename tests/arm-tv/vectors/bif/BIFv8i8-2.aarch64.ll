define <2 x i32> @p_splatvec(<2 x i32> %0, <2 x i32> %1, <2 x i32> %2) {
  %4 = and <2 x i32> %0, %2
  %5 = xor <2 x i32> %2, <i32 -1, i32 -1>
  %6 = and <2 x i32> %5, %1
  %7 = xor <2 x i32> %4, %6
  ret <2 x i32> %7
}
