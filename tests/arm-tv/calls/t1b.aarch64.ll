
define i32 @f(i32 noundef %x) {
entry:
  %a = call i16 @g()
  %aext = sext i16 %a to i32
  %mul = shl nsw i32 %x, %aext
  ret i32 %mul
}

declare i16 @g()

