define void @f(ptr %0, ptr %1) {
  %3 = getelementptr inbounds i8, ptr %0, i64 16
  %4 = getelementptr inbounds i8, ptr %0, i64 32
  %5 = getelementptr inbounds i8, ptr %0, i64 48
  %6 = load <2 x i8>, ptr %5, align 1
  %7 = load <2 x i8>, ptr %4, align 1
  %8 = load <2 x i8>, ptr %3, align 1
  %9 = load <2 x i8>, ptr %0, align 1
  %10 = shufflevector <2 x i8> %6, <2 x i8> %7, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
  %11 = shufflevector <2 x i8> %8, <2 x i8> %9, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
  %12 = shufflevector <4 x i8> %10, <4 x i8> %11, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  store <8 x i8> %12, ptr %1, align 1
  ret void
}
