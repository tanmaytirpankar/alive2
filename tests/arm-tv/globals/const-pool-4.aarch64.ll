define <4 x i32> @f(<4 x i32> %0) {
  %2 = add <4 x i32> %0, <i32 21, i32 undef, i32 8, i32 8>
  %3 = add <4 x i32> %2, <i32 2, i32 3, i32 undef, i32 2>
  ret <4 x i32> %3
}
