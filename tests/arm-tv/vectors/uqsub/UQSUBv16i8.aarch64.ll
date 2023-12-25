define <16 x i8> @vqsubQu8(ptr %0, ptr %1) {
  %3 = load <16 x i8>, ptr %0, align 16
  %4 = load <16 x i8>, ptr %1, align 16
  %5 = call <16 x i8> @llvm.usub.sat.v16i8(<16 x i8> %3, <16 x i8> %4)
  ret <16 x i8> %5
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare <16 x i8> @llvm.usub.sat.v16i8(<16 x i8>, <16 x i8>) #1
