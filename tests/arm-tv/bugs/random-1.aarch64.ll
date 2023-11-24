define i64 @f1(i64 %0, i32 %1) {
  %3 = sext i32 %1 to i64
  %4 = add i64 %0, %3
  ret i64 %4
}
