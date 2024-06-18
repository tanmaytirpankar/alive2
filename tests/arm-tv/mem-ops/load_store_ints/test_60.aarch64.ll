@a = external global i60

define void @f() {
  %1 = load i60, ptr @a, align 1
  %x = add i60 %1, 1
  store i60 %x, ptr @a, align 1
  ret void
}
