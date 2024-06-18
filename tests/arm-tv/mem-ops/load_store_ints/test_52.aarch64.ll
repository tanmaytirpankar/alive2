@a = external global i52

define void @f() {
  %1 = load i52, ptr @a, align 1
  %x = add i52 %1, 1
  store i52 %x, ptr @a, align 1
  ret void
}
