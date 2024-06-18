@a = external global i21

define void @f() {
  %1 = load i21, ptr @a, align 1
  %x = add i21 %1, 1
  store i21 %x, ptr @a, align 1
  ret void
}
