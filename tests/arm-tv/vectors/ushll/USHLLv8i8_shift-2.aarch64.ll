define i32 @f(ptr %0) {
  %2 = load <4 x i8>, ptr %0, align 4
  %3 = extractelement <4 x i8> %2, i32 0
  %4 = zext i8 %3 to i32
  ret i32 %4
}
