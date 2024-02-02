
define i32 @f(i32 %x) {
  %y = add i32 %x, 77
  %z = call i32 @g(i32 1, i32%x, i32 -1, i32 %y)
  %w = call i32 @g(i32 0, i32%z, i32 %x, i32 %y)
  %r = add i32 %z, %w
  ret i32 %r
}

declare i32 @g(i32, i32, i32, i32)

