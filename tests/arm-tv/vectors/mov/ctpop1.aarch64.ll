define void @f3(ptr %0, i64 %1) {
  %3 = zext i64 %1 to i128
  %4 = load i128, ptr %0, align 4
  %5 = xor i128 %4, 127
  %6 = zext i64 %1 to i128
  %7 = call i128 @llvm.ctpop.i128(i128 %4)
  %8 = and i128 %6, %7
  %9 = add i128 %5, %8
  store i128 %9, ptr %0, align 4
  ret void
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i128 @llvm.ctpop.i128(i128) #0

attributes #0 = { nocallback nofree nosync nounwind speculatable willreturn memory(none) }
