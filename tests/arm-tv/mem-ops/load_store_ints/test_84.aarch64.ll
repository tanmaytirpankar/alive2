@a = external global i84

define void @f() {
  %1 = load i84, ptr @a, align 1
  %x = add i84 %1, 1
  store i84 %x, ptr @a, align 1
  ret void
}
