@a = external global i10

define void @f() {
  %1 = load i10, ptr @a, align 1
  %x = add i10 %1, 1
  store i10 %x, ptr @a, align 1
  ret void
}
