@a = external global i39

define void @f() {
  %1 = load i39, ptr @a, align 1
  %x = add i39 %1, 1
  store i39 %x, ptr @a, align 1
  ret void
}
