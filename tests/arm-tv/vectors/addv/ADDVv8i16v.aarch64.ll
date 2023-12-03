define i16 @f(<8 x i16> %0) {
  %2 = call i16 @llvm.vector.reduce.add.v8i16(<8 x i16> %0)
  ret i16 %2
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i16 @llvm.vector.reduce.add.v8i16(<8 x i16>) #0
