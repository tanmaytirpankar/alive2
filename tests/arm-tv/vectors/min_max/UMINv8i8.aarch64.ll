define <8 x i8> @umin_v8i8(<8 x i8> %0, <8 x i8> %1) {
  %3 = call <8 x i8> @llvm.umin.v8i8(<8 x i8> %0, <8 x i8> %1)
  ret <8 x i8> %3
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare <8 x i8> @llvm.umin.v8i8(<8 x i8>, <8 x i8>) #1
