@a = external global i23

define void @f() {
  %1 = load i23, ptr @a, align 1
  %x = add i23 %1, 1
  store i23 %x, ptr @a, align 1
  ret void
}
