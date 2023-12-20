define void @ilvev_v16i8_0(ptr %0, ptr %1, ptr %2) {
  %4 = load <16 x i8>, ptr %1, align 16
  %5 = load <16 x i8>, ptr %2, align 16
  %6 = shufflevector <16 x i8> %4, <16 x i8> %5, <16 x i32> <i32 0, i32 16, i32 2, i32 18, i32 4, i32 20, i32 6, i32 22, i32 8, i32 24, i32 10, i32 26, i32 12, i32 28, i32 14, i32 30>
  store <16 x i8> %6, ptr %0, align 16
  ret void
}
