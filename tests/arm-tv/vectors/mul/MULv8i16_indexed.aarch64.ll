define <8 x i16> @test_vmulq_laneq_u16_0(<8 x i16> %0, <8 x i16> %1) {
  %3 = shufflevector <8 x i16> %1, <8 x i16> undef, <8 x i32> zeroinitializer
  %4 = mul <8 x i16> %3, %0
  ret <8 x i16> %4
}
