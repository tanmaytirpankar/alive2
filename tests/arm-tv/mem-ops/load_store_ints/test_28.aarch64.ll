@a = external global i28

define void @f() {
  %1 = load i28, ptr @a, align 1
  %x = add i28 %1, 1
  store i28 %x, ptr @a, align 1
  ret void
}
