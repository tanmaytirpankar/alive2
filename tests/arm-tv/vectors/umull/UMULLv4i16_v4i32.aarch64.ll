define <4 x i32> @test_mul_v4i32_v4i16_minsize(<4 x i16> %0) {
  %2 = zext <4 x i16> %0 to <4 x i32>
  %3 = mul nuw nsw <4 x i32> %2, <i32 18778, i32 18778, i32 18778, i32 18778>
  ret <4 x i32> %3
}
