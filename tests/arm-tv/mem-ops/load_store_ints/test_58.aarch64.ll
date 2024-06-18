@a = external global i58

define void @f() {
  %1 = load i58, ptr @a, align 1
  %x = add i58 %1, 1
  store i58 %x, ptr @a, align 1
  ret void
}
