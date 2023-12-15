declare <2 x i32> @llvm.smax.v2i32(<2 x i32>, <2 x i32>) #0

define <2 x i32> @smax2i32(<2 x i32> %0, <2 x i32> %1) {
  %3 = call <2 x i32> @llvm.smax.v2i32(<2 x i32> %0, <2 x i32> %1)
  ret <2 x i32> %3
}
