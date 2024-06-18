@a = external global i65

define void @f() {
  %1 = load i65, ptr @a, align 1
  %x = add i65 %1, 1
  store i65 %x, ptr @a, align 1
  ret void
}
