@a = external global i34

define void @f() {
  %1 = load i34, ptr @a, align 1
  %x = add i34 %1, 1
  store i34 %x, ptr @a, align 1
  ret void
}
