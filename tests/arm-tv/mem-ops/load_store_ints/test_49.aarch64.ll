@a = external global i49

define void @f() {
  %1 = load i49, ptr @a, align 1
  %x = add i49 %1, 1
  store i49 %x, ptr @a, align 1
  ret void
}
