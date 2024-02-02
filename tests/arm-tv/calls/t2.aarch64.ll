
define i32 @f(i32 %x) {
  %y = add i32 %x, 77
  %z = call i32 @g(i32 1, i32%x, i32 -1, i32 %y)
  ret i32 %z
}

declare i32 @g(i32, i32, i32, i32)

