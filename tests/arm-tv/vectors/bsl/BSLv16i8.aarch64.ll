define void @binsl_v16i8_i(ptr %0, ptr %1, ptr %2) {
  %4 = load <16 x i8>, ptr %1, align 16
  %5 = load <16 x i8>, ptr %2, align 16
  %6 = and <16 x i8> %4, <i8 -64, i8 -64, i8 -64, i8 -64, i8 -64, i8 -64, i8 -64, i8 -64, i8 -64, i8 -64, i8 -64, i8 -64, i8 -64, i8 -64, i8 -64, i8 -64>
  %7 = and <16 x i8> %5, <i8 63, i8 63, i8 63, i8 63, i8 63, i8 63, i8 63, i8 63, i8 63, i8 63, i8 63, i8 63, i8 63, i8 63, i8 63, i8 63>
  %8 = or <16 x i8> %6, %7
  store <16 x i8> %8, ptr %0, align 16
  ret void
}
