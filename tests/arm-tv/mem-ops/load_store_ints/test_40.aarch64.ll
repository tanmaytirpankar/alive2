@a = external global i40

define void @f() {
  %1 = load i40, ptr @a, align 1
  %x = add i40 %1, 1
  store i40 %x, ptr @a, align 1
  ret void
}
