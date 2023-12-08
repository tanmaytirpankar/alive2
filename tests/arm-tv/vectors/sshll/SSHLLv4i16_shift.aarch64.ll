define <4 x i32> @test_sshll_v4i16(<4 x i16> %0) {
  %2 = sext <4 x i16> %0 to <4 x i32>
  %3 = shl <4 x i32> %2, <i32 9, i32 9, i32 9, i32 9>
  ret <4 x i32> %3
}
