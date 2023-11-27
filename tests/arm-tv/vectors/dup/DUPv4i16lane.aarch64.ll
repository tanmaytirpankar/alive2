define <4 x i16> @test_vdup_laneq_s16(<8 x i16> %0) {
  %2 = shufflevector <8 x i16> %0, <8 x i16> undef, <4 x i32> <i32 2, i32 2, i32 2, i32 2>
  ret <4 x i16> %2
}
