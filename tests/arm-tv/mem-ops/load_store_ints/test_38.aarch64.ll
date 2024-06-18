@a = external global i38

define void @f() {
  %1 = load i38, ptr @a, align 1
  %x = add i38 %1, 1
  store i38 %x, ptr @a, align 1
  ret void
}
