declare <4 x i32> @llvm.usub.sat.v4i32(<4 x i32>, <4 x i32>) #0

define <4 x i32> @usub_v4i32_vx(<4 x i32> %0, i32 %1) {
  %3 = insertelement <4 x i32> poison, i32 %1, i32 0
  %4 = shufflevector <4 x i32> %3, <4 x i32> poison, <4 x i32> zeroinitializer
  %5 = call <4 x i32> @llvm.usub.sat.v4i32(<4 x i32> %0, <4 x i32> %4)
  ret <4 x i32> %5
}
