@a = external global i55

define void @f() {
  %1 = load i55, ptr @a, align 1
  %x = add i55 %1, 1
  store i55 %x, ptr @a, align 1
  ret void
}
