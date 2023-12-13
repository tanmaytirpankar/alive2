define <4 x i32> @test_vmulq_lane_s32_0(<4 x i32> %0, <2 x i32> %1) {
  %3 = shufflevector <2 x i32> %1, <2 x i32> undef, <4 x i32> zeroinitializer
  %4 = mul <4 x i32> %3, %0
  ret <4 x i32> %4
}
