; CHECK: 1 incorrect

define i32 @add(i16, i16) {
2:
  %a = add i16 %0, %1
  %b = zext i16 %a to i32
  ret i32 %b
}
