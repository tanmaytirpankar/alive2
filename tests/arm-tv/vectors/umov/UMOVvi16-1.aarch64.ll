define zeroext i16 @f2(<4 x i16> %0) {
  %2 = extractelement <4 x i16> %0, i32 2
  ret i16 %2
}
