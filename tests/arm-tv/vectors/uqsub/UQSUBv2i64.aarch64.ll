define <2 x i64> @sub_umax_v2i64(<2 x i64> %0, <2 x i64> %1) {
  %3 = call <2 x i64> @llvm.umax.v2i64(<2 x i64> %0, <2 x i64> %1)
  %4 = sub <2 x i64> %3, %1
  ret <2 x i64> %4
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare <2 x i64> @llvm.umax.v2i64(<2 x i64>, <2 x i64>) #0
