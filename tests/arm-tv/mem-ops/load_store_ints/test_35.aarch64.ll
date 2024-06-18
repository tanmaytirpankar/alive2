@a = external global i35

define void @f() {
  %1 = load i35, ptr @a, align 1
  %x = add i35 %1, 1
  store i35 %x, ptr @a, align 1
  ret void
}
