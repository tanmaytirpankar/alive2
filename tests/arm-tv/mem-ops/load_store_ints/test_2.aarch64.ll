@a = external global i2

define void @f() {
  %1 = load i2, ptr @a, align 1
  %x = add i2 %1, 1
  store i2 %x, ptr @a, align 1
  ret void
}
