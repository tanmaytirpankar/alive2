@a = external global i59

define void @f() {
  %1 = load i59, ptr @a, align 1
  %x = add i59 %1, 1
  store i59 %x, ptr @a, align 1
  ret void
}
