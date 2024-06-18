@a = external global i7

define void @f() {
  %1 = load i7, ptr @a, align 1
  %x = add i7 %1, 1
  store i7 %x, ptr @a, align 1
  ret void
}
