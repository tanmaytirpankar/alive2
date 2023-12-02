define i16 @f(<8 x i16> %0) {
  %2 = extractelement <8 x i16> %0, i32 0
  ret i16 %2
}
