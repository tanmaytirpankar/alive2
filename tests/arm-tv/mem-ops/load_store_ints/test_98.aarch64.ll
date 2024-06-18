@a = external global i98

define void @f() {
  %1 = load i98, ptr @a, align 1
  %x = add i98 %1, 1
  store i98 %x, ptr @a, align 1
  ret void
}
