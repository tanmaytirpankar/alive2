define ptr @test_array2(ptr %0) {
  %2 = getelementptr inbounds i32, ptr %0, i64 4
  ret ptr %2
}
