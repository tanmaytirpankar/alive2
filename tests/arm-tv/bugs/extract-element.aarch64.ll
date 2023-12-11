define i8 @f4(<16 x i8> %0, i32 %1) {
  %3 = extractelement <16 x i8> %0, i32 %1
  ret i8 %3
}
