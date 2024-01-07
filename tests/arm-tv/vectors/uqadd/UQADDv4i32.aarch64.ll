define <4 x i32> @f(ptr %0, ptr %1) {
  %3 = load <4 x i32>, ptr %0, align 16
  %4 = load <4 x i32>, ptr %1, align 16
  %5 = call <4 x i32> @llvm.uadd.sat.v4i32(<4 x i32> %3, <4 x i32> %4)
  ret <4 x i32> %5
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare <4 x i32> @llvm.uadd.sat.v4i32(<4 x i32>, <4 x i32>) #1
