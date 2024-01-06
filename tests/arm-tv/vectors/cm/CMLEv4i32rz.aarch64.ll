define i32 @f(<4 x i32> %0, <4 x i32> %1) {
  %3 = icmp slt <4 x i32> %0, %1
  %4 = sext <4 x i1> %3 to <4 x i32>
  %5 = icmp sle <4 x i32> %4, zeroinitializer
  %6 = extractelement <4 x i1> %5, i32 1
  %7 = sext i1 %6 to i32
  ret i32 %7
}
