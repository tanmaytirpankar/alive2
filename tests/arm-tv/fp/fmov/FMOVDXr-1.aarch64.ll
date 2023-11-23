define i64 @f8(double %0) {
  %2 = bitcast double %0 to i64
  ret i64 %2
}
