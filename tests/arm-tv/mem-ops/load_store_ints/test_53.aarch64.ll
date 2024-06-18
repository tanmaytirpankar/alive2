@a = external global i53

define void @f() {
  %1 = load i53, ptr @a, align 1
  %x = add i53 %1, 1
  store i53 %x, ptr @a, align 1
  ret void
}
