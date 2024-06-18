@a = external global i54

define void @f() {
  %1 = load i54, ptr @a, align 1
  %x = add i54 %1, 1
  store i54 %x, ptr @a, align 1
  ret void
}
