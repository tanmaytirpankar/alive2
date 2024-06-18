@a = external global i13

define void @f() {
  %1 = load i13, ptr @a, align 1
  %x = add i13 %1, 1
  store i13 %x, ptr @a, align 1
  ret void
}
