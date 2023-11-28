define <2 x i64> @sdiv_constant_op0_not_undef_lane(i64 signext %0, <2 x i64> %1, <2 x i64> %2) {
  %4 = insertelement <2 x i64> %1, i64 %0, i32 0
  %5 = sdiv <2 x i64> <i64 5, i64 2>, %4
  ret <2 x i64> %4
}
