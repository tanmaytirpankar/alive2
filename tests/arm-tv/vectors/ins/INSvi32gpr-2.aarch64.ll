
define <4 x i32> @fun5(<4 x i32> %0, <4 x i32> %1) {
  %3 = sdiv <4 x i32> %0, %1
  ret <4 x i32> %3
}
