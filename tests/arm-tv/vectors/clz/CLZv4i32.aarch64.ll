declare <4 x i32> @llvm.ctlz.v4i32(<4 x i32>, i1 immarg) #0

define <4 x i32> @f5(<4 x i32> %0) {
  %2 = call <4 x i32> @llvm.ctlz.v4i32(<4 x i32> %0, i1 false)
  ret <4 x i32> %2
}
