define i8 @f(i8 %0) {
  %2 = call i8 @llvm.ctpop.i8(i8 %0)
  ret i8 %2
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i8 @llvm.ctpop.i8(i8) #0
