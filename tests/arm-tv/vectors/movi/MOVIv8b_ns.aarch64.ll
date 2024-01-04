; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare <8 x i8> @llvm.fshl.v8i8(<8 x i8>, <8 x i8>, <8 x i8>) #0

define <8 x i8> @f(<8 x i8> %0, <8 x i8> %1) {
  %3 = call <8 x i8> @llvm.fshl.v8i8(<8 x i8> %0, <8 x i8> %0, <8 x i8> %1)
  ret <8 x i8> %3
}
