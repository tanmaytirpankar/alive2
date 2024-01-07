declare <4 x i32> @llvm.bswap.v4i32(<4 x i32>) #0

define <4 x i32> @f(ptr %0) {
  %2 = load i32, ptr %0, align 4
  %3 = insertelement <4 x i32> zeroinitializer, i32 %2, i32 1
  %4 = call <4 x i32> @llvm.bswap.v4i32(<4 x i32> %3)
  ret <4 x i32> %4
}
