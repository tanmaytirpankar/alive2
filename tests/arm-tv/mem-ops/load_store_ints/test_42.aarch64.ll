@a = external global i42

define void @f() {
  %1 = load i42, ptr @a, align 1
  %x = add i42 %1, 1
  store i42 %x, ptr @a, align 1
  ret void
}
