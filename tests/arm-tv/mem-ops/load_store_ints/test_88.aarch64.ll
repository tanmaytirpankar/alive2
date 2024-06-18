@a = external global i88

define void @f() {
  %1 = load i88, ptr @a, align 1
  %x = add i88 %1, 1
  store i88 %x, ptr @a, align 1
  ret void
}
