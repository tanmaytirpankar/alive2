@a = external global i77

define void @f() {
  %1 = load i77, ptr @a, align 1
  %x = add i77 %1, 1
  store i77 %x, ptr @a, align 1
  ret void
}
