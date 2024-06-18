@a = external global i56

define void @f() {
  %1 = load i56, ptr @a, align 1
  %x = add i56 %1, 1
  store i56 %x, ptr @a, align 1
  ret void
}
