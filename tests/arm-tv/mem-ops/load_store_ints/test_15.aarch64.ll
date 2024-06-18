@a = external global i15

define void @f() {
  %1 = load i15, ptr @a, align 1
  %x = add i15 %1, 1
  store i15 %x, ptr @a, align 1
  ret void
}
