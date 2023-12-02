define i8 @f(<16 x i8> %0) {
  %2 = extractelement <16 x i8> %0, i32 0
  ret i8 %2
}
