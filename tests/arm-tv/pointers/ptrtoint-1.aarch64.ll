@A1 = external global i32 align 4

define i64 @g1() {
  %1 = and i64 ptrtoint (ptr @A1 to i64), -1
  ret i64 %1
}
