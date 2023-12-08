define <2 x i64> @test_v8i16_v8i16(i16 %0, i16 %1) {
  %3 = insertelement <8 x i16> undef, i16 %0, i32 0
  %4 = bitcast <8 x i16> %3 to <2 x i64>
  %5 = insertelement <8 x i16> undef, i16 %1, i32 0
  %6 = bitcast <8 x i16> %5 to <2 x i64>
  %7 = shufflevector <2 x i64> %4, <2 x i64> %6, <2 x i32> <i32 0, i32 2>
  ret <2 x i64> %7
}
