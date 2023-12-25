define <8 x i16> @test_x86_sse2_psubus_w(<8 x i16> %0, <8 x i16> %1) {
  %3 = icmp ugt <8 x i16> %0, %1
  %4 = select <8 x i1> %3, <8 x i16> %0, <8 x i16> %1
  %5 = sub <8 x i16> %4, %1
  ret <8 x i16> %5
}
