@a = external global i79

define void @f() {
  %1 = load i79, ptr @a, align 1
  %x = add i79 %1, 1
  store i79 %x, ptr @a, align 1
  ret void
}
