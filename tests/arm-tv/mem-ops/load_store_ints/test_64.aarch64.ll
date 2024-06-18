@a = external global i64

define void @f() {
  %1 = load i64, ptr @a, align 1
  %x = add i64 %1, 1
  store i64 %x, ptr @a, align 1
  ret void
}
