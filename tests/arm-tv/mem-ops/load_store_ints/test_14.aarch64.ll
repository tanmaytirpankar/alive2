@a = external global i14

define void @f() {
  %1 = load i14, ptr @a, align 1
  %x = add i14 %1, 1
  store i14 %x, ptr @a, align 1
  ret void
}
