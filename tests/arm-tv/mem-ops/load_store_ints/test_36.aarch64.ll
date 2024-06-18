@a = external global i36

define void @f() {
  %1 = load i36, ptr @a, align 1
  %x = add i36 %1, 1
  store i36 %x, ptr @a, align 1
  ret void
}
