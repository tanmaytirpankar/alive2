define <8 x i16> @test_vzip1q_p16(<8 x i16> %0, <8 x i16> %1) {
  %3 = shufflevector <8 x i16> %0, <8 x i16> %1, <8 x i32> <i32 0, i32 8, i32 1, i32 9, i32 2, i32 10, i32 3, i32 11>
  ret <8 x i16> %3
}
