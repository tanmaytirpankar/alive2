@a = external global i20

define void @f() {
  %1 = load i20, ptr @a, align 1
  %x = add i20 %1, 1
  store i20 %x, ptr @a, align 1
  ret void
}
