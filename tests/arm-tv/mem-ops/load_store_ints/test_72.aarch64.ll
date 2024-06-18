@a = external global i72

define void @f() {
  %1 = load i72, ptr @a, align 1
  %x = add i72 %1, 1
  store i72 %x, ptr @a, align 1
  ret void
}
