@a = external global i76

define void @f() {
  %1 = load i76, ptr @a, align 1
  %x = add i76 %1, 1
  store i76 %x, ptr @a, align 1
  ret void
}
