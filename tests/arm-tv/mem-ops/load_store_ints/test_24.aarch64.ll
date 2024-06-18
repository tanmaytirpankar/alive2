@a = external global i24

define void @f() {
  %1 = load i24, ptr @a, align 1
  %x = add i24 %1, 1
  store i24 %x, ptr @a, align 1
  ret void
}
