define <8 x i16> @test_sshll_shl0_v8i8(<8 x i8> %0) {
  %2 = sext <8 x i8> %0 to <8 x i16>
  ret <8 x i16> %2
}
