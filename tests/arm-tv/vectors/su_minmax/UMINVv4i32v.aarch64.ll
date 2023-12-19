define i32 @reduce_umin_failed(<2 x i32> %0) {
  %2 = shufflevector <2 x i32> %0, <2 x i32> poison, <4 x i32> <i32 2, i32 3, i32 0, i32 1>
  %3 = call i32 @llvm.vector.reduce.umin.v4i32(<4 x i32> %2)
  ret i32 %3
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i32 @llvm.vector.reduce.umin.v4i32(<4 x i32>) #0
