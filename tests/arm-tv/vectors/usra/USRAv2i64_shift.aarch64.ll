define <2 x i64> @rax1(<2 x i64> %0, <2 x i64> %1) {
  %3 = call <2 x i64> @llvm.fshl.v2i64(<2 x i64> %1, <2 x i64> %1, <2 x i64> <i64 1, i64 1>)
  %4 = xor <2 x i64> %0, %3
  ret <2 x i64> %4
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare <2 x i64> @llvm.fshl.v2i64(<2 x i64>, <2 x i64>, <2 x i64>) #0
