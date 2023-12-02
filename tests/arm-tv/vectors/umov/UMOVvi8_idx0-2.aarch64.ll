define i8 @f(i8 %0, ptr %1) {
  %3 = load <8 x i8>, ptr %1, align 4
  %4 = call i8 @llvm.vector.reduce.mul.v8i8(<8 x i8> %3)
  %5 = mul i8 %0, %4
  ret i8 %5
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i8 @llvm.vector.reduce.mul.v8i8(<8 x i8>) #0
