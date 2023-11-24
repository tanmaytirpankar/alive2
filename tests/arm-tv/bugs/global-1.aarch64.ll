@g3 = external hidden global i64

define i64 @load3() {
  %1 = load i64, ptr @g3, align 4
  ret i64 %1
}
