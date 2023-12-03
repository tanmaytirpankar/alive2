define i32 @f(ptr %0) {
  %2 = load <8 x i32>, ptr %0, align 4
  %3 = getelementptr inbounds i32, ptr %0, i32 8
  %4 = load <16 x i32>, ptr %3, align 4
  %5 = call i32 @llvm.vector.reduce.add.v16i32(<16 x i32> %4)
  %6 = call i32 @llvm.vector.reduce.add.v8i32(<8 x i32> %2)
  %7 = add nsw i32 %5, %6
  ret i32 %7
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i32 @llvm.vector.reduce.add.v8i32(<8 x i32>) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i32 @llvm.vector.reduce.add.v16i32(<16 x i32>) #0
