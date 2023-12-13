define <8 x i16> @test_v8i16_nosignbit(<8 x i16> %0, <8 x i16> %1) {
  %3 = and <8 x i16> %0, <i16 255, i16 255, i16 255, i16 255, i16 255, i16 255, i16 255, i16 255>
  %4 = lshr <8 x i16> %1, <i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1>
  %5 = icmp ult <8 x i16> %3, %4
  %6 = select <8 x i1> %5, <8 x i16> %3, <8 x i16> %4
  ret <8 x i16> %6
}
