@a = external global i97

define void @f() {
  %1 = load i97, ptr @a, align 1
  %x = add i97 %1, 1
  store i97 %x, ptr @a, align 1
  ret void
}
