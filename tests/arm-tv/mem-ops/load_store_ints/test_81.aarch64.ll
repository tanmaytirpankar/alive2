@a = external global i81

define void @f() {
  %1 = load i81, ptr @a, align 1
  %x = add i81 %1, 1
  store i81 %x, ptr @a, align 1
  ret void
}
