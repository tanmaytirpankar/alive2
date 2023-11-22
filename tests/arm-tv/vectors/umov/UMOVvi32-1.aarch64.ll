define i32 @f3(<2 x i32> %0) {
  %2 = extractelement <2 x i32> %0, i32 1
  ret i32 %2
}

