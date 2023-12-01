define <2 x i64> @test_int_x86_xop_vpcmov(<2 x i64> %0, <2 x i64> %1, <2 x i64> %2) {
  %4 = xor <2 x i64> %2, <i64 -1, i64 -1>
  %5 = and <2 x i64> %0, %2
  %6 = and <2 x i64> %1, %4
  %7 = or <2 x i64> %5, %6
  ret <2 x i64> %7
}
