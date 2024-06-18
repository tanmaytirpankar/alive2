@a = external global i27

define void @f() {
  %1 = load i27, ptr @a, align 1
  %x = add i27 %1, 1
  store i27 %x, ptr @a, align 1
  ret void
}
