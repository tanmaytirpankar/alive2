@a = external global i30

define void @f() {
  %1 = load i30, ptr @a, align 1
  %x = add i30 %1, 1
  store i30 %x, ptr @a, align 1
  ret void
}
