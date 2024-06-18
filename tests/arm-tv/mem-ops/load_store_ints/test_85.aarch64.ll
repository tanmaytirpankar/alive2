@a = external global i85

define void @f() {
  %1 = load i85, ptr @a, align 1
  %x = add i85 %1, 1
  store i85 %x, ptr @a, align 1
  ret void
}
