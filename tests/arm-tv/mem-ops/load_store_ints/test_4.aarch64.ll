@a = external global i4

define void @f() {
  %1 = load i4, ptr @a, align 1
  %x = add i4 %1, 1
  store i4 %x, ptr @a, align 1
  ret void
}
