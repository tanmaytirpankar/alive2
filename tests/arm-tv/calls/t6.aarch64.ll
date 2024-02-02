; XFAIL:

define i1 @f(ptr %x) {
  %a = call i1 @g(ptr %x)
  ret i1 %a
}

declare i1 @g(ptr)

