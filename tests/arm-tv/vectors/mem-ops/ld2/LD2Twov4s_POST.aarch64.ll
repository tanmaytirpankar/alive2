; Function Attrs: nounwind
define void @f(ptr %0, ptr %1) {
  %3 = load <16 x i32>, ptr %0, align 64
  %4 = shufflevector <16 x i32> %3, <16 x i32> undef, <8 x i32> <i32 1, i32 3, i32 5, i32 7, i32 9, i32 11, i32 13, i32 15>
  store <8 x i32> %4, ptr %1, align 32
  ret void
}