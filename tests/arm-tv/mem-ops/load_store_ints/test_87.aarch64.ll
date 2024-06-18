@a = external global i87

define void @f() {
  %1 = load i87, ptr @a, align 1
  %x = add i87 %1, 1
  store i87 %x, ptr @a, align 1
  ret void
}
