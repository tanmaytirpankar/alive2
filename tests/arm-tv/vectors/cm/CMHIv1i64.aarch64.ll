define dso_local i64 @v2i64(<2 x i64> %0) local_unnamed_addr {
  %2 = call i64 @llvm.vector.reduce.umax.v2i64(<2 x i64> %0)
  ret i64 %2
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i64 @llvm.vector.reduce.umax.v2i64(<2 x i64>) #1

