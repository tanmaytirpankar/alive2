@a = external global i18

define void @f() {
  %1 = load i18, ptr @a, align 1
  %x = add i18 %1, 1
  store i18 %x, ptr @a, align 1
  ret void
}
