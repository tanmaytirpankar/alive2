define <2 x i64> @test_mm_unpackhi_epi32(<2 x i64> %0, <2 x i64> %1) {
  %3 = bitcast <2 x i64> %0 to <4 x i32>
  %4 = bitcast <2 x i64> %1 to <4 x i32>
  %5 = shufflevector <4 x i32> %3, <4 x i32> %4, <4 x i32> <i32 2, i32 6, i32 3, i32 7>
  %6 = bitcast <4 x i32> %5 to <2 x i64>
  ret <2 x i64> %6
}
