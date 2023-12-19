declare i1 @llvm.vector.reduce.or.v16i1(<16 x i1>) #0

; Function Attrs: nounwind
define i32 @reduce_or_v16(<16 x i8> %0, i32 %1, i32 %2) {
  %4 = icmp slt <16 x i8> %0, zeroinitializer
  %5 = call i1 @llvm.vector.reduce.or.v16i1(<16 x i1> %4)
  %6 = select i1 %5, i32 %1, i32 %2
  ret i32 %6
}
