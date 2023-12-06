define <4 x i32> @ult_one(<4 x i32> %0) {
  %2 = icmp ult <4 x i32> %0, <i32 1, i32 1, i32 1, i32 1>
  %3 = sext <4 x i1> %2 to <4 x i32>
  ret <4 x i32> %3
}
