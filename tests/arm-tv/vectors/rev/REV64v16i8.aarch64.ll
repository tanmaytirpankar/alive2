declare <2 x i64> @llvm.bitreverse.v2i64(<2 x i64>) #0

define <2 x i64> @g_vec_2x64(<2 x i64> %0) {
  %2 = call <2 x i64> @llvm.bitreverse.v2i64(<2 x i64> %0)
  ret <2 x i64> %2
}
