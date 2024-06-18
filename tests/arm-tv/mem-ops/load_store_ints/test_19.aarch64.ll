@a = external global i19

define void @f() {
  %1 = load i19, ptr @a, align 1
  %x = add i19 %1, 1
  store i19 %x, ptr @a, align 1
  ret void
}
