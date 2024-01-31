; XFAIL: 

define i32 @f(i32 %x) {
  %y = add i32 %x, 77
  %z = call i32 @g(i32 1, i32 2, i32 3, i32 4, i32 %x, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12) 
  %r = add i32 %z, 33
  ret i32 %r
}

declare i32 @g(i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32)

