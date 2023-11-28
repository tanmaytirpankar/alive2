define <4 x i32> @variable_srl0(<4 x i32> %0, <4 x i32> %1) {
  %3 = lshr <4 x i32> %0, %1
  ret <4 x i32> %3
}
