@a = external global i41

define void @f() {
  %1 = load i41, ptr @a, align 1
  %x = add i41 %1, 1
  store i41 %x, ptr @a, align 1
  ret void
}
