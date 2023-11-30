define i32 @add_pair_v8i16_v8i32_zext(<8 x i16> %0, <8 x i16> %1) {
  %3 = zext <8 x i16> %0 to <8 x i32>
  %4 = call i32 @llvm.vector.reduce.add.v8i32(<8 x i32> %3)
  %5 = zext <8 x i16> %1 to <8 x i32>
  %6 = call i32 @llvm.vector.reduce.add.v8i32(<8 x i32> %5)
  %7 = add i32 %4, %6
  ret i32 %7
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i32 @llvm.vector.reduce.add.v8i32(<8 x i32>) #0


