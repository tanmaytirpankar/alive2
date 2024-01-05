define <2 x i32> @f(<2 x i32> %0, <2 x i32> %1, <2 x i32> %2) {
  %4 = mul <2 x i32> %1, %2
  %5 = add <2 x i32> %0, %4
  ret <2 x i32> %5
}
