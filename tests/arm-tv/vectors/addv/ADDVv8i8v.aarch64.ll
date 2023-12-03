declare i8 @llvm.vector.reduce.add.v8i8(<8 x i8>) #0

define i8 @f(<8 x i8> %0) {
  %2 = call i8 @llvm.vector.reduce.add.v8i8(<8 x i8> %0)
  ret i8 %2
}

