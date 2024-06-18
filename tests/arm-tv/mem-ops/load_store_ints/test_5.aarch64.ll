@a = external global i5

define void @f() {
  %1 = load i5, ptr @a, align 1
  %x = add i5 %1, 1
  store i5 %x, ptr @a, align 1
  ret void
}
