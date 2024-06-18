@a = external global i50

define void @f() {
  %1 = load i50, ptr @a, align 1
  %x = add i50 %1, 1
  store i50 %x, ptr @a, align 1
  ret void
}
