define i11 @f(ptr %0, i32 %z) {
  %2 = tail call zeroext i11 %0(i32 %z)
  ret i11 %2
}