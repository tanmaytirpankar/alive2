define i8 @f(<16 x i8> %0) {
  %2 = call i8 @llvm.vector.reduce.add.v16i8(<16 x i8> %0)
  ret i8 %2
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i8 @llvm.vector.reduce.add.v16i8(<16 x i8>) #0
