define <4 x i16> @trunc_v4i64_to_v4i16(ptr %0) {
  %2 = load <4 x i64>, ptr %0, align 32
  %3 = trunc <4 x i64> %2 to <4 x i16>
  ret <4 x i16> %3
}
