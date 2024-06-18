@a = external global i80

define void @f() {
  %1 = load i80, ptr @a, align 1
  %x = add i80 %1, 1
  store i80 %x, ptr @a, align 1
  ret void
}
