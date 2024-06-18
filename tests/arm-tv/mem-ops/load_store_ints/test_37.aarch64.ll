@a = external global i37

define void @f() {
  %1 = load i37, ptr @a, align 1
  %x = add i37 %1, 1
  store i37 %x, ptr @a, align 1
  ret void
}
