define <2 x i64> @vmull_extvec_s32(<2 x i32> %0) {
  %2 = sext <2 x i32> %0 to <2 x i64>
  %3 = mul <2 x i64> %2, <i64 -1234, i64 -1234>
  ret <2 x i64> %3
}
