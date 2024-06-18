@a = external global i8

define void @f() {
  %1 = load i8, ptr @a, align 1
  %x = add i8 %1, 1
  store i8 %x, ptr @a, align 1
  ret void
}
