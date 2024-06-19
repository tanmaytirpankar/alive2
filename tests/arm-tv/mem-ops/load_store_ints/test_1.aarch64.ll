@a = external global i1

declare i1 @llvm.fshr.i1 (i1 %a, i1 %b, i1 %c)

define void @f() {
  %1 = load i1, ptr @a, align 1
  %r = call i1 @llvm.fshr.i1(i1 %1, i1 %1, i1 1)
  store i1 %r, ptr @a, align 1
  ret void
}
