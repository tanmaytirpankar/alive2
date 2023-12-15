declare <8 x i16> @llvm.uadd.sat.v8i16(<8 x i16>, <8 x i16>) #0

define <8 x i16> @combine_no_overflow_v8i16(<8 x i16> %0, <8 x i16> %1) {
  %3 = lshr <8 x i16> %0, <i16 10, i16 10, i16 10, i16 10, i16 10, i16 10, i16 10, i16 10>
  %4 = lshr <8 x i16> %1, <i16 10, i16 10, i16 10, i16 10, i16 10, i16 10, i16 10, i16 10>
  %5 = call <8 x i16> @llvm.uadd.sat.v8i16(<8 x i16> %3, <8 x i16> %4)
  ret <8 x i16> %5
}

