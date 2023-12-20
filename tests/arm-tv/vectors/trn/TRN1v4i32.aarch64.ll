define void @store_factor2_high2(ptr %0, ptr %1, <4 x i32> %2, <4 x i32> %3) {
  %5 = shufflevector <4 x i32> %2, <4 x i32> %3, <4 x i32> <i32 0, i32 4, i32 1, i32 6>
  %6 = shufflevector <4 x i32> %2, <4 x i32> %3, <4 x i32> <i32 2, i32 6, i32 3, i32 7>
  store <4 x i32> %5, ptr %0, align 4
  store <4 x i32> %6, ptr %1, align 4
  ret void
}
