define i1 @n24_ule(i32 %x, i32 %y) {
  %t = icmp ule i32 %x, %y    
  ret i1 %t
}