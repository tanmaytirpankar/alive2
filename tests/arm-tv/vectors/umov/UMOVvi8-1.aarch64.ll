define zeroext i8 @f1(<16 x i8> %0) {
  %2 = extractelement <16 x i8> %0, i32 3
  ret i8 %2
}
