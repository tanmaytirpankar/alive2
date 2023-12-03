define <2 x i32> @f(<2 x i32> %0) {
  %2 = and <2 x i32> %0, <i32 -17, i32 -17>
  ret <2 x i32> %2
}
