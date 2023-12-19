define dso_local i16 @v4i16(<4 x i16> %0) local_unnamed_addr {
  %2 = call i16 @llvm.vector.reduce.smax.v4i16(<4 x i16> %0)
  ret i16 %2
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i16 @llvm.vector.reduce.smax.v4i16(<4 x i16>) #1
