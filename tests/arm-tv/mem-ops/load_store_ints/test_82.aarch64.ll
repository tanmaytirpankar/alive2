@a = external global i82

define void @f() {
  %1 = load i82, ptr @a, align 1
  %x = add i82 %1, 1
  store i82 %x, ptr @a, align 1
  ret void
}
