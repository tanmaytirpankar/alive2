
@b = external dso_local global [5 x i32]

define void @test1() {
  br label %1

1:                                                ; preds = %1, %0
  store i32 0, ptr @b, align 4
  br label %1
}
