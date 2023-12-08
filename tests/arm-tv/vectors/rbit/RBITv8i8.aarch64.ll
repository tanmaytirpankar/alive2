declare <8 x i8> @llvm.bitreverse.v8i8(<8 x i8>) #0

define <8 x i8> @g_vec(<8 x i8> %0) {
  %2 = call <8 x i8> @llvm.bitreverse.v8i8(<8 x i8> %0)
  ret <8 x i8> %2
}
