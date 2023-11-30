@G1 = external global i32, align 1

define i64 @f1() {
  %1 = and i64 ptrtoint (ptr @G1 to i64), 1
  ret i64 %1
}
