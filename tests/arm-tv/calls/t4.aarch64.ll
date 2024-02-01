; XFAIL: 

define i32 @f(i32 %x) {
  %y = add i32 %x, 77
  %z = call i32 @g(i32 1, i32%x, i32 -1, i32 %y)
  %w = call i32 @g(i32 0, i32%z, i32 %x, i32 %y)
  %a = call i32 @g(i32 3, i32%z, i32 %x, i32 %y)
  %b = call i32 @g(i32 2, i32%x, i32 %y, i32 %a)
  %r1 = add i32 %z, %w
  %r2 = add i32 %a, %b
  %r3 = add i32 %r1, %r2
  ret i32 %r3
}

declare i32 @g(i32, i32, i32, i32)

