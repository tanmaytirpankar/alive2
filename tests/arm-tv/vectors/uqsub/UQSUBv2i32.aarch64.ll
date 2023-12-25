define <2 x i32> @v_usubsat_v2i32(<2 x i32> %0, <2 x i32> %1) {
  %3 = call <2 x i32> @llvm.usub.sat.v2i32(<2 x i32> %0, <2 x i32> %1)
  ret <2 x i32> %3
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare <2 x i32> @llvm.usub.sat.v2i32(<2 x i32>, <2 x i32>) #0
