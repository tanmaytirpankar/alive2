define <8 x i16> @test4(<8 x i16> %0, <8 x i16> %1) {
  %3 = shufflevector <8 x i16> %1, <8 x i16> undef, <8 x i32> zeroinitializer
  %4 = lshr <8 x i16> %0, %3
  ret <8 x i16> %4
}
