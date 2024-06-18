@a = external global i3

define void @f() {
  %1 = load i3, ptr @a, align 1
  %x = add i3 %1, 1
  store i3 %x, ptr @a, align 1
  ret void
}
