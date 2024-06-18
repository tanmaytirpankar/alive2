@a = external global i12

define void @f() {
  %1 = load i12, ptr @a, align 1
  %x = add i12 %1, 1
  store i12 %x, ptr @a, align 1
  ret void
}
