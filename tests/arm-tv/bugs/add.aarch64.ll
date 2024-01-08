define <1 x i64> @f(<2 x i32> %0) {
  %2 = add <2 x i32> %0, %0
  %3 = bitcast <2 x i32> %2 to <1 x i64>
  %4 = add <1 x i64> %3, %3
  ret <1 x i64> %4
}
