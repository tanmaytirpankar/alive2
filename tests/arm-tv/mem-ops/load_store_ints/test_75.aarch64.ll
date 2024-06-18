@a = external global i75

define void @f() {
  %1 = load i75, ptr @a, align 1
  %x = add i75 %1, 1
  store i75 %x, ptr @a, align 1
  ret void
}
