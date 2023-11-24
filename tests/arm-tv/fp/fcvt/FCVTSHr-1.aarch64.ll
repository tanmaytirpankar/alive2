define i8 @f16tos8(half %0) {
  %2 = fptosi half %0 to i8
  ret i8 %2
}
