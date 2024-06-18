@a = external global i66

define void @f() {
  %1 = load i66, ptr @a, align 1
  %x = add i66 %1, 1
  store i66 %x, ptr @a, align 1
  ret void
}
