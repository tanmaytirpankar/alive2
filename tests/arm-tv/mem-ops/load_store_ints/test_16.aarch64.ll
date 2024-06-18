@a = external global i16

define void @f() {
  %1 = load i16, ptr @a, align 1
  %x = add i16 %1, 1
  store i16 %x, ptr @a, align 1
  ret void
}
