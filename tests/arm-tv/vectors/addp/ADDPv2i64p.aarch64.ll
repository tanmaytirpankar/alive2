declare i64 @llvm.vector.reduce.add.v2i64(<2 x i64>) #0

define i64 @f(<2 x i64> %0) {
  %2 = call i64 @llvm.vector.reduce.add.v2i64(<2 x i64> %0)
  ret i64 %2
}
