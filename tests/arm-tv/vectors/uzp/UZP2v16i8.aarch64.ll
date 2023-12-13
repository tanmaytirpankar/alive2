define void @pckod_v16i8_0(ptr %0, ptr %1, ptr %2) {
  %4 = load <16 x i8>, ptr %1, align 16
  %5 = load <16 x i8>, ptr %2, align 16
  %6 = shufflevector <16 x i8> %4, <16 x i8> %5, <16 x i32> <i32 1, i32 3, i32 5, i32 7, i32 9, i32 11, i32 13, i32 15, i32 17, i32 19, i32 21, i32 23, i32 25, i32 27, i32 29, i32 31>
  store <16 x i8> %6, ptr %0, align 16
  ret void
}
