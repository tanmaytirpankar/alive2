@a = external global i57

define void @f() {
  %1 = load i57, ptr @a, align 1
  %x = add i57 %1, 1
  store i57 %x, ptr @a, align 1
  ret void
}
