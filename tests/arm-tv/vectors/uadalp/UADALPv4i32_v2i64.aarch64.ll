define i64 @f(<4 x i32> %0, <4 x i32> %1) {
  %3 = zext <4 x i32> %0 to <4 x i64>
  %4 = call i64 @llvm.vector.reduce.add.v4i64(<4 x i64> %3)
  %5 = zext <4 x i32> %1 to <4 x i64>
  %6 = call i64 @llvm.vector.reduce.add.v4i64(<4 x i64> %5)
  %7 = add i64 %4, %6
  ret i64 %7
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i64 @llvm.vector.reduce.add.v4i64(<4 x i64>) #0
