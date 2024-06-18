@a = external global i33

define void @f() {
  %1 = load i33, ptr @a, align 1
  %x = add i33 %1, 1
  store i33 %x, ptr @a, align 1
  ret void
}
