; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare <8 x i16> @llvm.ctpop.v8i16(<8 x i16>) #0

define <8 x i16> @f2(<8 x i16> %0) {
  %2 = call <8 x i16> @llvm.ctpop.v8i16(<8 x i16> %0)
  ret <8 x i16> %2
}

