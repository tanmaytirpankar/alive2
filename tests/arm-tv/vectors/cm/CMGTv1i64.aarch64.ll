define <1 x i64> @smin_v1i64(<1 x i64> %0, <1 x i64> %1) {
  %3 = call <1 x i64> @llvm.smin.v1i64(<1 x i64> %0, <1 x i64> %1)
  ret <1 x i64> %3
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare <1 x i64> @llvm.smin.v1i64(<1 x i64>, <1 x i64>) #1
