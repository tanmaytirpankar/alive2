; XFAIL: 

define i32 @f(i32 %x) {
  %y = add i32 %x, 77
  %a = call i32 @g(i32 1, i32%x, i32 -1, i32 %y)
  %b = call i32 @g(i32 0, i32%x, i32 %x, i32 %a)
  %c = call i32 @g(i32 3, i32%x, i32 %x, i32 %b)
  %d = call i32 @g(i32 2, i32%x, i32 %y, i32 %c)
  %e = call i32 @g(i32 1, i32%x, i32 -1, i32 %d)
  %f = call i32 @g(i32 0, i32%x, i32 %x, i32 %e)
  %g = call i32 @g(i32 3, i32%x, i32 %x, i32 %f)
  %h = call i32 @g(i32 2, i32%x, i32 %y, i32 %g)
  %i = call i32 @g(i32 1, i32%x, i32 -1, i32 %h)
  %j = call i32 @g(i32 0, i32%x, i32 %x, i32 %i)
  %k = call i32 @g(i32 3, i32%x, i32 %x, i32 %j)
  %l = call i32 @g(i32 2, i32%x, i32 %y, i32 %k)
  %r1 = add i32 %x, %f
  %r2 = add i32 %a, %b
  %r3 = add i32 %r1, %r2
  ret i32 %r3
}

declare i32 @g(i32, i32, i32, i32)

