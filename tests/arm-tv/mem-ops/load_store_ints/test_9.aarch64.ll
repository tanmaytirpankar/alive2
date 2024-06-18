@a = external global i9

define void @f() {
  %1 = load i9, ptr @a, align 1
  %x = add i9 %1, 1
  store i9 %x, ptr @a, align 1
  ret void
}
