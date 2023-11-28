define void @fcmp_ult_v16f16(ptr %0, ptr %1, ptr %2) {
  %4 = load <16 x half>, ptr %0, align 32
  %5 = load <16 x half>, ptr %1, align 32
  %6 = fcmp ult <16 x half> %4, %5
  %7 = sext <16 x i1> %6 to <16 x i16>
  store <16 x i16> %7, ptr %2, align 32
  ret void
}
