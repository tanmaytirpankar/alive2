define <2 x i32> @f(<2 x i32> %0, <2 x i32> %1) {
  %3 = tail call <2 x i32> @llvm.bswap.v2i32(<2 x i32> %0)
  %4 = or <2 x i32> %3, <i32 100001, i32 100001>
  ret <2 x i32> %4
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare <2 x i32> @llvm.bswap.v2i32(<2 x i32>) #0
