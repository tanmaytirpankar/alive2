@a = external global i73

define void @f() {
  %1 = load i73, ptr @a, align 1
  %x = add i73 %1, 1
  store i73 %x, ptr @a, align 1
  ret void
}
