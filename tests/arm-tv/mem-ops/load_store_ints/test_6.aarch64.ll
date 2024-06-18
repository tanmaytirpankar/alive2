@a = external global i6

define void @f() {
  %1 = load i6, ptr @a, align 1
  %x = add i6 %1, 1
  store i6 %x, ptr @a, align 1
  ret void
}
