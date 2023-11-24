define i32 @smovx4h(<4 x i16> %0) {
  %2 = extractelement <4 x i16> %0, i32 2
  %3 = sext i16 %2 to i32
  ret i32 %3
}
