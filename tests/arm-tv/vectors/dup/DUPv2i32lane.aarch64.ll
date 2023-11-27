define <2 x i32> @test_vdup_lane_s32(<2 x i32> %0) {
  %2 = shufflevector <2 x i32> %0, <2 x i32> undef, <2 x i32> <i32 1, i32 1>
  ret <2 x i32> %2
}
