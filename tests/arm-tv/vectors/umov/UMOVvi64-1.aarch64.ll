define i64 @f4(<2 x i64> %0) {
  %2 = extractelement <2 x i64> %0, i32 1
  ret i64 %2
}
