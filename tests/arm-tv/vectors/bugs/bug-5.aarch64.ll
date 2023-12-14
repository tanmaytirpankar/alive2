define <16 x i8> @smulh_v16i8(<16 x i8> %0) {
  %2 = sext <16 x i8> %0 to <16 x i16>
  %3 = mul <16 x i16> %2, %2
  %4 = trunc <16 x i16> %3 to <16 x i8>
  ret <16 x i8> %4
}
