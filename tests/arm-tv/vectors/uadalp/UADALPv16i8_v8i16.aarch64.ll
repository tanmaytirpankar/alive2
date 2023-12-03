define zeroext i16 @f(<16 x i8> %0, <16 x i8> %1) {
  %3 = zext <16 x i8> %0 to <16 x i16>
  %4 = call i16 @llvm.vector.reduce.add.v16i16(<16 x i16> %3)
  %5 = zext <16 x i8> %1 to <16 x i16>
  %6 = call i16 @llvm.vector.reduce.add.v16i16(<16 x i16> %5)
  %7 = add i16 %4, %6
  ret i16 %7
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i16 @llvm.vector.reduce.add.v16i16(<16 x i16>) #0

