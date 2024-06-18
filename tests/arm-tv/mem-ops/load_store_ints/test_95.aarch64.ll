@a = external global i95

define void @f() {
  %1 = load i95, ptr @a, align 1
  %x = add i95 %1, 1
  store i95 %x, ptr @a, align 1
  ret void
}
