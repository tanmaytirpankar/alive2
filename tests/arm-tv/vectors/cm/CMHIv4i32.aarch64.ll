define zeroext i8 @test_extractelement_varible_v4i1(<4 x i32> %0, <4 x i32> %1, i32 %2) {
  %4 = icmp ugt <4 x i32> %0, %1
  %5 = extractelement <4 x i1> %4, i32 %2
  %6 = zext i1 %5 to i8
  ret i8 %6
}
