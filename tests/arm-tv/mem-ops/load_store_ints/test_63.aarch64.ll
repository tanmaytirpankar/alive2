@a = external global i63

define void @f() {
  %1 = load i63, ptr @a, align 1
  %x = add i63 %1, 1
  store i63 %x, ptr @a, align 1
  ret void
}
