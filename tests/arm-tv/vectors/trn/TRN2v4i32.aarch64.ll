define <4 x i32> @test_same_vtrn2q_u32(<4 x i32> %0) {
  %2 = shufflevector <4 x i32> %0, <4 x i32> %0, <4 x i32> <i32 1, i32 5, i32 3, i32 7>
  ret <4 x i32> %2
}
