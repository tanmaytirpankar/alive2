define <8 x i16> @test12(<8 x i16> %0, <8 x i16> %1) {
  %3 = and <8 x i16> %0, %1
  ret <8 x i16> %3
}
