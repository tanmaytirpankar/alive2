@a = external global i71

define void @f() {
  %1 = load i71, ptr @a, align 1
  %x = add i71 %1, 1
  store i71 %x, ptr @a, align 1
  ret void
}
